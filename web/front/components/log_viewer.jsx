import React, { useEffect, useState, useRef } from "react";
import websocket_handler from "../ws_handler";

function LogViewer({ is_open, onClose })
{
    const [logs, set_logs] = useState([]);
    const [filter, set_filter] = useState("all");

    const log_container_ref = useRef(null);

    useEffect(() =>
    {
        /// Handles log messages from the backend
        function HandleLogMessage(message)
        {
            /// if we receive a type log it means its a single log message send by our on_log hook
            if (message.type === "log")
            {
                set_logs((prev_logs) =>
                    [...prev_logs, message.log].slice(-100)
                );
            }
            else if (message.type === "log_dump") /// Otherwise we're getting a dump of logs
            {
                /// We split the logs by new line, and filter out empty lines
                const new_logs = message.logs.split("\n").filter((log) => log.trim() !== "");

                /// We only keep the last 100 logs
                set_logs((prev_logs) =>
                    [...prev_logs, ...new_logs].slice(-100)
                );
            }
        }

        if (is_open)
        {
            /// When opening the log viewer, we subscribe to the log messages and request from the server to start the log stream
            websocket_handler.Subscribe("message", HandleLogMessage);
            websocket_handler.SendCommand("get_logs", { args: { type: "stream_start" } });

            if (logs.length === 0)
            {
                /// If we don't have any logs yet, we request a dump of the logs so that the users knows what happened before
                websocket_handler.SendCommand("get_logs", { args: { type: "dump" } });
            }
        }

        return () =>
        {
            /// Unsuscribe from the log messages when we're done
            /// Should also stop the log stream, but for some reason it wouldn't start again. Anyways, the backend takes care of
            /// cleaning up after us, telling the C++ to stop the log stream when we disconnect
            websocket_handler.Unsubscribe("message", HandleLogMessage);
        };
    }, [is_open]);

    ///Scroll to the bottom of the log container to show the latest logs
    useEffect(() =>
    {
        if (log_container_ref.current)
        {
            log_container_ref.current.scrollTop = log_container_ref.current.scrollHeight;
        }
    }, [logs]);

    /// Clear button functionallity
    function ClearLogs()
    {
        set_logs([]);
    }

    /// Simple filter function to filter the logs
    function FilterLogs()
    {
        if (filter === "all")
        {
            return logs;
        }

        ///lowercase the logs and the filter to make it case insensitive
        return logs.filter((log) => log.toLowerCase().includes(filter));
    }

    if (!is_open)
    {
        return null;
    }

    return (
        <div className="fixed inset-0 bg-black/30 backdrop-blur-lg flex items-center justify-center p-4 z-50">
            <div className="bg-gray-900 p-6 rounded-lg w-full max-w-[90vw] max-h-[90vh] overflow-auto flex flex-col shadow-lg">

                <h2 className="text-lg font-bold text-white mb-4">
                    📜 Log Viewer
                </h2>

                {/* Filters */}
                <div className="flex gap-2 my-2 flex-wrap">
                    <button
                        onClick={() => set_filter("all")}
                        className="px-4 py-2 bg-blue-500 rounded hover:bg-blue-600 text-sm"
                    >
                        All
                    </button>
                    <button
                        onClick={() => set_filter("error")}
                        className="px-4 py-2 bg-red-500 rounded hover:bg-red-600 text-sm"
                    >
                        Errors
                    </button>
                    <button
                        onClick={() => set_filter("info")}
                        className="px-4 py-2 bg-yellow-500 rounded hover:bg-yellow-600 text-sm"
                    >
                        Info
                    </button>
                    <button
                        onClick={ClearLogs}
                        className="ml-auto px-4 py-2 bg-gray-500 rounded hover:bg-gray-600 text-sm"
                    >
                        Clear
                    </button>
                </div>

                <div
                    ref={log_container_ref}
                    className="overflow-auto bg-gray-800/80 backdrop-blur-md text-white p-3 rounded-lg min-h-[40vh] max-h-[50vh] border border-gray-700"
                >
                    {FilterLogs().map((log, index) => (
                        <div key={index} className="text-sm py-2 border-b border-gray-700">
                            {log}
                        </div>
                    ))}
                </div>

                <button
                    onClick={onClose}
                    className="mt-2 px-4 py-3 bg-red-500 rounded hover:bg-red-600 text-sm"
                >
                    Close
                </button>
            </div>
        </div>
    );
}

export default LogViewer;
