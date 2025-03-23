import { config, SaveConfig } from "./config_manager.js";
import { Log } from "./logger.js";

let schedule = {};
let schedule_checker = null;

function LoadSchedule()
{
    if (!config.schedule)
    {
        /// If there is no schedule in the config, create a new one.
        schedule =
        {
            Sunday: [],
            Monday: [],
            Tuesday: [],
            Wednesday: [],
            Thursday: [],
            Friday: [],
            Saturday: []
        };

        config.schedule = schedule;

        /// Write it to the config file.
        SaveConfig(config);
    }
    else
    {
        /// Otherwise, just load the schedule from the config.
        schedule = config.schedule;
    }

    return schedule;
}


function SaveSchedule(new_schedule)
{
    /// Simply save the new schedule to the config file.
    schedule = new_schedule;
    config.schedule = new_schedule;
    SaveConfig(config);
}


/**
 * Applies the schedule to the current state.
 */
/**
 * Applies the schedule once to determine if we need to pause.
 * No timers, no loop — just one-shot logic.
 */
function ApplySchedule(current_state, handle_pause)
{
    /// Get the current time and day.
    const now = new Date();
    const day = now.toLocaleString("en-US", { weekday: "long" });
    const curr_unix_time = Math.floor(now.getTime() / 1000);

    /// Does the current day has a schedule ?
    if (schedule[day])
    {
        for (const block of schedule[day])
        {
            /// Convert block start and end to full timestamps.
            const start = new Date(now);
            start.setHours(block.start, 0, 0, 0);
            const start_unix = Math.floor(start.getTime() / 1000);

            /// Same for the end.
            const end = new Date(now);
            end.setHours(block.end, 0, 0, 0);
            const end_unix = Math.floor(end.getTime() / 1000);

            /// If we're inside the block, pause for the remaining time
            if (curr_unix_time >= start_unix && curr_unix_time < end_unix)
            {
                const duration_seconds = Math.max(10, end_unix - curr_unix_time);

                if (!current_state().is_paused)
                {
                    handle_pause(duration_seconds);
                    Log("⏸️", "Scheduler", `Pausing for ${duration_seconds} seconds.`);
                }

                break;
            }
        }
    }
}



export { LoadSchedule, SaveSchedule, ApplySchedule };
