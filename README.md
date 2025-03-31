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

<details>
<summary><strong>Showcase</strong></summary>

- [x] Motion detection with region of interest
- [x] Telegram bot control & alerts
- [x] Live view via WebSockets
- [x] Custom script hooks in Python
- [x] Schedule system and pause/resume logic
- [x] Event-based recording
- [x] Tested with YOLOv5n

TODO
</details>

---

## Why?

TODO
---

## Limitations & Future Work

- Currently only tested on Windows
- The web dashboard works but could use some UI cleanup
- A few more hook examples and tutorials

Todo
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


## Credits

Thank you to the maintainers of the following:
- nlohmann/json
- Asio
- WebSocket++
- base64.hpp by Tobias Locker
