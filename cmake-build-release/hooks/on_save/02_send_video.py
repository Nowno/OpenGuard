import os
import sys
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from openguard_api import *

open_guard = OpenGuard()

video_path = open_guard.get_arg("file")

open_guard.send_telegram_video(video_path)

open_guard.add_output("success", "true")

open_guard.return_output()
