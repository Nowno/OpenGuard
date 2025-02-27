import json
import sys
import requests
import os
import time

class OpenGuard:
    # These two will be written to by the setup script to avoid reading them on every hook call
    TELEGRAM_TOKEN = None
    TELEGRAM_CHAT_ID = None

    def __init__(self):
        """ Initialize OpenGuard API and parse JSON arguments from C++ """
        self.output = {}
        if len(sys.argv) < 2:
            self.add_output("error", "No JSON input received by OpenGuard API.")
            self.data = {}
        else:
            try:
                self.data = json.loads(sys.argv[1])
            except json.JSONDecodeError:
                self.add_output("error", "Invalid JSON received by OpenGuard API.")
                self.data = {}

    def add_output(self, key, value):
        """ Add to the output that will be returned to C++ """
        self.output[key] = value

    def get_event(self):
        """ Returns the event name that triggered the hook """
        return self.data.get("event", "unknown_event") # defaultsq to unknown

    def get_call_time(self):
        """ Returns the time the hook was called """
        return self.data.get("time", None)

    def get_arg(self, arg):
        """ Returns the value of a specific argument """
        return self.data.get(arg, None)

    def send_telegram_message(self, message):
        """ Sends a message through Telegram """
        if not OpenGuard.TELEGRAM_TOKEN or not OpenGuard.TELEGRAM_CHAT_ID:
            self.add_output("error", "Telegram not configured properly.")
            return

        url = f"https://api.telegram.org/bot{OpenGuard.TELEGRAM_TOKEN}/sendMessage?chat_id={OpenGuard.TELEGRAM_CHAT_ID}&text={message}"
        try:
            response = requests.get(url)
            if response.status_code != 200:
                self.add_output("error", f"Failed to send Telegram alert. Status Code: {response.status_code}")
        except Exception as e:
            self.add_output("error", f"Telegram alert failed: {e}")

    def send_telegram_video(self, video_path):
        """ Sends a recorded video through Telegram """
        if not OpenGuard.TELEGRAM_TOKEN or not OpenGuard.TELEGRAM_CHAT_ID:
            self.add_output("error", "Telegram not configured properly.")
            return
        if not os.path.exists(video_path):
            self.add_output("error", f"Video file not found: {video_path}")
            return

        url = f"https://api.telegram.org/bot{OpenGuard.TELEGRAM_TOKEN}/sendVideo?chat_id={OpenGuard.TELEGRAM_CHAT_ID}"
        try:
            with open(video_path, "rb") as video:
                files = {"video": video}
                response = requests.post(url, files=files)
                if response.status_code != 200:
                    self.add_output("error", f"Failed to send Telegram video. Status Code: {response.status_code}")
        except Exception as e:
            self.add_output("error", f"Telegram video failed: {e}")


    def pause_system(self, seconds):
        """ Pause motion detection for a specified number of seconds """
        self.add_output("sleep_call", seconds)
        sys.exit(0)

    def log(self, message, level="INFO"):
        """ Add a message to be logged by the native logger """
        self.add_output("log", f"{level}: {message}")

    def return_output(self):
        """ Returns the output dictionary as a JSON string """
        print(json.dumps(self.output))
