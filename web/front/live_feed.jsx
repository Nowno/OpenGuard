import React, { useEffect, useState, useRef } from "react";
import websocket_handler from "./core/ws_handler";

function LiveFeed()
{
    const [image, setImage] = useState(null);
    const [roi, setROI] = useState(null);
    const [isSelecting, setIsSelecting] = useState(false);
    const [startPoint, setStartPoint] = useState(null);
    const canvas_ref = useRef(null);

    useEffect(() =>
    {
        function CloseCallback()
        {
            setImage(null);
        }

        function MessageCallback(message)
        {
            if (message.type === "frame")
            {
                setImage(`data:image/jpeg;base64,${message.image}`);
            }
            if (message.type === "openguard_status")
            {
                if (message.status === "Disconnected")
                {
                    setImage(null);
                }
                else
                {
                    websocket_handler.SendCommand("snapshot", {});
                }
            }
        }

        websocket_handler.SendCommand("snapshot", {});
        websocket_handler.Subscribe("message", MessageCallback);
        websocket_handler.Subscribe("close", CloseCallback);

        return () =>
        {
            websocket_handler.Unsubscribe("message", MessageCallback);
            websocket_handler.Unsubscribe("close", CloseCallback);
        };
    }, []);

    useEffect(() =>
    {
        if (!image || !canvas_ref.current) return;

        const canvas = canvas_ref.current;
        const ctx = canvas.getContext("2d");
        const img = new Image();

        img.onload = () =>
        {
            canvas.width = canvas.clientWidth;
            canvas.height = canvas.clientHeight;
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            ctx.drawImage(img, 0, 0, canvas.width, canvas.height);

            if (roi)
            {
                ctx.strokeStyle = "red";
                ctx.lineWidth = 2;
                ctx.strokeRect(roi.x, roi.y, roi.width, roi.height);
            }
        };

        img.src = image;
    }, [image, roi]);

    function GetEventCoords(event)
    {
        let clientX, clientY;

        if (event.touches)
        {
            clientX = event.touches[0].clientX;
            clientY = event.touches[0].clientY;
        }
        else
        {
            clientX = event.clientX;
            clientY = event.clientY;
        }

        if (!canvas_ref.current) return null;
        const rect = canvas_ref.current.getBoundingClientRect();
        return {
            x: clientX - rect.left,
            y: clientY - rect.top
        };
    }

    function HandleStart(event)
    {
        event.preventDefault();
        DisableScroll();

        const start = GetEventCoords(event);
        if (!start) return;

        setStartPoint(start);
        setIsSelecting(true);
    }

    function HandleMove(event)
    {
        event.preventDefault();

        if (!isSelecting) return;

        const current = GetEventCoords(event);
        if (!current || !startPoint) return;

        const width = current.x - startPoint.x;
        const height = current.y - startPoint.y;

        setROI({ x: startPoint.x, y: startPoint.y, width, height });
    }

    function HandleEnd()
    {
        EnableScroll();
        setIsSelecting(false);

        if (roi && Math.abs(roi.width) > 10 && Math.abs(roi.height) > 10)
        {
            console.log("📤 Sending ROI to OpenGuard:", roi);
            websocket_handler.SendCommand("roi_select", { roi });
        }
        else
        {
            console.log("⚠️ ROI selection too small, ignoring.");
        }
    }

    function DisableScroll()
    {
        document.body.style.overflow = "hidden";
        document.body.style.touchAction = "none";
        document.addEventListener("touchmove", PreventDefault, { passive: false });
    }

    function EnableScroll()
    {
        document.body.style.overflow = "";
        document.body.style.touchAction = ""; // Re-enable touch gestures
        document.removeEventListener("touchmove", PreventDefault);
    }

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
            {!image && <p className="absolute text-white">📡 Waiting for live feed...</p>}
        </div>
    );
}

export default LiveFeed;