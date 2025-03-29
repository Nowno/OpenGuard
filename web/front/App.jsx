import React, { useState, useEffect } from "react";

import websocket_handler from "./ws_handler";
import "./config/src/styles/index.css";

import LiveFeed from "./components/live_feed";
import ConfigEditor from "./components/config_editor";
import LogViewer from "./components/log_viewer";
import SavedClips from "./components/saved_clips";
import HookEditor from "./components/hook_editor";
import PauseControl from "./components/pause_control";
import ScheduleManager from "./components/schedule_manager";

function App()
{
    /// Connection status
    const [backend_status, setBackendStatus] = useState("Connecting...");
    const [openguard_status, setOpenGuardStatus] = useState("Checking...");
    const [is_authenticated, setAuthenticated] = useState(false);

    /// Auth
    const [username, setUsername] = useState("");
    const [password, setPassword] = useState("");

    /// Pause button logic
    const [is_paused, setIsPaused] = useState(false);
    const [remaining_time, setRemainingTime] = useState(0);

    /// Components modals
    const [show_config_editor, setShowConfigEditor] = useState(false);
    const [show_logs, setShowLogs] = useState(false);
    const [show_saved_clips, setShowSavedClips] = useState(false);
    const [show_hook_editor, setShowHookEditor] = useState(false);
    const [showPauseModal, setShowPauseModal] = useState(false);
    const [show_schedule_settings, setShowScheduleSettings] = useState(false);

    /// Websocket init
    useEffect(() =>
    {

        websocket_handler.InitHandler(setBackendStatus, (data) =>
        {
            /// Reflect whether we could auth or not
            if (data.type === "login_success")
            {
                setAuthenticated(true);
                setBackendStatus("Connected");
            }
            else if (data.type === "openguard_status")
            {
                /// Update the status button
                setOpenGuardStatus(data.status);
            }
            else if (data.type === "error" && data.message === "Unauthorized")
            {
                setAuthenticated(false);
            }
            else if (data.type === "pause_status")
            {
                /// Make sure to keep the pause status in sync
                const is_paused = data.isPaused ?? data.is_paused;
                const resume_time = data.resumeTime ?? data.resume_time ?? 0;

                setIsPaused(is_paused);

                if (is_paused && is_paused)
                {
                    const time_left = Math.max(0, Math.floor((resume_time - Date.now()) / 1000));
                    setRemainingTime(time_left);
                }
            }
        });

        return () => websocket_handler.CloseConnection();
    }, []);

    /// Timer for the pause button
    useEffect(() =>
    {
        if (is_paused && remaining_time > 0)
        {
            /// If there is remaining time, keep counting
            const timer = setInterval(() =>
            {
                setRemainingTime((prev) => (prev > 0 ? prev - 1 : 0));
            }, 1000);

            return () => clearInterval(timer);
        }
        else if (remaining_time === 0 && is_paused)
        {
            /// Otherwise, if the time is up, resume the system
            setIsPaused(false);
        }
    }, [is_paused, remaining_time]);

    // Login handler
    async function HandleLogin()
    {
        try
        {
            const backend_ip = window.location.hostname;

            /// Send login request to our backend
            const response = await fetch(`http://${backend_ip}:3000/api/login`,
            {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ username, password })
            });

            /// Await the response and check if we are authenticated
            const data = await response.json();

            if (data.success)
            {
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

                /// We also login to the websocket backend, later should unify this
                websocket_handler.SendLogin(username, password);
            }
        }
        catch (error)
        {
            alert("⚠️ Error logging in: " + error);
        }
    }

    /// Simple so I kept it here, add a confirmation dialog
    function HandleRestart()
    {
        if (window.confirm("Are you sure you want to restart the system?"))
        {
            websocket_handler.SendCommand("restart", { args: {} });
        }
    }

    /// Login screen
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

                <button
                    onClick={HandleLogin}
                    className="mt-4 px-4 py-2 bg-blue-500 rounded hover:bg-blue-600"
                >
                    Login
                </button>
            </div>
        );
    }

    /// Main screen
    return (
        <div className="flex flex-col items-center p-4 min-h-screen bg-gray-900 text-white">

            {/* Connection current status */}
            <div className="mb-4 flex gap-4">
                <div className={`px-4 py-2 rounded ${backend_status === "Connected" ? "bg-green-500" : "bg-red-500"}`}>
                    Backend: {backend_status}
                </div>

                <div className={`px-4 py-2 rounded ${openguard_status === "Connected" ? "bg-green-500" : "bg-red-500"}`}>
                    OpenGuard: {backend_status === "Connected" ? openguard_status : "Disconnected"}
                </div>
            </div>

            {/* Live feed view */}
            <div className="border border-gray-700 w-full max-w-lg aspect-video flex items-center justify-center bg-black">
                <LiveFeed />
            </div>

            {/* Buttons */}
            <div className="flex flex-wrap gap-2 sm:gap-4 mt-4 justify-center w-full max-w-lg">
                <button onClick={() => setShowConfigEditor(true)} className="px-4 py-2 bg-blue-500 rounded hover:bg-blue-600">Edit Config ⚙️</button>
                <button onClick={() => setShowLogs(true)} className="px-4 py-2 bg-gray-700 rounded hover:bg-gray-800">View Logs 📜</button>
                <button onClick={() => setShowSavedClips(true)} className="px-4 py-2 bg-gray-700 rounded hover:bg-gray-800">Saved Clips 🎥</button>
                <button onClick={() => setShowHookEditor(true)} className="px-4 py-2 bg-yellow-500 rounded hover:bg-yellow-600">Edit Hooks 🛠️</button>
                <button onClick={() => setShowScheduleSettings(true)} className="px-4 py-2 bg-purple-500 rounded hover:bg-purple-600">Schedule 🕒</button>
                <button onClick={HandleRestart} className="px-4 py-2 bg-red-500 rounded hover:bg-red-600">Restart 🔄</button>
            </div>

            {/* Modals */}
            <ConfigEditor is_open={show_config_editor} onClose={() => setShowConfigEditor(false)} />
            <LogViewer is_open={show_logs} onClose={() => setShowLogs(false)} />
            <SavedClips is_open={show_saved_clips} onClose={() => setShowSavedClips(false)} />
            <HookEditor is_open={show_hook_editor} onClose={() => setShowHookEditor(false)} />
            <ScheduleManager is_open={show_schedule_settings} onClose={() => setShowScheduleSettings(false)} />

            {/* Pause Panel */}
            {is_paused ? (
                <div className="mt-4 text-center">
                    <div className="text-yellow-400 text-lg mb-2">
                        ⏸️ Paused for {Math.floor(remaining_time / 60)} min {remaining_time % 60} sec
                    </div>

                    <button
                        onClick={() =>
                        {
                            websocket_handler.SendCommand("pause_system", { args: { duration: "resume" } });
                            setIsPaused(false);
                            setRemainingTime(0);
                        }}
                        className="px-4 py-2 bg-green-500 rounded hover:bg-green-600"
                    >
                        ▶️ Resume Motion Detection
                    </button>
                </div>
            ) : (
                <button
                    onClick={() => setShowPauseModal(true)}
                    className="mt-4 px-4 py-2 bg-yellow-500 rounded hover:bg-yellow-600"
                >
                    ⏸️ Pause Motion Detedction
                </button>
            )}

            <PauseControl
                is_open={showPauseModal}
                onClose={() => setShowPauseModal(false)}
                setIsPaused={setIsPaused}
                setRemainingTime={setRemainingTime}
            />
        </div>
    );
}

export default App;
