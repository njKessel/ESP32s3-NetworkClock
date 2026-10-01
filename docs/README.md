# ESP32-S3 Network Clock

A modular, network-connected smart clock built around the ESP32-S3, featuring a fully custom 12-digit alphanumeric LED display. The system utilizes a custom non-blocking C++ state machine controlled by a rotary encoder and a custom button panel to manage timekeeping, live API data, and local I2C sensors.

For a deep dive into the hardware design, iterations, and software challenges, [read the full write-up on my website](https://nathanielkessel.com/projects/clock.html).

## Features

* **Custom 12-Digit Display:** Shift-register driven (74HC595 + TBD62083A transistor arrays) multiplexed in software with precision 20μs blanking intervals to eliminate ghosting.
* **Time & Environment:** NTP time synchronization, OpenWeatherMap API integration, and local temperature sensing (TMP275 via I2C).
* **State Machine UI:** Rotary encoder-driven menu system for configuring alarms, timers, a stopwatch, and on-device Wi-Fi setup via a custom scrollable keyboard.
* **Dynamic Auto-Brightness:** TEPT5700 ambient light sensor tied to hardware PWM for seamless room-lighting adaptation.
* **Contextual Status LEDs:** Dedicated indicators dynamically reflect Wi-Fi connectivity, active timers, and alarms scheduled within the next hour.

## Planned Features

* Google Calendar integration (via Apps Script proxy)
* Moon phase calculations

## Hardware Architecture

The project is split across a modular three-board architecture designed in KiCad 9.0 + 10.0:

* **Display Board (Rev A):** Houses the LTP-587HR alphanumeric modules. Utilizes TBD62083A / TBD62783A transistor arrays to source/sink enough current for uniform brightness across all segments. All SMT logic is routed on the back for a clean aesthetic. (note: there were previous versions of this PCB for breadboard testing)
* **Mainboard (Rev B):** The core logic board featuring the ESP32-S3, a spec-compliant USB-C implementation (5.1k pull-downs + ESD protection), a PAM8904 piezo driver, a 74HC165 PISO shift register for the button panel, and RC low-pass filters for hardware debouncing.
* **Button panel (Rev A):** Very simple 7-button panel for user interaction.

## Versioning Philosophy

This project uses a custom adaptation of Semantic Versioning tailored for embedded hardware: `Hardware.Firmware.Patch+Year` (e.g., `v1.0.0+2026`).

* **Major:** First digit indicates major changes. This includes hardware changes and swapped APIs
* **Minor:** Second digit indicates backwards-compatible firmware feature additions 
* **Patch:** Third digit indicates small changes and fixes in firmware
* **Metadata:** Release year.

## AI Usage

LLMs (primarily Google Gemini 3.1 Pro) were used in different parts of this project, especially towards the end. This was primarily for rapid feature implementation and repetitive tasks (for one example the time zone table). Hardware (PCBs and component choices), system architecture (state machine, logic, etc...), and validation was strictly human. This approach allowed me to rapidly build out the feature set towards the end of v1.0.0+2026.

## License

MIT License | see `LICENSE` for details.
