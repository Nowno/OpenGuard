#include <json/json.hpp>

#include "ws_server.hpp"
#include "../../utils/logger/logger.hpp"
#include "../../utils/config_manager/config_manager.hpp"
#include "../command_processor/command_processor.hpp"

using json = nlohmann::json;

WSServer& WSServer::GetInstance()
{
    static WSServer instance;
    return instance;
}

WSServer::WSServer()
{
    ws_server.init_asio();
    ws_server.set_open_handler(std::bind(&WSServer::OnOpen, this, std::placeholders::_1));
    ws_server.set_close_handler(std::bind(&WSServer::OnClose, this, std::placeholders::_1));
    ws_server.set_message_handler(std::bind(&WSServer::OnMessage, this, std::placeholders::_1, std::placeholders::_2));

    ws_server.clear_access_channels(websocketpp::log::alevel::all);

    int server_port = ConfigManager::GetInstance().GetConfig<int>("server_port");

    ws_server.listen(server_port);
    ws_server.start_accept();
    Logger::GetInstance().Log("INFO", "WebSocket Server started on ws://localhost:" + std::to_string(server_port));
}


void WSServer::Poll()
{
    if (client && !authenticated)
    {
        if (auth_timer.HasElapsed(6))
        {
            Logger::GetInstance().Log("ERROR", "Client failed to authenticate within 5 seconds, closing connection.");
            ws_server.close(client.value(), websocketpp::close::status::policy_violation, "Authentication timeout.");
            client.reset();
        }
    }
    ws_server.poll();
}

void WSServer::OnOpen(websocketpp::connection_hdl hdl)
{
    if (client)
    {
        Logger::GetInstance().Log("INFO", "Existing client disconnected, accepting new connection.");
        try
        {
            ws_server.close(client.value(), websocketpp::close::status::going_away, "Reconnecting...");
        }
        catch (const websocketpp::exception& e)
        {
            Logger::GetInstance().Log("ERROR", "Failed to close old client: " + std::string(e.what()));
        }
        client.reset();
    }

    Logger::GetInstance().Log("INFO", "New client connected.");
    client = hdl;

    json response = {{"type", "status"}, {"message", "connected"}};
    Send(response.dump());

    this->authenticated = false;
    this->auth_timer.Reset();
}


void WSServer::OnClose(websocketpp::connection_hdl hdl)
{
    if (client && client.value().lock() == hdl.lock())
    {
        this->authenticated = false;
        Logger::GetInstance().Log("INFO", "Client disconnected.");
        CommandProcessor::GetInstance().SetStreaming("snapshot", false);
        CommandProcessor::GetInstance().SetStreaming("log", false);
        client.reset();
    }
}


void WSServer::OnMessage(websocketpp::connection_hdl hdl, websocketpp::server<websocketpp::config::asio>::message_ptr msg)
{
    std::string command = msg->get_payload();
    Logger::GetInstance().Log("INFO", "Received command: " + command);

    auto response = CommandProcessor::GetInstance().Process(command);

    if (!authenticated)
    {
        if (response != "authenticated")
        {
            ws_server.close(hdl, websocketpp::close::status::policy_violation, "Authentication failed.");
            client.reset();
        }
        else
        {
            authenticated = true;
        }
    }
}


void WSServer::Send(const std::string& message)
{
    if (!client)
    {
        Logger::GetInstance().Log("ERROR", "No client connected.");
        return;
    }

    OpenGuard::Utils::SafeCall([&]()
    {
        ws_server.send(client.value(), message, websocketpp::frame::opcode::text);
    });
}

void WSServer::CloseServer()
{
    if (client)
    {
        /// For some reason status::going_away doesn't inform the client in time. Use policy_violation as a workaround.
        ws_server.close(client.value(), websocketpp::close::status::policy_violation, "Server closing.");
        client.reset();
    }

    /// Process pending events, such as the queued frame for the close event.
    for (int i = 0; i < 20; i++)
    {
        ws_server.poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    /// Stop listening and close the server to allow for a new instance to be created.
    ws_server.stop_listening();
    ws_server.stop();
}