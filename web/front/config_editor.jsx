import React, { useState } from "react";
import websocket_handler from "./ws_handler";

function ConfigEditor({ is_open, onClose })
{
    if (!is_open) return null;

    const [edited_config, setEditedConfig] = useState({ ...websocket_handler.og_config });

    function HandleChange(key, value)
    {
        setEditedConfig((prev) => ({ ...prev, [key]: value }));
    }

    function SaveConfig()
    {
        websocket_handler.og_config = edited_config;
        websocket_handler.SendCommand("update_config", { config: edited_config });
        onClose();
    }

    return (
        <div className="fixed inset-0 flex items-center justify-center bg-black bg-opacity-50">
            <div className="bg-gray-800 p-6 rounded-lg w-full max-w-lg">
                <h2 className="text-xl font-semibold text-white mb-4">Edit Configuration</h2>

                <div className="max-h-96 overflow-y-auto">
                    {Object.entries(edited_config).map(([key, value]) =>
                    {
                        return (
                            <div key={key} className="mb-2">
                                <label className="text-white text-sm">{key}</label>
                                {Array.isArray(value) ? (
                                    <input
                                        className="w-full p-2 bg-gray-700 text-white rounded"
                                        type="text"
                                        value={value.join(", ")}
                                        onChange={(e) => HandleChange(key, e.target.value.split(", ").map(v => v.trim()))}
                                    />
                                ) : typeof value === "boolean" || value === "true" || value === "false" ? (
                                    <select
                                        className="w-full p-2 bg-gray-700 text-white rounded"
                                        value={value.toString()}
                                        onChange={(e) => HandleChange(key, e.target.value === "true")}
                                    >
                                        <option value="true">True</option>
                                        <option value="false">False</option>
                                    </select>
                                ) : (
                                    <input
                                        className="w-full p-2 bg-gray-700 text-white rounded"
                                        type={typeof value === "number" ? "number" : "text"}
                                        value={value}
                                        onChange={(e) => HandleChange(key, e.target.value)}
                                    />
                                )}
                            </div>
                        );
                    })}
                </div>

                <div className="flex justify-end mt-4">
                    <button onClick={onClose} className="mr-2 px-4 py-2 bg-gray-500 rounded hover:bg-gray-600">
                        Cancel
                    </button>
                    <button onClick={SaveConfig} className="px-4 py-2 bg-blue-500 rounded hover:bg-blue-600">
                        Save Changes
                    </button>
                </div>
            </div>
        </div>
    );
}

export default ConfigEditor;
