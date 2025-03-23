import fs from "fs";
import path from "path";
import {fileURLToPath} from "url";
import {Log} from "./logger.js";
const config_path = path.resolve("./config.json");

/// Simple functions to save and load the config file.
function LoadConfig()
{
    try
    {
        const raw_data = fs.readFileSync(config_path);
        return JSON.parse(raw_data);
    }
    catch (error)
    {
        return null;
    }
}

function SaveConfig(config)
{
    try
    {
        fs.writeFileSync(config_path, JSON.stringify(config, null, 4));
        Log("💾", "Config", "Config saved.");
    }
    catch (error)
    {
        Log("❌", "Config", "Error saving config: " + error);
    }
}

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const video_dir = path.join(__dirname, "videos");
if (!fs.existsSync(video_dir)) fs.mkdirSync(video_dir);

const config = LoadConfig();
export { config, SaveConfig, video_dir};
