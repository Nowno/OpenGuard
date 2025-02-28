import os
import sys
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from openguard_api import *

open_guard = OpenGuard()

error_message = open_guard.get_arg("error")

open_guard.send_telegram_message("🚨 OpenGuard has encountered a fatal error: " + error_message)