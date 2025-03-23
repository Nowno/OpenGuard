import React, { useState } from "react";
import websocket_handler from "../ws_handler";

function PauseControl({ is_open, onClose, setIsPaused, setRemainingTime })
{
    const [pause_duration, set_pause_duration] = useState("300"); // default: 5 mins
    const [custom_duration, set_custom_duration] = useState("");

    function HandlePause()
    {
        let duration_in_seconds;

        /// If the user selected a custom duration, we parse it
        if (pause_duration === "custom")
        {
            const custom_minutes = parseInt(custom_duration);

            if (!custom_minutes || isNaN(custom_minutes) || custom_minutes <= 0)
            {
                alert("⚠️ Please enter a valid custom duration.");
                return;
            }

            /// convert minutes to seconds
            duration_in_seconds = custom_minutes * 60;
        }
        else
        {
            /// if not custom, we just parse the selected value
            duration_in_seconds = parseInt(pause_duration);
        }

        /// Send the pause command to the backend. Actually because of the way C++ is implemented, we could just send a timestamp.
        /// that's due to a late maybe design change.
        websocket_handler.SendCommand("pause_system", {
            args: { duration: duration_in_seconds }
        });

        onClose();
    }

    if (!is_open)
    {
        return null;
    }

    return (
        <div className="fixed inset-0 bg-black/30 backdrop-blur-lg flex items-center justify-center p-4 z-50">
            <div className="bg-gray-900 p-6 rounded-lg w-full max-w-sm shadow-lg">

                <h2 className="text-lg font-semibold text-white mb-4">
                    ⏸️ Select Pause Duration
                </h2>

                <select
                    className="p-2 bg-gray-800 text-white rounded w-full mb-3"
                    value={pause_duration}
                    onChange={(e) => set_pause_duration(e.target.value)}
                >
                    <option value="300">5 minutes</option>
                    <option value="600">10 minutes</option>
                    <option value="1800">30 minutes</option>
                    <option value="3600">60 minutes</option>
                    <option value="custom">Custom</option>
                </select>

                {pause_duration === "custom" && (
                    <input
                        type="number"
                        className="p-2 bg-gray-800 text-white rounded w-full mb-3"
                        placeholder="Enter minutes"
                        value={custom_duration}
                        onChange={(e) => set_custom_duration(e.target.value)}
                    />
                )}

                <div className="flex gap-2">
                    <button
                        onClick={HandlePause}
                        className="flex-1 px-4 py-2 bg-yellow-500 text-black rounded hover:bg-yellow-600"
                    >
                        Confirm Pause
                    </button>

                    <button
                        onClick={onClose}
                        className="flex-1 px-4 py-2 bg-gray-600 text-white rounded hover:bg-gray-700"
                    >
                        Cancel
                    </button>
                </div>
            </div>
        </div>
    );
}

export default PauseControl;
