import React, { useEffect, useState, useRef } from "react";
import websocket_handler from "../ws_handler";

function LiveFeed()
{
    // States
    const [image, set_image] = useState(null);
    const [roi, set_roi] = useState(null);
    const [is_selecting, set_is_selecting] = useState(false);
    const [start_point, set_start_point] = useState(null);

    const canvas_ref = useRef(null);

    useEffect(() =>
    {

        /// Clears the image when the websocket is closed, broken but I like the way it looks anyways
        function CloseCallback()
        {
            set_image(null);
        }

        /// Handles messages from the backend
        function MessageCallback(message)
        {
            /// If we receive a frame, update the image
            if (message.type === "frame")
            {
                /// Specify that it's base64 encoded
                set_image(`data:image/jpeg;base64,${message.image}`);
            }

            /// If we receive a status message, check if the feed is disconnected
            if (message.type === "openguard_status")
            {
                if (message.status === "Disconnected")
                {
                    set_image(null);
                }
                else
                {
                    /// Otherwise, if we're connected, request a snapshot
                    websocket_handler.SendCommand("snapshot", {});
                }
            }
        }

        /// When we load it first time, we request a snapshot, and subscribe to the message and close events
        websocket_handler.SendCommand("snapshot", {});
        websocket_handler.Subscribe("message", MessageCallback);
        websocket_handler.Subscribe("close", CloseCallback);

        return () =>
        {
            /// Unsubscribe from the events when we're done
            websocket_handler.Unsubscribe("message", MessageCallback);
            websocket_handler.Unsubscribe("close", CloseCallback);
        };
    }, []);

    // Draw canvas and roi.
    useEffect(() =>
    {
        /// If we don't have an image or the canvas is not ready, return
        if (!image || !canvas_ref.current)
        {
            return;
        }

        /// get the canvas and ctx
        const canvas = canvas_ref.current;
        const ctx = canvas.getContext("2d");
        const img = new Image();        /// Create a new image object

        img.onload = () =>
        {
            // When the image is loaded, resize the canvas to match the image size
            canvas.width = canvas.clientWidth;
            canvas.height = canvas.clientHeight;

            /// Clear the canvas and draw the image
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            ctx.drawImage(img, 0, 0, canvas.width, canvas.height);

            /// If we have an roi, draw it
            if (roi)
            {
                ctx.strokeStyle = "red";
                ctx.lineWidth = 2;
                ctx.strokeRect(roi.x, roi.y, roi.width, roi.height);
            }
        };

        img.src = image;
    }, [image, roi]);

    /// Get canva relative coords from event
    function GetEventCoords(event)
    {
        let client_x, client_y;

        /// Added support for phones
        if (event.touches)
        {
            /// Get the position of the first touch
            client_x = event.touches[0].clientX;
            client_y = event.touches[0].clientY;
        }
        else
        {
            /// if its not a touch event, get the position of the mouse
            client_x = event.clientX;
            client_y = event.clientY;
        }

        /// If the canvas is not ready, return null
        if (!canvas_ref.current)
        {
            return null;
        }

        /// Otherwise, get the relative position of the event
        const rect = canvas_ref.current.getBoundingClientRect();

        /// Return the position of the event relative to the canvas
        return {
            x: client_x - rect.left,
            y: client_y - rect.top
        };
    }

    /// Handle the start of the selection
    function HandleStart(event)
    {
        /// Small hotfixes, prevent default and disable scrolling
        /// because it made it super unpractical to select
        event.preventDefault();
        DisableScroll();

        const start = GetEventCoords(event);

        if (!start)
        {
            return;
        }

        set_start_point(start);
        set_is_selecting(true);
    }

    /// Update ROI as we move the mouse, helping the user to see what they're selecting
    function HandleMove(event)
    {
        /// Once again, prevent default
        event.preventDefault();

        if (!is_selecting)
        {
            return;
        }

        /// Get the current position of the mouse/touch
        const current = GetEventCoords(event);

        if (!current || !start_point)
        {
            return;
        }

        const x = Math.min(start_point.x, current.x);
        const y = Math.min(start_point.y, current.y);
        const width = Math.abs(current.x - start_point.x);
        const height = Math.abs(current.y - start_point.y);

        /// update the roi
        set_roi({ x, y, width, height });
    }

    /// Handle the end of the selection, the release of the mouse/touch
    function HandleEnd()
    {
        /// restore what we disabled
        EnableScroll();
        set_is_selecting(false);

        /// Also have a certain standard as to how big the roi should be
        if (roi && roi.width > 10 && roi.height > 10)
        {
            /// When we auth'd we received the config file of the C++, we use it to scale the roi as we may be displaying a smaller image
            const scale_x = websocket_handler.og_config.frame_width / canvas_ref.current.clientWidth;
            const scale_y = websocket_handler.og_config.frame_height / canvas_ref.current.clientHeight;

            /// Scale the roi and send it to the backend
            const scaled_roi = {
                x: Math.round(roi.x * scale_x),
                y: Math.round(roi.y * scale_y),
                width: Math.round(roi.width * scale_x),
                height: Math.round(roi.height * scale_y)
            };

            websocket_handler.SendCommand("roi_select", {
                args: {
                    x: scaled_roi.x,
                    y: scaled_roi.y,
                    width: scaled_roi.width,
                    height: scaled_roi.height,
                    type: "set"
                }
            });
        }
        else
        {
            /// Otherwise, if the roi is too small, reset it
            set_roi(null);
            /// Additionally, reset the roi on the backend
            const scaled_roi = {
                x: 0,
                y: 0,
                width: websocket_handler.og_config.frame_width,
                height: websocket_handler.og_config.frame_height
            };

            websocket_handler.SendCommand("roi_select", {
                args: {
                    x: scaled_roi.x,
                    y: scaled_roi.y,
                    width: scaled_roi.width,
                    height: scaled_roi.height,
                    type: "reset"
                }
            });

        }
    }

    /// Hacky way to disable scrolling while selecting
    function DisableScroll()
    {
        document.body.style.overflow = "hidden";
        document.body.style.touchAction = "none";

        document.addEventListener("touchmove", PreventDefault, { passive: false });
    }

    /// Re-enable scrolling
    function EnableScroll()
    {
        document.body.style.overflow = "";
        document.body.style.touchAction = "";

        document.removeEventListener("touchmove", PreventDefault);
    }

    /// events default browser behavior during touchmove
    function PreventDefault(event)
    {
        event.preventDefault();
    }

    return (
        <div className="w-full max-w-lg aspect-video flex items-center justify-center bg-black border border-gray-700 overflow-hidden">
            <canvas
                ref={canvas_ref}
                className="w-full h-full"
                onMouseDown={HandleStart}
                onMouseMove={HandleMove}
                onMouseUp={HandleEnd}
                onTouchStart={HandleStart}
                onTouchMove={HandleMove}
                onTouchEnd={HandleEnd}
            />

            {!image &&
                <p className="absolute text-white">
                    📡 Waiting for live feed...
                </p>
            }
        </div>
    );
}

export default LiveFeed;
