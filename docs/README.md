# ESP32s3-NetworkClock
ESP32-S3 smart clock with LED display, weather, moon phase, temperature, Google Calendar. Work in progress.

## Overview  

This project started as a simple network-connected clock and gradually evolved from LCD display to 7-segment LED to alphanumeric LED design. My goal is to create a compact, informative display that synchronizes time and data from various online sources all presented cleanly on minimalist hardware.

Eventually, I plan to add:  
- **Weather updates** (via an online API)  
- **Temperature display** (local sensor + web data)  
- **Moon phase indicator**  
- **Google Calendar integration** 

## Hurdles

FIrst major hurdle has been understanding the display im using and how sending data over I2C works. The Adafruit Backpack that I was using before is only intended for 4-digit displays. Instead of simply using this, I wanted to build my own version of it for 8 digit displays. Instead of rewriting the backpack, I decided to switch to a fully custom shift-register based driver for LTP-587HR alphanumeric display modules, for a full 12 digit display. This let me learn KiCad 9.0, and I designed 3 revisions of a PCB for this. My first revision I made before I had the display modules, and I misread the datasheet, which resulted in me placing the pins for the display modules incorrectly. To remedy this I spun a second revision that corrected the issue. This revision was funtional, and is what I used in the initial stages of clock logic. I then moved to using transistor arrays to drive the segments to have better control of the brightness + prevent it dipping when many segments are lit, as the shift registers (74HC595) struggled to drive them on their own. This resulted in revision 3 of the PCB that included these transistor arrays and an overall better PCB design (closer segments +more compact PCB).

## License  

MIT License – see LICENSE for details.
