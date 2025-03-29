import { WebSocket, WebSocketServer } from "ws";
import fs from "fs";
import Auth from "../auth.js";

import { config, SaveConfig, video_dir } from "./config_manager.js";
import { GetPauseState, HandlePauseMotion, HandleResumeMotion } from "./pause_manager.js";
import { LoadSchedule, SaveSchedule, ApplySchedule } from "./scheduler.js";
import { Log } from "./logger.js";
import { SendTelegramImage, requested_vids} from "./telegram_commands.js";

/**
    Please don't judge the following code, I focused all my efforts on the C++ side, and I had very little time
    for this part, and while I should have built it properly from the start as I did in C++, I really needed to get
    things working, and I'm far more familiar with C++ than js. Will definitely refactor in the future.
 */

class WebSocketHandler
{
    constructor()
    {
        this.clients = new Map();
        this.openguard_ws = null;
        this.reconnect_interval = 5000;
        this.is_openguard_connected = false;

        this.config = 0;

        this.downloading_video = false;
        this.video_buffer = [];

        this.schedule = LoadSchedule();
    }

    StartServer()
    {
        this.server = new WebSocketServer({ port: config.ws.port });

        Log("🌐", "INFO", `Started ws server on: ws://localhost:${config.ws.port}`);

        this.server.on("connection", (ws) =>
        {
            Log("🔗", "INFO", "Incoming connection. (FE)");

            /// Add the client to the map, and ensure it sets him as unauthenticated
            this.clients.set(ws, { authenticated: false });

            /// Send the status the client needs to be aware of (pause/openguard status) as they are reflected on the UI
            this.SendInitialStatus(ws);

            /// Bind our handlers.
            ws.on("message", (message) => this.HandleMessage(ws, message));
            ws.on("close", () => this.RemoveClient(ws));
            ws.on("error", () => this.RemoveClient(ws));
        });

        /// Initiate connection to our C++ back end
        this.ConnectToOpenGuard();

        /// This is a bit yucky
        ApplySchedule(() => GetPauseState(), (duration) => HandlePauseMotion(this, duration), () => HandleResumeMotion(this));

    }

    SendInitialStatus(ws)
    {
        /// Retrieve pause status incase openguard disconnected abruptly
        const { is_paused, resume_time } = GetPauseState();

        ws.send(JSON.stringify({ type: "openguard_status", status: this.is_openguard_connected ? "Connected" : "Disconnected" }));
        ws.send(JSON.stringify({ type: "pause_status", is_paused, resume_time }));
    }

    /**
     * Handle client disconnection
     */
    RemoveClient(ws)
    {
        /// Delete client and update viewer count to not send frames in vein from C++
        this.clients.delete(ws);
        this.UpdateViewers();

        /// Should probably move this to a function later
        if (this.clients.size === 0)
        {
            Log("🕒", "INFO", "No active clients. Cleaning up cache in ");

            setTimeout(() =>
            {
                if (this.clients.size === 0)
                {
                    /// https://stackoverflow.com/questions/5315138/node-js-remove-file
                    fs.readdir(video_dir, (err, files) =>
                    {
                        if (!err)
                        {
                            files.forEach(file =>
                            {
                                fs.unlinkSync(`${video_dir}/${file}`);
                            });
                        }
                    });
                }
            }, 5 * 60 * 1000); /// 5 times 60 seconds times 1000ms
        }
    }

    /**
     * Message handler, again I'm not proud of this code, please use the C++ side as a reference for my grade
     */
    HandleMessage(ws, message)
    {
        try
        {
            const data = JSON.parse(message);

            /// Handle login, don't allow the client to interact if he fails to.
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
            Log("❌", "ERROR", `Invalid message format: ${error}`);
            ws.send(JSON.stringify({ type: "error", message: "Invalid message format" }));
        }
    }

    /**
     * As before, handles client commands.
     */
    ProcessClientCommand(ws, data)
    {
        switch (data.type)
        {
            /// I'm not sure if switch or if-else looks better, but performance is the same afaik so I experimented with switch here
            case "pause_system":
                if (data.args === "resume")
                    HandleResumeMotion(this);
                else if (data.args?.duration)
                    HandlePauseMotion(this, data.args.duration);
                break;

            case "snapshot":
                this.StartLiveFeed();
                break;

            case "get_videos":
                if (data.args?.type === "stream")
                {
                    /// Forward it to Open Guard
                    this.SendToOpenGuard(data);
                    /// So that we know if we should push the video chunks to the buffer
                    this.downloading_video = true;
                }
                else if (data.args?.type === "list" || data.args?.type === "delete")
                {
                    /// Forward it to Open Guard
                    this.SendToOpenGuard(data);
                }
                break;

            case "set_config":
                /// Here we intercept the config change request to update our username and password
                /// in the eventuality they were altered to avoid getting locked out of the C++ backend.
                if (data.args.config?.username && data.args.config?.password)
                {
                    config.ws.username = data.args.config.username;
                    config.ws.password = data.args.config.password;
                    SaveConfig(config);
                }
                /// Forward the command to OpenGuard
                this.SendToOpenGuard(data);
                break;

            case "get_schedule":
                /// Return the schedule to the client when requested
                ws.send(JSON.stringify({ type: "schedule_config", schedule: this.schedule }));
                break;

            case "save_schedule":
                /// Save the schedule sent by the client
                this.schedule = data.schedule;
                SaveSchedule(this.schedule);

                /// Broadcast the new schedule to all clients so that they have accurate information
                this.BroadcastToClients({ type: "schedule_config", schedule: this.schedule });

                /// Apply the schedule to the motion detection. On C++ this is simply handled by a pause_system command, staying dumbed-down and we do the thinking here.
                ApplySchedule(() => GetPauseState(), (duration) => HandlePauseMotion(this, duration), () => HandleResumeMotion(this));
                break;

            default:
                /// Otherwise, forward the command to OpenGuard, not worried as the C++ side handles bad commands gracefully
                this.SendToOpenGuard(data);
                break;
        }
    }

    HandleLogin(ws, data)
    {
        /// Handles user authentication, definitely will re-write this in the future to employ cryptography.
        if (Auth.AuthenticateUser(data.username, data.password))
        {
            Log("🔑", "INFO", "Authentication successful.");
            this.clients.set(ws, { authenticated: true });
            ws.send(JSON.stringify({ type: "login_success" }));
            ws.send(JSON.stringify({ type: "config", config: this.config }));
        }
        else
        {
            Log("🔑", "INFO", "Authentication failed.");
            ws.send(JSON.stringify({ type: "login_failed", message: "Invalid credentials" }));
        }
    }

    StartLiveFeed()
    {
        Log("📡", "INFO", "Live feed requested.");
        this.UpdateViewers();

        if (this.active_viewers > 0)
        {
            Log("📡", "INFO", "Starting live feed.");
            this.SendToOpenGuard({ type: "snapshot", args: { status: "start" } });
        }
    }

    /**
     * Update a "viewer count" to make sure we tell the C++ core to stop streaming if no one is watching.
     */
    UpdateViewers()
    {
        /// Cool js thing, get the amount of authenticated clients
        this.active_viewers = Array.from(this.clients.values()).filter(c => c.authenticated).length;

        if (this.active_viewers === 0)
        {
            Log("📡", "INFO", "No active viewers. Stopping live feed.");

            /// We actually stream to things, logs and the frames, so we request both to stop
            this.SendToOpenGuard({ type: "snapshot", args: { status: "stop" } });
            this.SendToOpenGuard({ type: "get_logs", args: { type: "stream_stop" } });
        }
    }

    /// Connect to the C++ backend
    ConnectToOpenGuard()
    {
        Log("🔗", "INFO", `Connecting to OpenGuard on ws://localhost:${config.ws.openguard_port}`);

        /// Close the connection if we've opened it already exists
        if (this.openguard_ws)
        {
            this.openguard_ws.close();
            this.openguard_ws = null;
        }

        /// Create a new connection to the C++ backend
        this.openguard_ws = new WebSocket(`ws://localhost:${config.ws.openguard_port}`);


        this.openguard_ws.on("open", () =>
        {
            Log("🔗", "INFO", "Connected to OpenGuard.");
            this.is_openguard_connected = true;

            /// Broadcast to our clients that the C++ backend is connected
            this.BroadcastToClients({ type: "openguard_status", status: "Connected" });

            /// Auth to get authorisation to communicate.
            this.SendToOpenGuard({
                type: "auth",
                args: {
                    username: config.ws.username,
                    password: config.ws.password
                }
            });

            /// If we're paused, let the C++ backend know in case it disconnected abruptly or wasn't aware of the pause due to a crash etc.
            const { is_paused, resume_time } = GetPauseState();

            if (is_paused && resume_time)
            {
                const now = Date.now();
                if (resume_time > now)
                {
                    Log("🔁", "INFO", `Re-sending pause to OpenGuard until ${new Date(resume_time).toLocaleString()}`);

                    this.SendToOpenGuard({ type: "pause_system", args: { until: Math.floor(resume_time / 1000) } });

                    /// Send a resume command in C++ although it already handles that for itself, guess this is kind of redundant.
                    setTimeout(() => HandleResumeMotion(this), resume_time - now);
                }
                else
                {
                    HandleResumeMotion(this);
                }
            }
        });

        this.openguard_ws.on("message", (message) =>
        {
            const data = JSON.parse(message);

            switch (data.type)
            {
                /// Some requests are handled a bit differently, between frontend and C++ so we middle man them here.
                case "snapshot_stream":
                    this.BroadcastToClients({ type: "frame", image: data.image });
                    break;

                case "log_dump":
                    this.BroadcastToClients({ type: "log_dump", logs: data.message });
                    break;

                case "log":
                    this.BroadcastToClients({ type: "log", log: data.message });
                    break;

                case "video_list":
                    if (requested_vids)
                    {
                        /// send delete command to C++ backend for each video
                        let video_list = data.videos;
                        video_list.forEach(video =>
                        {
                            this.SendToOpenGuard({ type: "get_videos", args: { type: "delete", video: video } });
                        });
                        requested_vids = false;
                    }
                    this.BroadcastToClients({ type: "video_list", videos: data.videos });
                    break;

                case "screenshot":
                    /// Like video_stream, save the screenshot, but send it to telegram instead.
                    const screenshot_path = video_dir + `/screenshot.jpg`;
                    const screenshot = Buffer.from(data.image, "base64");

                    fs.writeFileSync(screenshot_path, screenshot);

                    console.log("Sending screenshot to telegram");

                    SendTelegramImage(screenshot_path);

                    break;

                case "video_stream":
                    if (data.args.data === "stop")
                    {
                        /// If the C++ is done streaming, save the video, and broadcast the url to the clients.
                        this.downloading_video = false;

                        const video_path = video_dir + `/${data.args.video}`;
                        fs.writeFileSync(video_path, Buffer.concat(this.video_buffer));

                        /// Reset the buffer
                        this.video_buffer = [];

                        /// Send the url to clients.
                        this.BroadcastToClients({ type: "video_url", url: `/videos/${data.args.video}` });
                    }
                    else
                    {
                        /// Only push the video chunks to the buffer if we're expecting them.
                        if (this.downloading_video)
                        {
                            this.video_buffer.push(Buffer.from(data.args.data, "base64"));
                        }
                        else
                        {
                            Log("⚠️", "WARNING", "Received unexpected video chunk, ignoring.");
                        }
                    }
                    break;

                case "get_hooks":
                    /// Okay, this is a cool feature but definitely needs more security, this is essentially a walking RCE. It's really cool, so I'm not sure how to find a middle ground.
                    if (data.args.type === "list")
                    {
                        this.BroadcastToClients({type: "hook_list", hooks: data.args.hooks});
                    }
                    else if (data.args.type === "content")
                    {
                        this.BroadcastToClients({ type: "hook_content", hook: data.args.hook, content: data.args.content });
                    }
                    break;

                case "authenticated":
                    /// When we authenticate, the server answers by sending us the latest config, here we update it for ourselves and send it to the clients.
                    this.config = data.message;

                    /// The config should also contain the telegram chat id/token, extract it and save it for the telegram commands.
                    if (this.config.telegram_bot_token && this.config.telegram_user_id)
                    {
                        if (!config.telegram)
                            config.telegram = {};

                        config.telegram.bot_token = this.config.telegram_bot_token;
                        config.telegram.chat_id = this.config.telegram_user_id;

                        SaveConfig(config);
                    }
                    else
                    {
                        Log("⚠️", "WARNING", "Telegram bot token or user id missing in OpenGuard config.");
                    }

                    this.BroadcastToClients({ type: "config", config: this.config });
                    break;
            }
        });

        /// Reconnect if the connection is lost
        this.openguard_ws.on("close", () =>
        {
            Log("❌", "ERROR", "OpenGuard disconnected. Reconnecting in 5s...");

            this.is_openguard_connected = false;
            this.BroadcastToClients({ type: "openguard_status", status: "Disconnected" });
            setTimeout(() => this.ConnectToOpenGuard(), this.reconnect_interval);
        });

        this.openguard_ws.on("error", () =>
        {
            Log("❌", "ERROR", "OpenGuard connection error.");
        });
    }

    SendToOpenGuard(data)
    {
        /// Make sure we only send if the connection is open
        if (this.openguard_ws?.readyState === WebSocket.OPEN)
        {
            this.openguard_ws.send(JSON.stringify(data));
        }
        else
        {
            Log("❌", "ERROR", "OpenGuard not connected, can't send.");
        }
    }

    BroadcastToClients(data)
    {
        /// Send data to all authenticated clients
        for (const [ws, client] of this.clients.entries())
        {
            if (client.authenticated && ws.readyState === WebSocket.OPEN)
            {
                ws.send(JSON.stringify(data));
            }
        }
    }
}

const ws_handler = new WebSocketHandler();
export default ws_handler;
