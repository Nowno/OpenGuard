### BLOCKING COOLDOWN 120
from openguard_api import *

open_guard = OpenGuard()

open_guard.add_output("message", "Hello from Python!")

open_guard.send_telegram_message("Hello from Python!")

### Example for later, check if user is home(connected to wifi for example) or this can be used to turn on the light to help detect the object

open_guard.return_output()
