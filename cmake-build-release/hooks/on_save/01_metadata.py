###
"""
import os
import sys
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from openguard_api import *

open_guard = OpenGuard()

video_path = open_guard.get_arg("file")

object_detected = open_guard.get_arg("object")

#Add metadata to the video
#todo {"label": "human", "confidence": "0.95", "timestamp": "2025-02-28 12:00:00"}
"""