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

    /// Singleton access of the WSServer instance
    static WSServer& GetInstance();

    /**
     * @brief Poll the server, and make sure the client is authenticated.
     */
    void Poll();

    /**
     * @brief Send a message to the client.
     * @param message The message to send.
     */
    void Send(const std::string& message);

    /**
     * @brief Close the server.
     */
    void CloseServer();

    /**
     * @brief Get if the client is authented.
     * @return If the client is authenticated or not.
     */
    bool GetAuthenticated() const { return authenticated; }

    private:
    /// Private constructor for singleton pattern
    WSServer();
    WSServer(const WSServer&) = delete;
    WSServer& operator=(const WSServer&) = delete;

    /// Callbacks for websocket events message, open, and close
    void OnMessage(websocketpp::connection_hdl hdl, websocketpp::server<websocketpp::config::asio>::message_ptr msg);
    void OnOpen(websocketpp::connection_hdl hdl);
    void OnClose(websocketpp::connection_hdl hdl);

    websocketpp::server<websocketpp::config::asio> ws_server; /// Instance of the ws server
    std::optional<websocketpp::connection_hdl> client;        /// The client, only one at a time
    bool authenticated = false;                               /// If the client is authenticated or not
    OpenGuard::Utils::Timer auth_timer;                       /// How much the client has to authenticate before getting kicked.
};

#endif // OPENGUARD_WS_SERVER_HPP
