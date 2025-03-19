import React, { useEffect, useState, useRef } from "react";
import websocket_handler from "./ws_handler";

function LogViewer({ is_open, onClose })
{
    const [logs, setLogs] = useState([]);
    const [filter, setFilter] = useState("all");
    const log_container_ref = useRef(null);

    useEffect(() =>
    {
        function HandleLogMessage(message)
        {
            if (message.type === "log")
            {
                setLogs(prevLogs => [...prevLogs, message.log].slice(-100));
            }

            if (message.type === "log_dump")
            {
                const newLogs = message.logs.split("\n").filter(log => log.trim() !== "");
                setLogs(prevLogs => [...prevLogs, ...newLogs].slice(-100));
            }
        }

        if (is_open)
        {
            websocket_handler.Subscribe("message", HandleLogMessage);
            websocket_handler.SendCommand({type: "get_logs", args: {type: "stream_start"}});

            if (logs.length === 0)
                websocket_handler.SendCommand("get_logs");
        }

        return () =>
        {
            websocket_handler.SendCommand({type: "get_logs", args: {type: "stream_stop"}});
            websocket_handler.Unsubscribe("message", HandleLogMessage);
        };
    }, [is_open]);

    useEffect(() =>
    {
        if (log_container_ref.current)
        {
            log_container_ref.current.scrollTop = log_container_ref.current.scrollHeight;
        }
    }, [logs]);

    function ClearLogs()
    {
        setLogs([]);
    }

    function FilterLogs()
    {
        if (filter === "all") return logs;
        return logs.filter(log => log.toLowerCase().includes(filter));
    }

    if (!is_open) return null;

    return (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center">
            <div className="bg-gray-800 p-4 rounded-lg w-3/4 max-h-[80vh] flex flex-col">
                <h2 className="text-lg font-bold text-white">📜 Log Viewer</h2>

                {/* Filters */}
                <div className="flex gap-2 my-2">
                    <button onClick={() => setFilter("all")} className="px-4 py-2 bg-blue-500 rounded hover:bg-blue-600">
                        All
                    </button>
                    <button onClick={() => setFilter("error")} className="px-4 py-2 bg-red-500 rounded hover:bg-red-600">
                        Errors
                    </button>
                    <button onClick={() => setFilter("info")} className="px-4 py-2 bg-yellow-500 rounded hover:bg-yellow-600">
                        Info
                    </button>
                    <button onClick={ClearLogs} className="ml-auto px-4 py-2 bg-gray-500 rounded hover:bg-gray-600">
                        Clear
                    </button>
                </div>

                {/* Log List */}
                <div ref={log_container_ref} className="overflow-auto bg-black text-white p-2 rounded h-[60vh] border border-gray-700">
                    {FilterLogs().map((log, index) => (
                        <div key={index} className="text-sm py-1 border-b border-gray-700">
                            {log}
                        </div>
                    ))}
                </div>

                {/* Close Button */}
                <button onClick={() =>
                {
                    onClose();
                    if (log_container_ref.current)
                    {
                        log_container_ref.current.scrollTop = 0;
                    }
                }} className="mt-2 px-4 py-2 bg-red-500 rounded hover:bg-red-600">
                    Close
                </button>
            </div>
        </div>
    );
}

export default LogViewer;
