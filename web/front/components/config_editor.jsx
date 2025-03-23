import React, { useState } from "react";
import websocket_handler from "../ws_handler";

function ConfigEditor({ is_open, onClose })
{
    if (!is_open)
    {
        return null;
    }

    const [edited_config, set_edited_config] = useState({ ...websocket_handler.og_config });


    /// Callback to handle changes in the input fields
    function HandleChange(key, value)
    {
        set_edited_config((prev_config) =>
        {
            const current_value = prev_config[key];
            let new_value = value;

            /// Small edge case for floats, they used to get converted to strings
            if (typeof current_value === "number")
            {
                new_value = parseFloat(value);

                if (isNaN(new_value))
                {
                    new_value = current_value;
                }
            }

            return {...prev_config, [key]: new_value};
        });
    }

    function SaveConfig()
    {
        /// Simple send the edited config to the backend, pretify it in case users change it from the file directly
        websocket_handler.SendCommand("set_config", {args: {config: JSON.stringify(edited_config, null, 4)}});

        /// Notify the user they will only take effect next restart
        alert("✅ Configuration will be updated next restart.");
        onClose();
    }

    return (
        <div className="fixed inset-0 bg-black/30 backdrop-blur-lg flex items-center justify-center p-4 z-50">
            <div className="bg-gray-900 p-6 rounded-lg w-full max-w-[90vw] max-h-[90vh] overflow-auto flex flex-col shadow-lg">

                <h2 className="text-lg font-bold text-white mb-4">
                    ⚙️ Edit Configuration
                </h2>

                {}
                <div className="overflow-auto bg-gray-800/80 backdrop-blur-md text-white p-3 rounded-lg h-[50vh]">
                    {Object.entries(edited_config).map(([key, value]) =>
                        (
                            <div key={key} className="mb-4">
                                <label className="text-white text-sm font-semibold">{key}</label>

                                {Array.isArray(value) ? (
                                    <input
                                        className="w-full p-3 bg-gray-700 text-white rounded text-base"
                                        type="text"
                                        value={value.join(", ")}
                                        onChange={(e) =>
                                            HandleChange(key, e.target.value.split(",").map(v => v.trim()))
                                        }
                                    />
                                ) : (typeof value === "boolean" || value === "true" || value === "false") ? (
                                    <select
                                        className="w-full p-3 bg-gray-700 text-white rounded text-base"
                                        value={value.toString()}
                                        onChange={(e) => HandleChange(key, e.target.value === "true")}
                                    >
                                        <option value="true">True</option>
                                        <option value="false">False</option>
                                    </select>
                                ) : (
                                    <input
                                        className="w-full p-3 bg-gray-700 text-white rounded text-base"
                                        type={typeof value === "number" ? "number" : "text"}
                                        value={value}
                                        onChange={(e) => HandleChange(key, e.target.value)}
                                    />
                                )}
                            </div>
                        ))}
                </div>

                {}
                <div className="flex justify-between mt-4">
                    <button
                        onClick={onClose}
                        className="px-4 py-3 bg-gray-500 rounded hover:bg-gray-600 text-sm"
                    >
                        Cancel
                    </button>

                    <button
                        onClick={SaveConfig}
                        className="px-4 py-3 bg-blue-500 rounded hover:bg-blue-600 text-sm"
                    >
                        Save Changes
                    </button>
                </div>
            </div>
        </div>
    );
}

export default ConfigEditor;
