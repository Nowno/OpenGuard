import React, { useState, useEffect } from "react";
import websocket_handler from "./ws_handler";
import LiveFeed from "./live_feed";
import "../config/src/styles/index.css";
import ConfigEditor from "./config_editor";
import LogViewer from "./log_viewer";
//todo cleanup
function App()
{
    const [backend_status, setBackendStatus] = useState("Connecting...");
    const [openguard_status, setOpenGuardStatus] = useState("Checking...");
    const [is_authenticated, setAuthenticated] = useState(false);
    const [username, setUsername] = useState("");
    const [password, setPassword] = useState("");
    const [pause_duration, setPauseDuration] = useState(300);
    const [custom_duration, setCustomDuration] = useState("");
    const [is_paused, setIsPaused] = useState(false);
    const [remaining_time, setRemainingTime] = useState(0);
    const [show_pause_modal, setShowPauseModal] = useState(false);
    const [show_config_editor, setShowConfigEditor] = useState(false);
    const [show_logs, setShowLogs] = useState(false);

    useEffect(() =>
    {
        console.log("🔄 Initializing WebSocket connection...");
        websocket_handler.InitHandler(setBackendStatus, (data) =>
        {
            if (data.type === "login_success")
            {
                console.log("🔓 WebSocket Login successful!");
                setAuthenticated(true);
                setBackendStatus("Connected");
            }
            if (data.type === "openguard_status")
            {
                setOpenGuardStatus(data.status);
            }
            if (data.type === "error" && data.message === "Unauthorized")
            {
                console.warn("⚠️ Unauthorized! Logging out...");
                setAuthenticated(false);
            }
            if (data.type === "pause_status")
            {
                console.log(`⏸ Motion detection paused: ${data.is_paused}`);
                setIsPaused(data.is_paused);
                if (data.is_paused && data.resume_time)
                {
                    const time_left = Math.max(0, Math.floor((data.resume_time - Date.now()) / 1000));
                    setRemainingTime(time_left);
                }
            }
        });

        return () =>
        {
            websocket_handler.CloseConnection();
        };
    }, []);

    useEffect(() =>
    {
        if (is_paused && remaining_time > 0)
        {
            const timer = setInterval(() =>
            {
                setRemainingTime((prev) => (prev > 1 ? prev - 1 : 0));
            }, 1000);
            return () => clearInterval(timer);
        }
        else if (remaining_time === 0 && is_paused)
        {
            setIsPaused(false);
        }
    }, [is_paused, remaining_time]);

    async function HandleLogin()
    {
        try
        {
            const backend_ip = window.location.hostname;
            const response = await fetch(`http://${backend_ip}:3000/api/login`,
                {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify({ username, password }),
                });

            const data = await response.json();
            if (data.success)
            {
                console.log("🔓 API Login successful!");
                setAuthenticated(true);

                await new Promise((resolve) =>
                {
                    const check_connection = setInterval(() =>
                    {
                        if (websocket_handler.websocket && websocket_handler.websocket.readyState === WebSocket.OPEN)
                        {
                            clearInterval(check_connection);
                            resolve();
                        }
                    }, 500);
                });

                websocket_handler.SendLogin(username, password);
            }
            else
            {
                alert("❌ Login failed: " + data.message);
            }
        }
        catch (error)
        {
            alert("⚠️ Error logging in: " + error);
        }
    }

    function HandlePause()
    {
        const duration = custom_duration ? parseInt(custom_duration) * 60 : parseInt(pause_duration);
        if (!duration || duration <= 0)
        {
            alert("⚠️ Please enter a valid pause duration!");
            return;
        }
        websocket_handler.SendCommand("pause_system", { args: { "duration": duration } });
        setIsPaused(true);
        setRemainingTime(duration);
        setShowPauseModal(false);
    }

    function HandleResume()
    {
        websocket_handler.SendCommand("pause_system", { args: { "duration": "resume" } });
        setIsPaused(false);
        setRemainingTime(0);
    }

    if (!is_authenticated)
    {
        return (
            <div className="flex flex-col items-center p-4 min-h-screen bg-gray-900 text-white">
                <h1 className="text-3xl font-bold">OpenGuard Login</h1>
                <input
                    className="mt-4 p-2 border rounded bg-gray-800 text-white"
                    type="text"
                    placeholder="Username"
                    value={username}
                    onChange={(e) => setUsername(e.target.value)}
                />
                <input
                    className="mt-2 p-2 border rounded bg-gray-800 text-white"
                    type="password"
                    placeholder="Password"
                    value={password}
                    onChange={(e) => setPassword(e.target.value)}
                />
                <button onClick={HandleLogin} className="mt-4 px-4 py-2 bg-blue-500 rounded hover:bg-blue-600">
                    Login
                </button>
            </div>
        );
    }

    return (
        <div className="flex flex-col items-center p-4 min-h-screen bg-gray-900 text-white">
            {/* Status Buttons */}
            <div className="mb-4 flex gap-4">
                <div className={`px-4 py-2 rounded ${backend_status === "Connected" ? "bg-green-500" : "bg-red-500"}`}>
                    Backend: {backend_status}
                </div>
                <div
                    className={`px-4 py-2 rounded ${backend_status === "Connected" && openguard_status === "Connected" ? "bg-green-500" : "bg-red-500"}`}>
                    OpenGuard: {backend_status === "Connected" ? openguard_status : "Disconnected"}
                </div>
            </div>

            {/* Live Feed */}
            <div
                className="border border-gray-700 w-full max-w-lg aspect-video flex items-center justify-center bg-black">
                <LiveFeed/>
            </div>

            {/* Buttons */}
            <div className="flex gap-4 mt-4">
                <button
                    onClick={() => setShowConfigEditor(true)}
                    className="px-4 py-2 bg-blue-500 rounded hover:bg-blue-600"
                >
                    Edit Config ⚙️
                </button>
            </div>

            {/* Config Editor Modal */}
            <ConfigEditor isOpen={show_config_editor} onClose={() => setShowConfigEditor(false)}/>

            {/* Pause System*/}
            {is_paused ? (
                <div className="mt-4 text-center">
                    <div className="text-yellow-400 text-lg mb-2">
                        ⏸️ Paused for {Math.floor(remaining_time / 60)} min {remaining_time % 60} sec
                    </div>
                    <button onClick={HandleResume} className="px-4 py-2 bg-green-500 rounded hover:bg-green-600">
                        Resume Motion Detection ▶️
                    </button>
                </div>
            ) : (
                <button onClick={() => setShowPauseModal(true)}
                        className="mt-4 px-4 py-2 bg-yellow-500 rounded hover:bg-yellow-600">
                    Pause Motion Detection ⏸️
                </button>
            )}
            <div className="flex flex-col items-center p-4 min-h-screen bg-gray-900 text-white">

                {/* Open Logs Button */}
                <button onClick={() => setShowLogs(true)} className="px-4 py-2 bg-gray-700 rounded hover:bg-gray-800">
                    View Logs 📜
                </button>

                {/* Log Viewer Modal */}
                <LogViewer isOpen={show_logs} onClose={() => setShowLogs(false)}/>
            </div>
            {/* Pause Modal */}
            {show_pause_modal && (
                <div className="fixed inset-0 flex items-center justify-center bg-black bg-opacity-50">
                    <div className="bg-gray-800 p-4 rounded-lg">
                        <h2 className="text-lg font-semibold">Select Pause Duration</h2>
                        <select className="p-2 bg-gray-800 text-white rounded w-full my-2" value={pause_duration}
                                onChange={(e) => setPauseDuration(e.target.value)}>
                            <option value="300">5 minutes</option>
                            <option value="600">10 minutes</option>
                            <option value="1800">30 minutes</option>
                            <option value="custom">Custom</option>
                        </select>

                        {pause_duration === "custom" && (
                            <input type="number" className="p-2 bg-gray-800 text-white rounded w-full mb-2"
                                   placeholder="Enter minutes" value={custom_duration}
                                   onChange={(e) => setCustomDuration(e.target.value)}/>
                        )}

                        <button onClick={HandlePause}
                                className="px-4 py-2 bg-yellow-500 rounded hover:bg-yellow-600 w-full">
                            Confirm Pause
                        </button>
                    </div>
                </div>
            )}
        </div>
    );
}

export default App;

//bug, logs in twice?
