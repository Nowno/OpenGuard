#ifndef OPENGUARD_WS_SERVER_HPP
#define OPENGUARD_WS_SERVER_HPP

#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_STL_

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <optional>

#include "../../utils/utils.hpp"

class WSServer
{
    public:
    static WSServer& GetInstance();

    void Poll();
    void Send(const std::string& message);
    bool GetAuthenticated() const { return authenticated; }

    private:
    WSServer();
    WSServer(const WSServer&) = delete;
    WSServer& operator=(const WSServer&) = delete;

    void OnMessage(websocketpp::connection_hdl hdl, websocketpp::server<websocketpp::config::asio>::message_ptr msg);
    void OnOpen(websocketpp::connection_hdl hdl);
    void OnClose(websocketpp::connection_hdl hdl);

    websocketpp::server<websocketpp::config::asio> wsServer;
    std::optional<websocketpp::connection_hdl> client;
    bool authenticated = false;
    OpenGuard::Utils::Timer auth_timer;
};

#endif // OPENGUARD_WS_SERVER_HPP
