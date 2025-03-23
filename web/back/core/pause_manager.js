let is_paused = false;
let resume_time = null;
let resume_timer = null;

/**
 * Returns the current pause state.
 */
export function GetPauseState()
{
    return { is_paused, resume_time };
}

/**
 * Handles a pause motion command..
 */
export function HandlePauseMotion(handler, duration)
{
    if (!duration || duration <= 0)
        return;

    const end_time = Date.now() + duration * 1000;
    is_paused = true;
    resume_time = end_time;

    /// Send the pause command to the OpenGuard system.
    handler.SendToOpenGuard({ type: "pause_system", args: { until: Math.floor(end_time / 1000) } });

    /// If there is a timer running, clear it.
    if (resume_timer)
    {
        clearTimeout(resume_timer);
    }

    /// Set a new timer to resume motion detection when the duration is over.
    resume_timer = setTimeout(() => HandleResumeMotion(handler), duration * 1000);

    /// And once again broadcast the pause status to all clients.
    handler.BroadcastToClients({ type: "pause_status", is_paused: true, resume_time: end_time });
}

/**
 * Handles a resume motion command.
 */
export function HandleResumeMotion(handler)
{
    is_paused = false;
    resume_time = null;

    /// C++ is made such as that if 0 is sent, it will pause till time(0) + 0, making it resume.
    handler.SendToOpenGuard({ type: "pause_system", args: { until: 0 } });

    /// Same as before, clear the timer if it's running.
    if (resume_timer)
    {
        clearTimeout(resume_timer);
        resume_timer = null;
    }

    /// Let the clients know that the system is no longer paused.
    handler.BroadcastToClients({ type: "pause_status", is_paused: false, resume_time: null });
}
