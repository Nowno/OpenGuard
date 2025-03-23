import React, { useState, useEffect } from "react";
import websocket_handler from "../ws_handler";

const DAYS = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];

function ScheduleManager({ is_open, onClose })
{
    const [schedule, set_schedule] = useState({});
    const [selected_day, set_selected_day] = useState("Monday");
    const [new_start, set_new_start] = useState(0);
    const [new_end, set_new_end] = useState(0);

    useEffect(() =>
    {
        if (!is_open)
        {
            return;
        }

        /// Subscribe to schedule config messages and ask for a schedule if one exists
        websocket_handler.Subscribe("message", HandleMessage);
        websocket_handler.SendCommand("get_schedule");

        return () =>
        {
            /// Unsubscribe from the events when we're done
            websocket_handler.Unsubscribe("message", HandleMessage);
        };
    }, [is_open]);


    function HandleMessage(message)
    {
        /// If we receive a schedule config, update the schedule
        if (message.type === "schedule_config")
        {
            set_schedule(message.schedule);
        }
    }

    /// Adds a new time block to the selected day
    function AddScheduleBlock()
    {
        /// Parse the start and end times
        const start = parseInt(new_start);
        const end = parseInt(new_end);

        /// Validate the times, 24 hour format
        if (isNaN(start) || isNaN(end) ||
            start < 0 || start >= 24 ||
            end <= start || end > 24)
        {
            alert("Invalid time range");
            return;
        }

        const updated = { ...schedule };

        /// If the day doesn't have any blocks yet, create an empty array
        if (!updated[selected_day])
        {
            updated[selected_day] = [];
        }

        /// Add the new block
        updated[selected_day].push({ start, end });
        set_schedule(updated);

        /// Send the updated schedule to the backend
        websocket_handler.SendCommand("save_schedule", { schedule: updated });
    }

    /// Removes a block from the selected day
    function RemoveBlock(day, index)
    {
        const updated = { ...schedule };

        /// Filter out the block at the given index
        updated[day] = updated[day].filter((_, i) => i !== index);

        set_schedule(updated);

        /// Update the backend
        websocket_handler.SendCommand("save_schedule", { schedule: updated });
    }

    if (!is_open)
    {
        return null;
    }

    return (
        <div className="fixed inset-0 bg-black/30 backdrop-blur-lg flex items-center justify-center p-4 z-50">
            <div className="bg-gray-900 p-6 rounded-lg shadow-lg max-w-xl w-full">
                <h2 className="text-lg font-bold mb-4">
                    📅 Pause Schedule Settings (Per Day)
                </h2>

                {/* Day Selector */}
                <div className="mb-4">
                    <label className="block mb-1">Select Day</label>
                    <select
                        value={selected_day}
                        onChange={(e) => set_selected_day(e.target.value)}
                        className="p-2 bg-gray-700 rounded w-full text-white"
                    >
                        {DAYS.map((day) => (
                            <option key={day} value={day}>{day}</option>
                        ))}
                    </select>
                </div>

                <div className="space-y-4 mb-4">
                    {DAYS.map((day) => (
                        <div key={day}>
                            <h3 className="text-sm font-semibold mb-1">{day}</h3>
                            {(schedule[day] || []).map((block, idx) => (
                                <div
                                    key={idx}
                                    className="flex justify-between items-center bg-gray-800 p-2 rounded mb-1"
                                >
                                    <span>{block.start}:00 → {block.end}:00</span>
                                    <button
                                        onClick={() => RemoveBlock(day, idx)}
                                        className="text-red-400 hover:text-red-600"
                                    >
                                        ✖
                                    </button>
                                </div>
                            ))}
                        </div>
                    ))}
                </div>

                <div className="mt-4 flex items-center gap-2">
                    <input
                        type="number"
                        min="0"
                        max="23"
                        value={new_start}
                        onChange={(e) => set_new_start(e.target.value)}
                        className="bg-gray-700 p-2 rounded text-white w-16"
                        placeholder="Start"
                    />
                    <span className="text-white">to</span>
                    <input
                        type="number"
                        min="1"
                        max="24"
                        value={new_end}
                        onChange={(e) => set_new_end(e.target.value)}
                        className="bg-gray-700 p-2 rounded text-white w-16"
                        placeholder="End"
                    />
                    <button
                        onClick={AddScheduleBlock}
                        className="bg-blue-500 hover:bg-blue-600 px-3 py-2 rounded"
                    >
                        ➕ Add
                    </button>
                </div>

                <button
                    onClick={onClose}
                    className="mt-6 w-full text-sm bg-red-500 hover:bg-red-600 px-4 py-2 rounded"
                >
                    Close
                </button>
            </div>
        </div>
    );
}

export default ScheduleManager;
