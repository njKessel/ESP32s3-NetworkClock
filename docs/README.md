# ESP32s3-NetworkClock

### ESP32-S3 smart clock with LED display, weather, moon phase, temperature, Google Calendar. Work in progress.
## Overview

This project started as a simple network-connected clock and gradually evolved from an LCD display, to a 7-segment LED, and finally to a custom alphanumeric LED design. My goal is to create a compact, informative display that synchronizes time and data from various online sources, all presented cleanly on minimalist hardware.

The current iteration features a fully custom 12-digit alphanumeric display driven by shift registers, controlled via a rotary encoder and tactile buttons using a custom-built, non-blocking C++ state machine.
### Current Features

  -  NTP Time Synchronization: Automatically pulls the current time over Wi-Fi with configurable Time Zone support.

  -  Custom UI & State Machine: Rotary encoder-driven menu system to navigate between the clock, settings, alarms, and timers.

  -  Stopwatch & Timer: Built-in logic for counting up/down with visual notifications.

  -  Dynamic Auto-Brightness: Uses a TEPT5700 ambient light sensor paired with hardware PWM to smoothly scale display brightness based on the room's lighting.

### Planned Features

  -  Weather updates (via OpenWeatherMap API)

  -  Temperature display (local I2C sensor + web data)

  -  Moon phase indicator

  -  Google Calendar integration (via Apps Script proxy)

## Hurdles & Iterations
### Hardware

The first major hurdle was understanding the displays and how to drive them. I initially used an Adafruit I2C Backpack intended for 4-digit displays. Instead of rewriting the backpack logic, I decided to switch to a fully custom shift-register-based driver for LTP-587HR alphanumeric display modules to support a full 12-digit layout.

This forced me to learn KiCad 9.0, leading to a modular two-board architecture (Mainboard + Display Board) that went through multiple iterations:

  -  Display Revisions 1 & 2: Designed before the physical modules arrived. I initially misread the datasheet and placed the pins incorrectly. Revision 2 corrected the pinout issue and served as the testbed for my initial clock logic.

  -  Display Revision 3: I realized the standard 74HC595 shift registers struggled to source/sink enough current on their own, causing brightness dips when many segments were lit.

  -  Display Revision 4 (Current): Focused on aesthetic perfection and power delivery. I introduced TBD62083A / TBD62783A transistor arrays to drive the segments, completely solving the brightness issue. To achieve a clean look, all SMT driving logic was moved to the back of the PCB. I also integrated the Ambient Light Reader (ALR) on the front panel for accurate environmental tracking and utilized the remaining shift-register outputs to add 3 front-facing status LEDs.

  -  Mainboard Revision 1 (Current): A dedicated logic board that houses the ESP32-S3 and peripherals. It features a fully spec-compliant USB-C implementation (5.1k pull-downs + ESD protection), a PAM8904 piezo driver for clean audio, expansion bay for future sensor expansion, a 74HC165 PISO shift register for a modular navigation pad/button panel, and RC low-pass filters for true hardware debouncing of the rotary encoder.

## Software

Moving from basic time-telling to a multiplexed display with a menu system introduced a new set of software challenges:

  -  Display Ghosting: Cycling through 12 digits fast enough to prevent visible flickering resulted in "ghosting" (adjacent segments staying dimly lit). I solved this by injecting a microscopic 20-microsecond blanking frame between digit multiplexing steps to allow the transistors to drain.

  -  Input Debouncing: Standard software debouncing wasn't fast enough to catch rapid spins on the rotary encoder. As an intermediate solution, I implemented an esp_timer hardware interrupt to poll the encoder pins every 1 millisecond. This ensures zero missed steps for now, but will be replaced by the true hardware debouncing in the first mainboard revision.

  -  Hardware/Software Desyncs: Managing the active state of the hardware LEDs (PWM duty cycles) while navigating the software menus required careful state-machine logic to ensure the clock accurately reverted to its saved settings when operations were canceled.

License

MIT License — see LICENSE for details.
