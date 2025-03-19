import fs from "fs";
import path from "path";

const configPath = path.resolve("./config.json");

function LoadConfig()
{
    try
    {
        const rawData = fs.readFileSync(configPath);
        return JSON.parse(rawData);
    }
    catch (error)
    {
        console.error("Error loading config:", error);
        return null;
    }
}

const config = LoadConfig();
export default config;
