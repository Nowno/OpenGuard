import React, { useState, useEffect } from "react";
import websocketHandler from "./core/ws_handler";
import LiveFeed from "./front/live_feed";
import "./config/src/styles/index.css";

function App() {
    const [connectionStatus, setConnectionStatus] = useState("Connecting...");
    const [logs, setLogs] = useState([]);

    useEffect(() => {
        websocketHandler.InitHandler(setConnectionStatus, (data) => {
            if (data.type === "log")
            {
                setLogs((prevLogs) => [...prevLogs, data.message]);
            }
        });

        return () => {
            websocketHandler.CloseWebSocket();
        };
    }, []);

    return (
        <div className="flex flex-col items-center p-4 min-h-screen bg-gray-900 text-white">
            {/* Connection Status */}
            <div className={`mb-4 px-4 py-2 rounded ${connectionStatus === "Connected" ? "bg-green-500" : "bg-red-500"}`}>
                {connectionStatus}
            </div>

            <h1 className="text-4xl font-bold text-blue-500">OpenGuard Web Panel</h1>

            {/* Buttons */}
            <div className="flex gap-4 mt-4">
                <button onClick={() => websocketHandler.sendCommand("pause")} className="px-4 py-2 bg-blue-500 rounded hover:bg-blue-600">
                    Pause
                </button>
                <button onClick={() => websocketHandler.sendCommand("restart")} className="px-4 py-2 bg-red-500 rounded hover:bg-red-600">
                    Restart
                </button>
            </div>

            {}
            <LiveFeed />

            {}
            <div className="mt-6 bg-gray-800 p-4 w-full max-w-lg rounded">
                <h2 className="text-lg font-semibold">Logs</h2>
                <div className="overflow-auto h-40">
                    {logs.map((log, index) => (
                        <p key={index} className="text-sm">{log}</p>
                    ))}
                </div>
            </div>
        </div>
    );
}

export default App;
