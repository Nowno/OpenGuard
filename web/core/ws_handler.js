const WS_URL = "ws://localhost:9002";

class WebSocketHandler
{
    constructor()
    {
        this.websocket = null;
        this.manual_disconnect = false;
        this.is_reconnecting = false;
        this.reconnect_attemps = 0;
        this.snapshot_requested = false;
        this.reconnect_timer = null;
        this.status_call_back = null;
        this.message_call_back = null;
    }

    InitHandler(status_call_back, message_call_back)
    {
        if (this.websocket && this.websocket.readyState === WebSocket.OPEN)
        {
            console.log("⚠️ Already connected, aborting.");
            return;
        }

        if (this.is_reconnecting)
            return;

        this.is_reconnecting = true;
        this.status_call_back = status_call_back;
        this.message_call_back = message_call_back;
        this.status_call_back("Connecting...");

        this.websocket = new WebSocket(WS_URL);

        this.websocket.onopen = () =>
        {
            console.log("✅ Connected ");

            this.status_call_back("Connected");
            this.reconnect_attemps = 0;
            this.is_reconnecting = false;

            clearTimeout(this.reconnect_timer);



            if (this.snapshot_requested)
            {
                this.SendCommand("start_stream");
            }
        };

        this.websocket.onmessage = (event) =>
        {
            if (this.message_call_back)
            {
                this.message_call_back(JSON.parse(event.data));
            }
        };

        this.websocket.onclose = (event) =>
        {
            if (this.manual_disconnect)
                return;

            this.reconnect_attemps++;
            this.is_reconnecting = false;
            this.status_call_back("Disconnected");
            console.log(`❌ Closed (${event.code}). Retrying in 5s...`);


            clearTimeout(this.reconnect_timer);
            this.reconnect_timer = setTimeout(() => this.InitHandler(this.status_call_back, this.message_call_back), 5000);
        };

        this.websocket.onerror = (error) =>
        {
            console.error("⚠️ WebSocket Error:", error);
            this.is_reconnecting = false;
            this.status_call_back("Disconnected");
        };
    }

    SendCommand(command)
    {
        if (this.websocket && this.websocket.readyState === WebSocket.OPEN)
        {
            this.websocket.send(JSON.stringify({ type: command }));
        }
        else
        {
            console.error("⚠️ Server not found.");
        }
    }

    CloseWebSocket()
    {
        this.manual_disconnect = true;
        if (this.websocket) this.websocket.close();
    }

    RequestLiveFeed()
    {
        this.snapshot_requested = true;
        this.SendCommand("start_stream");
    }

    StopLiveFeed()
    {
        this.snapshot_requested = false;
        this.SendCommand("stop_stream");
    }
}

const websocketHandler = new WebSocketHandler();
export default websocketHandler;
