const WS_URL = `ws://${window.location.hostname}:3001`;

class WebSocketHandler
{
    constructor()
    {
        this.websocket = null;
        this.is_authenticated = false;
        this.is_connecting = false;
        this.status_callback = null;
        this.og_config = null;
        this.message_callbacks = [];
        this.close_callbacks = [];
    }

    InitHandler(status_callback, message_callback)
    {
        this.status_callback = status_callback;

        if (message_callback && !this.message_callbacks.includes(message_callback))
        {
            this.message_callbacks.push(message_callback);
        }

        if (this.is_connecting || this.websocket && this.websocket.readyState === WebSocket.OPEN)
        {
            return;
        }

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
                    this.is_authenticated = false;
                    this.status_callback("Unauthorized");
                }
                else if (data.type === "config")
                {
                    this.og_config = data.config;
                }

                this.message_callbacks.forEach(callback => callback(data));
            }
            catch (error)
            {
                console.error("⚠️ WebSocket Error:", error);
            }
        };

        this.websocket.onclose = () =>
        {
            console.warn("❌ WebSocket disconnected. Retrying in 5s...");

            this.is_connecting = false;
            this.status_callback("Disconnected");

            this.close_callbacks.forEach(callback => callback());
            setTimeout(() => this.InitHandler(this.status_callback), 5000);
        };

        this.websocket.onerror = (error) =>
        {
            console.error("⚠️ WebSocket Error:", error);

            this.is_connecting = false;
            this.status_callback("Disconnected");
        };
    }

    SendLogin(username, password)
    {
        if (!this.websocket || this.websocket.readyState !== WebSocket.OPEN) return;

        this.websocket.send(JSON.stringify({ type: "login", username, password }));
    }

    SendCommand(command, data = {})
    {
        if (!this.websocket || this.websocket.readyState !== WebSocket.OPEN) return;

        this.websocket.send(JSON.stringify({ type: command, ...data }));
    }
    CloseConnection()
    {
        if (!this.websocket) return;

        this.websocket.close();
    }
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