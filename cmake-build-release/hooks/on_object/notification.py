### COOLDOWN 15
import os
import sys
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from openguard_api import *

open_guard = OpenGuard()

detected_object = open_guard.get_arg("object")

out_message = ""

match detected_object:
    case "person":
        out_message = "👤 Person detected"
    case "car":
        out_message = "🚗 Dad came back home!"
        open_guard.add_output("cancel", "true")
    case "pet":
        out_message = "🐾 Looks like Minette is awake!"
        open_guard.add_output("cancel", "true")
    case _:
        out_message = "❓ Unknown object detected!"


if detected_object:
    out_message = "🚨 Motion detected at " + open_guard.unix_to_human(int(open_guard.get_call_time())) + " - " + out_message

open_guard.send_telegram_message(out_message)

open_guard.add_output("success", "true")

open_guard.return_output()