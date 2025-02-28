import os
import sys
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from openguard_api import *

#sence nice message greeting with emoji saying the system is ready
open_guard = OpenGuard()
time = open_guard.unix_to_human(int(open_guard.get_call_time()))
out_message = "🚀 OpenGuard is up and running. Ready to protect your home! 🏡 - " + time

open_guard.send_telegram_message(out_message)

open_guard.add_output("success", "true")
open_guard.return_output()
