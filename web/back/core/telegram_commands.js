import fetch from "node-fetch";
import fs from "fs";
import FormData from "form-data";


import ws_handler from "./ws_handler.js";
import { config, SaveConfig } from "./config_manager.js";
import { Log } from "./logger.js";
import { GetPauseState, HandlePauseMotion, HandleResumeMotion } from "./pause_manager.js";

let last_update_id = 0;
let requested_vids = false;


async function PollTelegram()
{

    /// If we didn't configure Telegram, just wait 10 seconds and try again
    /// by that time hopefully the C++ would have sent the config.
    if (!config.telegram)
    {
        Log("⚠️", "Warning", "Telegram bot not configured");
        setTimeout(PollTelegram, 10000);
        return;
    }

    try
    {
        const request_res = await fetch(`https://api.telegram.org/bot${config.telegram.bot_token}/getUpdates?timeout=30&offset=${last_update_id + 1}`);
        const data = await request_res.json();

        /// Request the updates from Telegram and process them, if the request was successful
        if (data.ok)
        {
            /// Go through the last updates
            for (const update of data.result)
            {
                last_update_id = update.update_id;

                const msg = update.message;
                let message = msg.text.trim();

                /// If the message is missing or not a string, skip it
                if (!msg || typeof msg.text !== "string")
                    continue;

                /// If the chat ID is different from the one we configured, skip it
                if (msg.chat.id !== config.telegram.chat_id)
                    continue;


                /// If the message is not a command, skip it
                if (message[0] !== "/")
                {
                    HandleCommand("invalid", []);
                    continue;
                }

                /// Otherwise, remove the slash
                message = message.slice(1);

                /// And split the message into command and arguments
                let split_message = message.split(" ");
                let command = split_message[0].toLowerCase();
                let args = split_message.slice(1);

                /// Pass the command to the handler
                HandleCommand(command, args);
            }
        }
    }
    catch (err)
    {
        Log("❌", "Telegram", `Error polling Telegram: ${err}`);
    }

    /// Poll every 600ms, fast enough to feel responsive.
    setTimeout(PollTelegram, 600);
}

/// Handle Telegram commands
function HandleCommand(cmd, args)
{
    if (cmd === "pause")
    {
        const duration = parseInt(args[0]);

        /// make sure the duration is a number and greater than 0
        if (args.length === 0 || isNaN(duration) || duration <= 0)
        {
            SendTelegramMessage("🤖: ❌ Incorrect duration. Try /pause [duration]");
            return;
        }

        /// Simply forward it to the pause manager
        HandlePauseMotion(ws_handler, duration * 60);
        SendTelegramMessage(`🤖: ⏸ System paused for ${duration} minutes.`);
    }
    else if (cmd === "resume")
    {
        /// Same thing here
        HandleResumeMotion(ws_handler);
        SendTelegramMessage("🤖: ✅ System resumed.");
    }
    else if (cmd === "screenshot")
    {
        /// The C++ already handles this correctly, just forward the request
        let og_request = { type: "snapshot", args: { status: "screenshot" } };
        ws_handler.SendToOpenGuard(og_request);

        SendTelegramMessage("🤖: 📸 Screenshot requested.");
    }
    else if (cmd === "setconfig")
    {
        args = args.join(" ");
        let [key, value] = args.split(" ");

        /// sanity check
        if (!key || !value)
        {
            SendTelegramMessage("🤖: ❌ Invalid arguments. Try /setconfig <key> <value>");
            return;
        }

        /// if the key exists, allow modification
        if (ws_handler.config[key] !== undefined)
        {
            /// Convert the value to the correct type
            if (typeof ws_handler.config[key] === "number")
                value = parseFloat(value);
            else if (typeof ws_handler.config[key] === "boolean")
                value = value === "true";
            else if (Array.isArray(ws_handler.config[key]))
                value = value.split(",").map(v => v.trim());
            else if (typeof ws_handler.config[key] === "string")
                value = value.toString();

            ws_handler.config[key] = value;

            /// if its the username or password, update it for us.
            if (key === "server_password" || key === "server_username")
                SaveConfig();

            /// Send the new config to the C++
            websocket_handler.SendCommand("set_config", {args: {config: JSON.stringify(ws_handler.config, null, 4)}});

            SendTelegramMessage(`🤖: ✅ Config updated.`);
        }
        else
        {
            SendTelegramMessage("🤖: ❌ Invalid key.");
        }
    }
    else if (cmd === "getconfig")
    {
        /// Send the current config to the user
        let config_str = JSON.stringify(ws_handler.config, null, 4);
        SendTelegramMessage(`🤖: 📄 Current config:\n\`\`\`${config_str}\`\`\``);
    }
    else if (cmd === "restart")
    {
        /// Restart the system
        SendTelegramMessage("🤖: 🔄 Restarting system.");
        ws_handler.SendToOpenGuard({type: "restart",  args: {} });
    }
    else if (cmd === "clearvideos")
    {
        SendTelegramMessage("🤖: 🗑 Clearing videos.");
        /// First get the list of videos
        ws_handler.SendToOpenGuard({type: "get_videos", args: { type: "list" } });
        requested_vids = true; /// Signify that we requested the videos, the ws_handler will interpret this as a command to clear them.
    }
    else if (cmd === "help")
    {
        SendTelegramMessage([
            "🤖: 📖 Commands:",
            "/pause [duration] - Pause system, in minutes.",
            "/resume - Resume system",
            "/screenshot - Take a screenshot of the feed",
            "/setconfig <key> <value> - Update config key",
            "/getconfig - Show current config",
            "/restart - Restart the system",
            "/clearvideos - Clear all videos from the device running OpenGuard)",
            "/help - This list"
        ].join("\n"));
    }
    else
    {
        SendTelegramMessage("🤖: ❓ Unknown command. Try /help");
    }
}

function SendTelegramMessage(text)
{
    if (!config.telegram || !config.telegram.chat_id)
        return;

    let chat_id = config.telegram.chat_id;

    if (!chat_id)
        return;

    /// simply following telegram api documentation
    return fetch(`https://api.telegram.org/bot${config.telegram.bot_token}/sendMessage`,
    {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ chat_id, text })
    });
}


function SendTelegramImage(image_path)
{
    let chat_id = config.telegram.chat_id;

    if (!chat_id)
        return;

    /// only can send images as form data or url, opted for form data
    let form = new FormData();
    form.append("chat_id", chat_id);
    form.append("photo", fs.createReadStream(image_path));

    return fetch(`https://api.telegram.org/bot${config.telegram.bot_token}/sendPhoto`, {
        method: "POST",
        body: form
    });
}

function SetRequestedVids(val)
{
    requested_vids = val;
}

function GetRequestedVids()
{
    return requested_vids;
}

/// Notify the user that the bot is up and running
SendTelegramMessage("🤖: Telegram bot up and running.");


export { PollTelegram, SendTelegramImage, SetRequestedVids, GetRequestedVids };