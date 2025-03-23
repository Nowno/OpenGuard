import React, { useEffect, useState } from "react";
import websocket_handler from "../ws_handler";

function HookEditor({ is_open, onClose })
{
    /// States for the hooks
    const [hooks, set_hooks] = useState({});
    const [new_hook_name, set_new_hook_name] = useState("");
    const [selected_hook, set_selected_hook] = useState(null);
    const [hook_content, set_hook_content] = useState("");
    const [expanded_folders, set_expanded_folders] = useState({});
    const [selected_folder, set_selected_folder] = useState("");

    useEffect(() =>
    {
        if (!is_open)
        {
            return;
        }

        /// When we open the editor, we request the list of hooks
        websocket_handler.SendCommand("get_hooks", { args: { type: "list" } });

        /// Subscribe to the message event
        websocket_handler.Subscribe("message", HandleMessage);

        /// Unsubscribe when the editor is closed
        return () => websocket_handler.Unsubscribe("message", HandleMessage);
    }, [is_open]);

    function HandleMessage(message)
    {
        /// If we receive a request of type hook list, we parse them and update our structure
        if (message.type === "hook_list")
        {
            /// Sanity check though shouldn't happen
            if (!Array.isArray(message.hooks))
            {
                return;
            }

            set_hooks((prev_hooks) =>
            {
                /// Append the new hooks to the existing ones
                const new_hooks = { ...prev_hooks };

                /// Organize the hooks in a tree structure
                const folder_tree = OrganizeHooks(message.hooks);

                for (const folder in folder_tree)
                {
                    /// If the folder doesn't exist, we add it
                    new_hooks[folder] = folder_tree[folder];
                }

                return new_hooks;
            });
        }
        else if (message.type === "hook_content")
        {
            /// Otherwise if the request is hook content, we update the content
            set_hook_content(message.content);
        }
    }

    /// Function to organize the hooks in a tree structure
    function OrganizeHooks(hook_paths)
    {
        const tree = {};

        /// Anothoer sanity check
        if (!Array.isArray(hook_paths))
        {
            return tree;
        }

        hook_paths.forEach((hook_path) =>
        {
            /// We replace the backslashes with forward slashes and split the path
            const parts = hook_path.replace(/\\/g, "/").split("/").slice(1);
            let current = tree;

            /// now, for each part of the path, we create a nested object
            parts.forEach((part, index) =>
            {
                ///if we're at the end of the path we set the value to null
                if (index === parts.length - 1)
                {
                    current[part] = null;
                }
                else
                {
                    /// otherwise we create a new object as we go deeper
                    if (!current[part])
                    {
                        current[part] = {};
                    }

                    current = current[part];
                }
            });
        });

        /// Return the tree
        return tree;
    }

    /// Manages the expansion of folders
    function ToggleFolder(folder)
    {
        set_expanded_folders((prev) =>
        {
            const is_expanding = !prev[folder];

            if (is_expanding)
            {
                /// Request the hooks in a given folder
                websocket_handler.SendCommand("get_hooks", { args: { type: "list", folder } });
            }

            return { ...prev, [folder]: is_expanding };
        });
    }

    /// Loads the content of a hook
    function LoadHook(folder_path, filename)
    {
        /// This function is called when a hook is clicked on, and it fetches its content
        const full_path = `${folder_path}/${filename}`;
        set_selected_hook(full_path);
        websocket_handler.SendCommand("get_hooks", {args: { type: "get", hook: full_path }});
    }

    /// Saves the content of a hook
    function SaveHook()
    {
        /// If we have nothing selected (shouldn't happen), we return
        if (!selected_hook)
        {
            return;
        }

        /// Send the content of either our new hook, or edited hook to the backend
        websocket_handler.SendCommand("get_hooks", {args: {type: "save", content: hook_content, file_name: selected_hook}});
    }

    function DeleteHook(folder_path, filename)
    {
        const full_path = `${folder_path}/${filename}`;

        /// Send a delete command to the backend with the full path of the hook, the C++ does the heavy lifting
        websocket_handler.SendCommand("get_hooks", {args: { type: "delete", hook: full_path }});

        set_hooks((prev_hooks) =>
        {

            const new_hooks = { ...prev_hooks };
            const folder = new_hooks[folder_path] || {};

            /// Delete the hook from the folder
            delete folder[filename];

            /// If the folder is empty, we delete it
            /// btw, this is future proofing, but this shouldn't happen.
            if (Object.keys(folder).length === 0)
            {
                delete new_hooks[folder_path];
            }
            else
            {
                new_hooks[folder_path] = folder;
            }

            /// Update the hooks
            return new_hooks;
        });

        if (selected_hook === full_path)
        {
            set_selected_hook(null);
            set_hook_content("");
        }
    }

    function CreateHook()
    {
        /// Was created in a robust way to support nested folders in the future.
        /// if no name was given,  return
        if (!new_hook_name.trim())
        {
            return;
        }

        /// form the name, and add .py
        const full_path = selected_folder ? `${selected_folder}/${new_hook_name}.py` : new_hook_name;

        set_hooks((prev_hooks) =>
        {
            const new_hooks = { ...prev_hooks };
            let current = new_hooks;

            const parts = full_path.split("/");

            /// Similar logic to the one visitred before, here we create the folder structure and include the new hook
            parts.forEach((part, index) =>
            {
                /// Keep going until we reach the end of the path
                if (index === parts.length - 1)
                {
                    current[part] = null;
                }
                else
                {
                    if (!current[part])
                    {
                        current[part] = {};
                    }

                    current = current[part];
                }
            });

            return new_hooks;
        });

        set_new_hook_name("");
    }

    function RenderHookTree(tree, parent_path = "")
    {
        return Object.keys(tree).map((key) =>
        {
            const full_path = parent_path ? `${parent_path}/${key}` : key;
            const is_folder = tree[key] !== null;

            return (
                <div key={full_path} className="ml-2">
                    {is_folder ? (
                        <div className="flex items-center text-white font-semibold cursor-pointer p-1 rounded hover:bg-gray-700">
                            <span className="mr-2" onClick={() => ToggleFolder(full_path)}>
                                {expanded_folders[full_path] ? "📂 ▼" : "📁 ▶" /* Emojis made it a lot easier in here for the first time in my life */}
                            </span>
                            <span
                                className={`flex-1 ${selected_folder === full_path ? "bg-blue-600 p-1 rounded" : ""}`}
                                onClick={() => set_selected_folder(full_path)}
                            >
                                {key}
                            </span>
                        </div>
                    ) : (
                        <div
                            className={`flex justify-between items-center p-1 pl-4 text-white rounded ${selected_hook === `${parent_path}/${key}` ? "bg-blue-600" : "hover:bg-gray-700"}`}
                        >
                            <button
                                onClick={() => LoadHook(parent_path, key)}
                                className="text-left flex-1 hover:text-blue-400"
                            >
                                📄 {key}
                            </button>
                            <button
                                onClick={() => DeleteHook(parent_path, key)}
                                className="text-red-500 hover:text-red-700"
                            >
                                ❌
                            </button>
                        </div>
                    )}
                    {is_folder && expanded_folders[full_path] && (
                        <div className="ml-4">{RenderHookTree(tree[key], full_path)}</div>
                    )}
                </div>
            );
        });
    }

    if (!is_open)
    {
        return null;
    }

    return (
        <div className="fixed inset-0 bg-black/30 backdrop-blur-lg flex items-center justify-center p-4 z-50">
            <div className="bg-gray-900 p-4 rounded-lg w-full max-w-[800px] max-h-[90vh] overflow-hidden shadow-lg flex flex-col">

                <h2 className="text-lg font-bold text-white mb-4">🛠️ Hook Editor</h2>

                <div className="flex flex-col md:flex-row gap-4 flex-grow">

                    {/* Hook Tree */}
                    <div className="w-full md:w-1/3 bg-gray-800 p-2 rounded flex flex-col">
                        <h3 className="text-white text-sm font-semibold mb-2">📂 Available Hooks</h3>

                        <div className="overflow-auto flex-grow">
                            {RenderHookTree(hooks)}
                        </div>

                        <div className="mt-2 flex flex-col bg-gray-900 p-2 rounded sticky bottom-0 w-full">
                            {selected_folder && (
                                <div className="text-xs text-gray-400 mb-1">
                                    Adding inside: <span className="text-blue-400">{selected_folder}</span>
                                </div>
                            )}

                            <div className="flex items-center">
                                <input
                                    type="text"
                                    className="p-2 bg-gray-700 text-white rounded flex-grow min-w-0"
                                    placeholder="New Hook Name"
                                    value={new_hook_name}
                                    onChange={(e) => set_new_hook_name(e.target.value)}
                                />

                                <button
                                    onClick={CreateHook}
                                    className="ml-2 px-3 py-2 bg-green-500 rounded hover:bg-green-600 text-sm flex items-center justify-center whitespace-nowrap"
                                    style={{ minWidth: "70px" }}
                                >
                                    ➕ Add
                                </button>
                            </div>
                        </div>
                    </div>

                    {/*  Editor */}
                    <div className="w-full md:w-2/3 flex flex-col">
                        <h3 className="text-white text-sm font-semibold mb-2">📝 Hook Content</h3>

                        {selected_hook ? (
                            <>
                                <textarea
                                    className="w-full p-2 bg-gray-700 text-white rounded flex-grow min-h-[300px]"
                                    value={hook_content}
                                    onChange={(e) => set_hook_content(e.target.value)}
                                />

                                <button
                                    onClick={SaveHook}
                                    className="mt-2 px-4 py-2 bg-blue-500 rounded hover:bg-blue-600 text-sm w-full"
                                >
                                    💾 Save Hook
                                </button>
                            </>
                        ) : (
                            <p className="text-gray-400">Select a hook to edit...</p>
                        )}
                    </div>
                </div>

                <button
                    onClick={onClose}
                    className="mt-4 px-4 py-2 bg-red-500 rounded hover:bg-red-600 text-sm w-full"
                >
                    Close Editor
                </button>
            </div>
        </div>
    );
}

export default HookEditor;
