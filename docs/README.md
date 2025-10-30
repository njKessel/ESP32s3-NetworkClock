# ESP32s3-NetworkClock
ESP32-S3 smart clock with LED display, weather, moon phase, temperature, Google Calendar. Work in progress.

## Overview  

This project started as a simple network-connected clock and gradually evolved from an LCD display to a 7-segment LED design.  
My goal is to create a compact, informative display that synchronizes time and data from various online sources — all presented cleanly on minimalist hardware.

Eventually, I plan to add:  
- **Weather updates** (via an online API)  
- **Temperature display** (local sensor + web data)  
- **Moon phase indicator**  
- **Google Calendar integration**  
- **Optional VFD display upgrade** for a more refined look <- Likely necessary for legible alphanumeric display

## Hurdles

FIrst major hurdle has been understanding the display im using and how sending data over I2C works. The Adafruit Backpack that I was using before is only intended for 4-digit displays. Instead of simply using this, I want to build my own version of it for 8 digit displays.

---

## License  

MIT License – see LICENSE for details.
