import { WebSocket, WebSocketServer } from "ws";
import config from "./config_manager.js";
import Auth from "./auth.js";

class WebSocketHandler
{
    constructor()
    {
        this.clients = new Map();
        this.openguard_ws = null;
        this.reconnect_interval = 5000;
        this.is_openguard_connected = false;
        this.is_paused = false;
        this.resume_time = null;
        this.resume_timer = null;
        this.active_viewers = 0;
    }

    StartServer()
    {
        this.server = new WebSocketServer({ port: config.ws.port });
        console.log(`✅ WebSocket server running on ws://localhost:${config.ws.port}`);

        this.server.on("connection", (ws) =>
        {
            console.log("🔗 New frontend client connected.");
            this.clients.set(ws, { authenticated: false });

            this.SendInitialStatus(ws);

            ws.on("message", (message) => this.HandleMessage(ws, message));
            ws.on("close", () => this.RemoveClient(ws));
            ws.on("error", () => this.RemoveClient(ws));
        });

        this.ConnectToOpenGuard();
    }

    SendInitialStatus(ws)
    {
        ws.send(JSON.stringify({ type: "openguard_status", status: this.is_openguard_connected ? "Connected" : "Disconnected" }));
        ws.send(JSON.stringify({ type: "pause_status", isPaused: this.is_paused, resumeTime: this.resume_time }));
    }

    RemoveClient(ws)
    {
        this.clients.delete(ws);
        this.UpdateViewers();
    }

    HandleMessage(ws, message)
    {
        try
        {
            const data = JSON.parse(message);
            if (data.type === "login")
            {
                this.HandleLogin(ws, data);
            }
            else if (this.clients.get(ws)?.authenticated)
            {
                this.ProcessClientCommand(ws, data);
            }
            else
            {
                ws.send(JSON.stringify({ type: "error", message: "Unauthorized" }));
            }
        }
        catch (error)
        {
            console.error("⚠️ Error parsing WebSocket message:", error);
            ws.send(JSON.stringify({ type: "error", message: "Invalid message format" }));
        }
    }

    ProcessClientCommand(ws, data)
    {
        switch (data.type)
        {
            case "pause_system":
                if (data.args === "resume")
                {
                    this.HandleResumeMotion();
                }
                else if (data.args && data.args.duration)
                {
                    this.HandlePauseMotion(data.args.duration);
                }
                break;
            case "snapshot":
                this.StartLiveFeed();
                break;
            default:
                this.SendToOpenGuard(data);
        }
    }

    HandleLogin(ws, data)
    {
        if (Auth.AuthenticateUser(data.username, data.password))
        {
            console.log("🔓 Authentication successful.");
            this.clients.set(ws, { authenticated: true });
            ws.send(JSON.stringify({ type: "login_success" }));
        }
        else
        {
            console.log("❌ Authentication failed.");
            ws.send(JSON.stringify({ type: "login_failed", message: "Invalid credentials" }));
        }
    }

    HandlePauseMotion(duration)
    {
        if (!duration || duration <= 0) return;

        this.is_paused = true;
        this.resume_time = Date.now() + duration * 1000;

        console.log(`⏸ Motion detection paused for ${duration / 60} minutes.`);
        this.SendToOpenGuard({ type: "pause_system", args: { duration } });

        if (this.resume_timer) clearTimeout(this.resume_timer);
        this.resume_timer = setTimeout(() => this.HandleResumeMotion(), duration * 1000);

        this.BroadcastToClients({ type: "pause_status", is_paused: true, resume_time: this.resume_time });
    }

    HandleResumeMotion()
    {
        console.log("▶️ Resuming motion detection.");
        this.is_paused = false;
        this.resume_time = null;
        this.SendToOpenGuard({ type: "pause_system", args: "resume"});

        if (this.resume_timer)
        {
            clearTimeout(this.resume_timer);
            this.resume_timer = null;
        }

        this.BroadcastToClients({ type: "pause_status", is_paused: false, resume_time: null });
    }

    StartLiveFeed()
    {
        console.log("📡 Live feed requested.");
        this.active_viewers = this.CountAuthenticatedViewers();

        if (this.active_viewers > 0)
        {
            console.log("📡 Requesting OpenGuard to start sending frames.");
            this.SendToOpenGuard({ type: "snapshot", args: { status: "start" } });
        }
    }

    UpdateViewers()
    {
        this.active_viewers = this.CountAuthenticatedViewers();

        if (this.active_viewers === 0)
        {
            console.log("🚫 No viewers, stopping frame requests.");
            this.SendToOpenGuard({ type: "snapshot", args: { status: "stop" } });
        }
    }

    CountAuthenticatedViewers()
    {
        return Array.from(this.clients.values()).filter(client => client.authenticated).length;
    }

    ConnectToOpenGuard()
    {
        console.log(`🔄 Connecting to OpenGuard WebSocket at ws://localhost:${config.ws.openguard_port}`);

        if (this.openguard_ws)
        {
            this.openguard_ws.close();
            this.openguard_ws = null;
        }

        this.openguard_ws = new WebSocket(`ws://localhost:${config.ws.openguard_port}`);

        this.openguard_ws.on("open", () =>
        {
            console.log("✅ Connected to OpenGuard WebSocket");
            this.is_openguard_connected = true;
            this.BroadcastToClients({ type: "openguard_status", status: "Connected" });
            this.SendToOpenGuard({ type: "auth", args: { username: config.ws.username, password: config.ws.password } });
        });

        this.openguard_ws.on("message", (message) =>
        {
            const data = JSON.parse(message);
            if (data.type === "stream")
            {
                this.BroadcastToClients({ type: "frame", image: data.image });
            }
        });

        this.openguard_ws.on("close", () =>
        {
            console.log("❌ OpenGuard disconnected. Reconnecting in 5s...");
            this.is_openguard_connected = false;
            this.BroadcastToClients({ type: "openguard_status", status: "Disconnected" });

            setTimeout(() => this.ConnectToOpenGuard(), this.reconnect_interval);
        });

        this.openguard_ws.on("error", () =>
        {
            console.error("⚠️ OpenGuard WebSocket error.");
        });
    }

    SendToOpenGuard(data)
    {
        if (this.openguard_ws && this.openguard_ws.readyState === WebSocket.OPEN)
        {
            this.openguard_ws.send(JSON.stringify(data));
        }
        else
        {
            console.error("⚠️ Cannot send command, OpenGuard WebSocket is not connected.");
        }
    }

    BroadcastToClients(data)
    {
        this.clients.forEach((client, ws) =>
        {
            if (client.authenticated && ws.readyState === WebSocket.OPEN)
            {
                ws.send(JSON.stringify(data));
            }
        });
    }
}

const ws_handler = new WebSocketHandler();
export default ws_handler;
