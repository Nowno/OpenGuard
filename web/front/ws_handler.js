const WS_URL = `ws://${window.location.hostname}:3001`;

class WebSocketHandler
{
    constructor()
    {
        this.websocket = null;         /// The websocket object
        this.is_authenticated = false; /// If we are authenticated
        this.is_connecting = false;    /// if we are currently trying to connect
        this.status_callback = null;   /// Callback for status changes
        this.og_config = null;         /// config file sent upon authentication
        this.message_callbacks = [];   /// List of message callbacks
        this.close_callbacks = [];     /// List of close callbacks
    }

    InitHandler(status_callback, message_callback)
    {
        /// set the status callback
        this.status_callback = status_callback;

        /// If we don't have a message callback, add it to the list
        if (message_callback && !this.message_callbacks.includes(message_callback))
        {
            this.message_callbacks.push(message_callback);
        }

        /// If we are already connected or connecting, return
        if (this.is_connecting || this.websocket && this.websocket.readyState === WebSocket.OPEN)
        {
            return;
        }

        /// update connection status and create a new websocket object
        this.is_connecting = true;
        this.websocket = new WebSocket(WS_URL);

        this.websocket.onopen = () =>
        {
            this.is_connecting = false;
            this.status_callback("Connected");
        };

        this.websocket.onmessage = (event) =>
        {
            try
            {
                const data = JSON.parse(event.data);

                if (data.type === "login_success")
                {
                    this.is_authenticated = true;
                }
                else if (data.type === "error" && data.message === "Unauthorized")
                {
                    /// If we receive an unauthorized error, set the authenticated status to false
                    /// we do this to prompt the user to login again
                    this.is_authenticated = false;
                    this.status_callback("Unauthorized");
                }
                else if (data.type === "config")
                {
                    /// If we receive a config file, store it
                    this.og_config = data.config;
                }

                /// Call all the message callbacks
                this.message_callbacks.forEach(callback => callback(data));
            }
            catch (error)
            {
                console.error("⚠️ WebSocket Error:", error);
            }
        };

        /// If the connection is closed, continuously try to reconnect
        this.websocket.onclose = () =>
        {
            console.warn("❌ WebSocket disconnected. Retrying in 5s...");

            /// reset all our states
            this.is_connecting = false;
            this.status_callback("Disconnected");
            this.og_config = null;

            /// Call all the close callbacks
            this.close_callbacks.forEach(callback => callback());

            /// Try to reconnect
            setTimeout(() => this.InitHandler(this.status_callback), 5000);
        };

        this.websocket.onerror = (error) =>
        {
            console.error("⚠️ WebSocket Error:", error);

            this.og_config = null;
            this.is_connecting = false;
            this.status_callback("Disconnected");
        };
    }

    SendLogin(username, password)
    {
        /// If we are not connected, return
        if (!this.websocket || this.websocket.readyState !== WebSocket.OPEN) return;

        /// Send the login request to the ws backend
        this.websocket.send(JSON.stringify({ type: "login", username, password }));
    }

    SendCommand(command, data = {})
    {
        /// Same chk as above
        if (!this.websocket || this.websocket.readyState !== WebSocket.OPEN) return;

        /// Send the command to the ws backend
        this.websocket.send(JSON.stringify({ type: command, ...data }));
    }
    CloseConnection()
    {
        if (!this.websocket) return;

        this.websocket.close();
    }

    /// Subscribe to the desired event, providing a callback
    Subscribe(event, callback)
    {
        if (event === "message")
        {
            this.message_callbacks.push(callback);
        }
        else if (event === "close")
        {
            this.close_callbacks.push(callback);
        }
    }

    /// Unsubscribe from the desired event, providing a callback
    Unsubscribe(event, callback)
    {
        if (event === "message")
        {
            this.message_callbacks = this.message_callbacks.filter(cb => cb !== callback);
        }
        else if (event === "close")
        {
            this.close_callbacks = this.close_callbacks.filter(cb => cb !== callback);
        }
    }

}

const websocket_handler = new WebSocketHandler();
export default websocket_handler;