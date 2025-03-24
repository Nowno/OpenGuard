import subprocess
import sys
import time
import json
import os
import requests

logo = """
                        + ++*          
                       +%++++++*       
                        +++++++++++    
                        @#++++++++++@  
                           @%+++@++@+@@
                            ++@%++@+@@ 
 ::::::::::::     ++++++++++++@    @   
::::::::::::::@ ++++++++++++++@        
:::@       :::@ ++++@  @@@@+++@        
:::@       :::@ ++++@      +++@        
:::@       :::@ ++++@      +++@        
:::@       :::@ ++++@      +++@        
:::@       :::@ ++++@                  
:::@       :::@ ++++@                  
:::@       :::@ ++++@                  
:::@       :::@ ++++@  +++++++         
:::@       :::@ ++++@  +++++++@        
:::@       :::@ ++++@      +++@        
:::@       :::@ ++++@      +++@        
:::@       :::@ ++++@      +++@        
:::@       :::@ ++++@      +++@        
:::@       :::@ ++++@      +++@        
:::@       :::@ ++++@      +++@        
::::::::::::::@  +++++++++++++@        
 @@@@@@@@@@@@@    @@@@@@@@@@@@           
"""


def pretty_print(message):
    for char in message:
        print(char, end="", flush=True)
        time.sleep(0.01)


def install_dependencies():
    dependencies = ["requests"]
    for package in dependencies:
        subprocess.run([sys.executable, "-m", "pip", "install", package], check=True)


def load_config():
    default_config = { ... }  # Same as before
    config_path = os.path.join(os.path.dirname(__file__), "config", "config.json")

    try:
        with open(config_path, "r") as file:
            return json.load(file)
    except FileNotFoundError:
        with open(config_path, "w") as file:
            json.dump(default_config, file, indent=4)
            return default_config
    except json.JSONDecodeError:
        print("Invalid JSON in config file.")
        exit(1)


def prompt_config_changes(config, config_path):
    current_python_prefix = config["python_prefix"]
    current_ffmpeg_path = config["ffmpeg_path"]

    pretty_print(f"🐍 Current Python prefix: {current_python_prefix}\n")
    new_prefix = input("🐍 Enter a new Python prefix (i.e. py, python): ")
    if new_prefix:
        config["python_prefix"] = new_prefix

    pretty_print(f"🎥 Current FFmpeg path: {current_ffmpeg_path}\n")
    pretty_print("🎥 FFmpeg is required for video processing. You can download it from https://ffmpeg.org/download.html\n")
    pretty_print("🎥 You may also enter a prefix if already installed\n")
    new_ffmpeg = input("🎥 Enter a new FFmpeg path, or ignore to proceed\n")
    if new_ffmpeg:
        config["ffmpeg_path"] = new_ffmpeg

    with open(config_path, "w") as f:
        json.dump(config, f, indent=4)
    pretty_print("✅ Configuration saved!\n")


def ask_for_bot_token():
    pretty_print("\n🤖 Step 3 - Telegram bot setup\n")
    pretty_print("🤖 Repeating the setup instructions found in the wiki...\n")
    pretty_print("\t1. Create a new bot with BotFather (@BotFather on Telegram)\n")
    pretty_print("\t2. Type /newbot and you will be promped to enter a display name and username for your bot\n")
    pretty_print("\t3. After having provided the two, you will be provided with a bot token\n")
    pretty_print("\t   The message containing the token will look like this: 'Use this token to access the HTTP API: 1234567890:ABCdefGhIjKlMnOpQrStUvWxYz'\n")
    pretty_print("\t4. Copy the token and paste it here\n")
    bot_token = input("🤖 Enter your bot token: ").strip()

    while not bot_token:
        pretty_print("🤖 You must provide a token to proceed\n")
        bot_token = input("🤖 Enter your bot token: ").strip()

    return bot_token


def wait_for_user_message(bot_token):
    request_url = f"https://api.telegram.org/bot{bot_token}/getUpdates"
    prev_user_id = None

    while True:
        try:
            response = requests.get(request_url, timeout=10)
            data = response.json()
            if data.get("ok"):
                for update in data["result"]:
                    if "message" in update:
                        sender = update["message"]["from"]
                        user_id = sender.get("id")
                        if user_id == prev_user_id:
                            continue

                        name = sender.get("first_name", "Unknown")
                        if sender.get("last_name"):
                            name += " " + sender["last_name"]

                        pretty_print(f"🤖 Detected user: {name} (ID: {user_id})\n")
                        confirm = input("✅ Is this you? (y/n): ").strip().lower()
                        if confirm == "y":
                            return user_id, name
                        else:
                            prev_user_id = user_id
                            print("📨 Waiting for a message from you to the bot...\n")
            time.sleep(2)

        except Exception as e:
            print(f"Rrror while polling Telegram: {e}")
            time.sleep(2)


def send_test_message(bot_token, chat_id, user_name):
    msg_url = f"https://api.telegram.org/bot{bot_token}/sendMessage"
    test_msg = f"✅ Hey {user_name}, OpenGuard has been successfully bound to your Telegram!"
    try:
        requests.post(msg_url, json={"chat_id": chat_id, "text": test_msg})
        pretty_print("🤖 Sent test message to your Telegram.\n")
    except Exception as e:
        print(f"⚠️ Failed to send test message: {e}")


def inject_telegram_token(file_path, token, user_id):
    abs_path = os.path.join(os.path.dirname(__file__), file_path)
    try:
        with open(abs_path, "r", encoding="utf-8") as f:
            lines = f.readlines()

        new_lines = []
        token_written = False
        id_written = False

        for line in lines:
            if "TELEGRAM_TOKEN" in line:
                new_lines.append(f'    TELEGRAM_TOKEN = "{token}"  \n')
                token_written = True
            elif "TELEGRAM_CHAT_ID" in line:
                new_lines.append(f'    TELEGRAM_CHAT_ID = "{user_id}" \n')
                id_written = True
            else:
                new_lines.append(line)

        if not token_written or not id_written:
            raise ValueError("Lines not found")

        with open(abs_path, "w", encoding="utf-8") as f:
            f.writelines(new_lines)

        print("✅ Telegram credentials injected into", file_path)

    except Exception as e:
        print("failed to inject tlegram credentials:", e)

def main():
    print(logo)
    pretty_print("👋 Welcome to the OpenGuard setup tool! 🛡️\n")

    config = load_config()
    config_path = os.path.join(os.path.dirname(__file__), "config", "config.json")

    print("\n📦 Step 1 - Dependencies")
    install_dependencies()
    pretty_print("✅ Dependencies installed!\n")

    print("\n📦 Step 2 - Configuration")
    prompt_config_changes(config, config_path)

    bot_token = ask_for_bot_token()
    pretty_print("📨 Waiting for your message...\n")
    user_id, user_name = wait_for_user_message(bot_token)

    config["telegram_bot_token"] = bot_token
    config["telegram_user_id"] = user_id

    try:
        with open(config_path, "w") as file:
            json.dump(config, file, indent=4)
    except Exception as e:
        print(f"Failed to save config: {e}")

    send_test_message(bot_token, user_id, user_name)

    pretty_print("🔧 Injecting Telegram infos into the PY API...\n")
    inject_telegram_token("hooks/openguard_api.py", bot_token, user_id)
    pretty_print("🎉 Telegram setup complete! You're now ready to use OpenGuard.\n")


main()
