import { useState, useEffect } from "react";
import websocketHandler from "../core/ws_handler";

function LiveFeed()
{
    const [frame, setFrame] = useState(null);

    useEffect(() =>
    {
        websocketHandler.RequestLiveFeed();

        return () =>
        {
            websocketHandler.StopLiveFeed();
        };
    }, []);

    return (
        <div className="flex flex-col items-center p-4">
            <h1 className="text-2xl font-bold text-white">Live Feed</h1>
            {frame ? (
                <img src={frame} className="mt-4 border border-gray-700 rounded-lg" />
            ) : (
                <p className="text-gray-400">Waiting for motion...</p>
            )}
        </div>
    );
}

export default LiveFeed;
