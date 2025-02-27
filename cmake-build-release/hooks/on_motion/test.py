### BLOCKING COOLDOWN 120

from openguard_api import *

open_guard = OpenGuard()

open_guard.add_output("message", "Hello from Python!")

open_guard.send_telegram_message("Hello from Python!")

open_guard.return_output()
