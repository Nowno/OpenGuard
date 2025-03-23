import express from "express";
import cors from "cors";
import fs from "fs";
import path from "path";

import ws_handler from "./core/ws_handler.js";
import {config, video_dir} from "./core/config_manager.js";
import Auth from "./auth.js";
import { Log } from "./core/logger.js";

const app = express();
const port = config.web.port || 3000;

app.use(cors());
app.use(express.json());

/// server video directory statically.
app.use("/videos", express.static(video_dir));

/// Simple login API to authenticate the user.
app.post("/api/login", (req, res) =>
{
    try
    {
        const { username, password } = req.body;

        if (!username || !password)
        {
            console.warn("⚠️ Missing username or password in login request.");
            return res.status(400).json({ success: false, message: "Username and password are required." });
        }

        if (Auth.AuthenticateUser(username, password))
        {
            Log("🔑", "Auth", `Login successful for user: ${username}`);
            return res.json({ success: true });
        }

        Log("🔑", "Auth", `Login failed for user: ${username}`);

        return res.status(401).json({ success: false, message: "Invalid credentials" });

    }
    catch (error)
    {
        Log("❌", "Auth", `API error: ${error}`);
        return res.status(500).json({ success: false, message: "Server error" });
    }
});

/// Handle the video viewing with requests rather than websocket to allow for scrubbing, besides it's a well implemented standard.
/// we'll have to check if the user is authenticated here as well, but for now we'll just serve the video.
app.get("/videos/:filename", (req, res) =>
{
    const file_path = path.join(video_dir, req.params.filename);

    /// Make sure the file exists before serving
    if (fs.existsSync(filePath))
    {
        res.setHeader("Content-Type", "video/mp4");

        /// Curiously enough I had to set these for the video to play on my iPhone.
        res.setHeader("Accept-Ranges", "bytes");
        res.setHeader("Access-Control-Allow-Origin", "*");
        fs.createReadStream(file_path).pipe(res);
    }
    else
    {
        res.status(404).json({ error: "File not found" });
    }
});

/// API to get the list of videos available.
app.get("/api/videos", (req, res) =>
{
    fs.readdir(video_dir, (err, files) =>
    {
        if (err)
            return res.status(500).json({ success: false, message: "Could not retrieve videos" });

        const videoFiles = files.filter(file => file.endsWith(".mp4"));
        res.json({ success: true, videos: videoFiles });
    });
});



/// Start the WebSocket server.
ws_handler.StartServer();

/// Start listening
app.listen(port, () =>
{
    console.log(`🚀 Backend HTTP Server running on port ${port}`);

});
