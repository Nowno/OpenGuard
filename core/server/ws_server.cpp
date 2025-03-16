#include "ws_server.hpp"
#include "../../utils/logger/logger.hpp"
#include <json/json.hpp>

using json = nlohmann::json;

WSServer& WSServer::GetInstance()
{
    static WSServer instance;
    return instance;
}

WSServer::WSServer()
{
    wsServer.init_asio();
    wsServer.set_open_handler(std::bind(&WSServer::OnOpen, this, std::placeholders::_1));
    wsServer.set_close_handler(std::bind(&WSServer::OnClose, this, std::placeholders::_1));
    wsServer.set_message_handler(std::bind(&WSServer::OnMessage, this, std::placeholders::_1, std::placeholders::_2));

    wsServer.clear_access_channels(websocketpp::log::alevel::all);

    wsServer.listen(9002);
    wsServer.start_accept();
    Logger::GetInstance().Log("INFO", "WebSocket Server started on ws://localhost:9002");
}


void WSServer::Poll()
{
    wsServer.poll();
}

void WSServer::OnOpen(websocketpp::connection_hdl hdl)
{
    if (client)
    {
        Logger::GetInstance().Log("INFO", "Existing client disconnected, accepting new connection.");
        try
        {
            wsServer.close(client.value(), websocketpp::close::status::going_away, "Reconnecting...");
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
}


void WSServer::OnClose(websocketpp::connection_hdl hdl)
{
    if (client && client.value().lock() == hdl.lock())
    {
        Logger::GetInstance().Log("INFO", "Client disconnected.");
        client.reset();
    }
}

void WSServer::OnMessage(websocketpp::connection_hdl hdl, websocketpp::server<websocketpp::config::asio>::message_ptr msg)
{
    std::string command = msg->get_payload();
    Logger::GetInstance().Log("INFO", "Received command: " + command);
}


void WSServer::Send(const std::string& message)
{
    if (!client)
    {
        Logger::GetInstance().Log("ERROR", "No client connected.");
        return;
    }

    try
    {
        wsServer.send(client.value(), message, websocketpp::frame::opcode::text);
    }
    catch (const websocketpp::exception &e)
    {
        Logger::GetInstance().Log("ERROR", "Failed to send message: " + std::string(e.what()));
    }
}
