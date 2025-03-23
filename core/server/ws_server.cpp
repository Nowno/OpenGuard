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

/**
 * @brief Initialize the ws server
 */
WSServer::WSServer()
{
    ws_server.init_asio();

    /// Set the callbacks for the server
    ws_server.set_open_handler(std::bind(&WSServer::OnOpen, this, std::placeholders::_1));
    ws_server.set_close_handler(std::bind(&WSServer::OnClose, this, std::placeholders::_1));
    ws_server.set_message_handler(std::bind(&WSServer::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
    ws_server.set_fail_handler([](websocketpp::connection_hdl hdl){Logger::GetInstance().Log("ERROR", "Connection failed.");});

    /// Disable logging as we don't want it flooding the console, and we handle it ourselves.
    ws_server.clear_access_channels(websocketpp::log::alevel::all);

    /// Retrieve the server port from the config
    int server_port = ConfigManager::GetInstance().GetConfig<int>("server_port");

    /// Listen on the specified port
    ws_server.listen(server_port);

    /// Start accepting connections
    ws_server.start_accept();
    Logger::GetInstance().Log("INFO", "WebSocket Server started on ws://localhost:" + std::to_string(server_port));
}

/**
 * @brief Poll the server, and make sure the client is authenticated.
 */
void WSServer::Poll()
{
    /// If a client connected and didn't authenticate within 6 seconds, close the connection.
    if (client && !authenticated)
    {
        if (auth_timer.HasElapsed(6))
        {
            Logger::GetInstance().Log("ERROR", "Client failed to authenticate within 5 seconds, closing connection.");
            ws_server.close(client.value(), websocketpp::close::status::policy_violation, "Authentication timeout.");
            client.reset();
        }
    }

    /// Process the queued events.
    /// This is is fine for our usecase, we don't need super low latency.
    ws_server.poll();
}

/**
 * @brief Callback for when a client connects.
 */
void WSServer::OnOpen(websocketpp::connection_hdl hdl)
{
    /// If we already have a client, close the connection.
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

    /// Let the client know they are connected.
    json response = {{"type", "status"}, {"message", "connected"}};
    Send(response.dump());

    /// Reset the auth state and start the timer.
    this->authenticated = false;
    this->auth_timer.Reset();
}

/**
 * @brief Callback for when a client disconnects.
 */
void WSServer::OnClose(websocketpp::connection_hdl hdl)
{
    if (client && client.value().lock() == hdl.lock())
    {
        /// Reset the auth state.
        this->authenticated = false;

        Logger::GetInstance().Log("INFO", "Client disconnected.");

        /// Stop streaming anything we were if the client disconnects.
        CommandProcessor::GetInstance().SetStreaming(CommandProcessor::Stream::SNAPSHOT, false);
        CommandProcessor::GetInstance().SetStreaming(CommandProcessor::Stream::LOG, false);
        client.reset();
    }
}

/**
 * @brief Callback for when a client sends a message.
 */
void WSServer::OnMessage(websocketpp::connection_hdl hdl, websocketpp::server<websocketpp::config::asio>::message_ptr msg)
{
    /// Get the message payload and print it.
    std::string command = msg->get_payload();
    Logger::GetInstance().Log("INFO", "Received command: " + command);

    /// Here we call our command processor to handle the command, the server only relays the message.
    /// I believe the correct name for this is command pattern. This allows for future expandability.
    auto response = CommandProcessor::GetInstance().Process(command);

    /// If the client wasn't authenticated, check if our command processor returned authenticated
    if (!authenticated)
    {
        /// If it didn't, the user failed to authenticate, close the connection.
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

/**
 * @brief Send a message to the client.
 */
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

/**
 * @brief Close the server.
 */
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