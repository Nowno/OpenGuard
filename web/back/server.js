import express from "express";
import cors from "cors";
import ws_handler from "./ws_handler.js";
import config from "./config_manager.js";
import Auth from "./auth.js";

const app = express();
const port = config.web.port || 3000;

app.use(cors());
app.use(express.json());

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
            console.log(`✅ Login successful for user: ${username}`);
            return res.json({ success: true });
        }

        console.warn(`❌ Invalid login attempt for user: ${username}`);
        return res.status(401).json({ success: false, message: "Invalid credentials" });

    }
    catch (error)
    {
        console.error("❌ Error in login API:", error);
        return res.status(500).json({ success: false, message: "Server error" });
    }
});

try
{
    ws_handler.StartServer();
}
catch (error)
{
    console.error("❌ Failed to start WebSocket server:", error);
}

app.listen(port, () =>
{
    console.log(`🚀 Backend HTTP Server running on port ${port}`);

});
