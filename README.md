![OpenGuard](https://github.com/Nowno/OpenGuard/blob/master/web/front/config/src/assets/logowide_github.png)

<p align="center">
    A C++ Modular, Real-Time, and User-owned Surveillance System.
</p>

---

## What is OpenGuard?

**OpenGuard** is a real-time home surveillance system written in C++ with a focus on modularity, efficiency, and privacy. It includes:

- Motion detection (MOG2)
- AI object detection (YOLOv5)
- Native + Python hook system for automation
- Real-time recording with metadata & compression
- Web dashboard and Telegram integration

includes a simple Python API to let users script automations without having to dive into the C++ internals.
Windows support is tested and stable. Linux support is in progress, the code is cross-platform by design and just needs some testing.

---

## Screenshots & Demo


<summary><strong>Showcase</strong></summary>

- [x] Motion detection with region of interest
- [x] Telegram bot control & alerts
- [x] Live view via WebSockets
- [x] Custom script hooks in Python
- [x] Schedule system and pause/resume logic
- [x] Event-based recording
- [x] Tested with YOLOv5n

<details>
<summary><strong>Program structure</strong></summary>
<img src="https://github.com/Nowno/OpenGuard/blob/master/web/front/config/src/assets/structure.png?raw=true" alt="Program Structure" />
</details>

---

## Why?

Today's surveillance systems are often cloud-dependent and invasive by offloading footage to third-party servers.
OpenGuard was built to challenge that model.

- Everything runs locally. 
- From the C++ core to the Python hook system, every part is designed to be extended.
- Real-time and efficient.

OpenGuard is designed for users that want total control over their surveillance system.

---

## Limitations & Future Work

- Currently only tested on Windows
- The web dashboard works but could use some UI cleanup
- The web code is not up to my code quality standards as it had to be rushed due to time constraint. 
- A few more hook examples and tutorials (i.e. Trigger lights, check network for mac address to disable the system when user is home, etc..).

Todo
- /!!!\ Encrypt login ws -> OpenGuard.
- Better UI/UX for the panel
- Object-specific fine-tuned YOLO models

---

## Building / Running
Was built with CMAKE and MSVC. Replace OpenCV path in CMAKE to compile.
Release is provided with pre-compiled executable.

to run website.
web/front/config: npm install

web/front/config: npm run dev

web/back: npm install

web/back: node server

py setup.py

## Credits

Thank you to the maintainers of the following:
- nlohmann/json
- Asio
- WebSocket++
- base64.hpp by Tobias Locker
