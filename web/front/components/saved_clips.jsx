import React, { useEffect, useState } from "react";
import websocket_handler from "../ws_handler";

function SavedClips({ is_open, onClose })
{
    const [clips, set_clips] = useState([]);
    const [selected_clip, set_selected_clip] = useState(null);
    const [video_src, set_video_src] = useState(null);
    const [show_video_modal, set_show_video_modal] = useState(false);


    useEffect(() =>
    {
        /// Handles messages from the backend
        function HandleMessage(message)
        {
            /// If we receive a video list, update the clips
            if (message.type === "video_list")
            {
                set_clips(message.videos);
            }
            else if (message.type === "video_url" && message.url)
            {
                /// otherwise, if we receive a video url, update the video source
                const ip = window.location.hostname;
                set_video_src("http://" + ip + ":3000" + message.url);
            }
        }

        if (is_open)
        {
            /// On open, subscribe to the message event and request the video list
            websocket_handler.Subscribe("message", HandleMessage);
            websocket_handler.SendCommand("get_videos", { args: { type: "list" } });
        }

        return () =>
        {
            /// Unsubscribe from the messages when we're done
            websocket_handler.Unsubscribe("message", HandleMessage);
        };
    }, [is_open, selected_clip]);

    /// Request the backend to send the video stream
    function RequestClip(filename)
    {
        set_selected_clip(filename);
        set_video_src(null);
        set_show_video_modal(true);

        /// request the backend to ask the C++ to send the video stream. After the backend receives the video stream, it will send the url back to us
        websocket_handler.SendCommand("get_videos", {args: { type: "stream", video: filename }});
    }

    /// Send a delete command to the backend to delete the video
    function DeleteClip(filename)
    {
        websocket_handler.SendCommand("get_videos", {args: { type: "delete", video: filename }});
        /// Remove it on our end
        set_clips((prev) => prev.filter((clip) => clip !== filename));
    }

    /// Close the video modal
    function CloseVideo()
    {
        set_show_video_modal(false);
        set_video_src(null);
    }

    ///Parses the filename to get the motion type, date and duration
    function FormatClipName(filename)
    {
        /// First, ensure the filename is valid
        if (!filename || typeof filename !== "string")
        {
            /// Fallback to unknown if it's not
            return { type: "🎥 Unknown Motion", datetime: "Unknown", duration: "Unknown" };
        }

        let name = filename;


        /// remove the .mp4 extension
        if (name.endsWith(".mp4"))
        {
            name = name.slice(0, -4);
        }

        /// remove the motion_ prefix
        if (name.startsWith("motion_"))
        {
            name = name.slice(7);
        }

        /// split the name by underscores, which should give us three parts at this point
        /// the time/date, the duration, and the detected object
        const parts = name.split("_");


        /// If we don't have at least 3 parts, we can't parse the filename
        if (parts.length < 3)
        {
            /// and fallback to unknown
            return { type: "🎥 Unknown Motion", datetime: "Unknown", duration: "Unknown" };
        }


        let raw_date = parts[0];
        let raw_time = "";
        let duration_index = 1;

        /// If the second part contains a dash, it means the time is in the first part
        if (parts[1].includes("-"))
        {
            raw_time = parts[1];
            duration_index = 2;
        }
        else
        {
            raw_time = raw_date.split(" ")[1];
            raw_date = raw_date.split(" ")[0];
        }

        /// Parse the duration
        const duration_sec = parseInt(parts[duration_index]);

        if (isNaN(duration_sec))
        {
            /// If we can't parse the duration, fallback to unknown
            return { type: "🎥 Unknown Motion", datetime: "Unknown", duration: "Unknown" };
        }

        /// extract the detected object
        const detected_obj = parts.length > duration_index + 1
            ? parts[duration_index + 1]
            : "unknown";

        /// here we just format the date and time to a more readable format
        const [year, month, day] = raw_date.split("-");
        const formatted_time = raw_time.replace(/-/g, ":");
        const formatted_date = `${day}/${month}/${year} ${formatted_time}`;

        const mins = Math.floor(duration_sec / 60);
        const secs = duration_sec % 60;
        const formatted_duration = `${mins > 0 ? mins + "m " : ""}${secs}s`;

        /// Set a default motion type based on the detected object metadata
        let motion_type = "🎥 Unknown Motion";
        if (detected_obj.includes("human") || detected_obj.includes("person"))
        {
            motion_type = "🎥 Human Motion";
        }
        else if (detected_obj.includes("pet"))
        {
            motion_type = "🐶 Pet Motion";
        }
        else if (detected_obj.includes("vehicle"))
        {
            motion_type = "🚗 Vehicle Motion";
        }
        else
        {
            /// Shouldn't happen, future proofing.
            motion_type = `🎥 Unknown Motion (${detected_obj})`;
        }

        /// Return the formatted data for the given clip
        return {
            type: motion_type,
            datetime: formatted_date,
            duration: formatted_duration
        };
    }

    if (!is_open)
    {
        return null;
    }

    return (
        <div className="fixed inset-0 bg-black/30 backdrop-blur-lg flex items-center justify-center p-4 z-50">
            <div className="bg-gray-900 p-4 rounded-lg w-full max-w-[95vw] max-h-[90vh] overflow-auto flex flex-col shadow-lg">

                <h2 className="text-lg font-bold text-white mb-4">
                    🎥 Saved Clips
                </h2>

                <div className="grid grid-cols-[1fr_1fr_1fr_1fr] bg-gray-800 text-white p-2 font-semibold rounded-t-lg text-center">
                    <div className="text-left pl-4">Motion Type</div>
                    <div className="text-center">Date & Time</div>
                    <div className="text-center">Duration</div>
                    <div className="text-right pr-2">Actions</div>
                </div>

                <div className="overflow-auto bg-gray-800/80 backdrop-blur-md text-white p-2 rounded-b-lg min-h-[40vh] max-h-[50vh]">
                    {clips.map((filename, index) =>
                    {
                        /// for each clip, format the name and display it
                        const { type, datetime, duration } = FormatClipName(filename);

                        return (
                            <div
                                key={filename || index}
                                className="grid grid-cols-[1fr_1fr_1fr_1fr] p-3 text-sm bg-gray-700 hover:bg-gray-600 rounded-lg my-1 items-center gap-2"
                            >
                                <div className="pl-4 font-semibold flex items-center">{type}</div>
                                <div className="text-center flex items-center justify-center">{datetime}</div>
                                <div className="text-center flex items-center justify-center">{duration}</div>

                                <div className="flex gap-2 justify-end pr-2">
                                    <button
                                        onClick={() => RequestClip(filename)}
                                        className="px-2 py-1 bg-blue-500 rounded hover:bg-blue-600 text-sm"
                                    >
                                        ▶ Play
                                    </button>
                                    <button
                                        onClick={() => DeleteClip(filename)}
                                        className="px-2 py-1 bg-red-500 rounded hover:bg-red-600 text-sm"
                                    >
                                        ❌ Delete
                                    </button>
                                </div>
                            </div>
                        );
                    })}
                </div>

                {/* Close Button */}
                <button
                    onClick={onClose}
                    className="mt-2 px-4 py-3 bg-red-500 rounded hover:bg-red-600 text-sm"
                >
                    Close
                </button>
            </div>

            {show_video_modal && (
                <div className="fixed inset-0 flex items-center justify-center bg-black/40 backdrop-blur-lg">
                    <div className="bg-gray-900 p-4 rounded-lg shadow-lg relative max-w-2xl w-full">

                        <h2 className="text-lg font-bold text-white">
                            🎬 Video Player
                        </h2>

                        <div className="relative w-full h-64 bg-transparent flex items-center justify-center rounded">
                            {video_src ? (
                                <video
                                    className="w-full h-full object-contain rounded"
                                    src={video_src}
                                    controls
                                    autoPlay
                                />
                            ) : (
                                <p className="text-gray-400">
                                    📡 Loading video...
                                </p>
                            )}
                        </div>

                        <button
                            onClick={CloseVideo}
                            className="mt-2 px-4 py-2 bg-red-500 rounded hover:bg-red-600 text-sm w-full"
                        >
                            Close Video
                        </button>
                    </div>
                </div>
            )}
        </div>
    );
}

export default SavedClips;
