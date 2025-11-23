# ESP32 DMX Controller

### Overview

This project aims to use a basic ESP32 board to control multiple DMX lighting fixtures wirelessly through a web interface, a la WLED, for less than $15 in parts.

DMX uses a differential signaling method over RS485, so a MAX485 module is used to convert signals from the ESP32's UART pins to DMX-compatible signals.  Youtuber Gadget Reboot seemed to experience no difficulties bitbanging his Arduino's GPIO pins directly into the MAX485 without a UART like the ESP32 has, but presumably that would prove more inconsistent in a project where the CPU was having to deal with WIFI components or otherwise deviating from the strict timing requirements of DMX.

---

### Goal Illustration

![alt text](images/wireless_ESP32_DMX_schematic.png)

---

### proof of concept

![alt text](images/proof-of-concept-1.gif)

---

### basic circuit / wiring

![alt text](images/basic-circuit-1.png)

![alt text](images/XLR_wiring.png)

---

### Parts List

Buy:

- ESP32 Development Board (~$3)

- MAX485 Module (~$3)

Assumed on-hand:

- USB-C Cable and Wall Adapter

- DMX Lighting Fixture(s) + XLR/DMX Cables

- Soldering Iron and/or Jumper Wires + Breadboard

---

## to-do

- [ ] Implement WIFI connectivity for remote control (get the interface to relay DMX commands successfully)

- [ ] Test with multiple DMX fixtures

  - [ ]  Document latency, distance, and reliability concerns

- [ ] Implement more advanced fixture routines and effects (probably will want to use a library for this like [https://github.com/cansik/esp-dmx-max485](https://github.com/cansik/esp-dmx-max485), think more on interfacing with ShowBuddy or FL's DMX output)

- [ ] Optimize code for performance and reliability

- [ ] More consideration for enclosure design, strain relief, and power supply options

- [x] Create basic web interface for controlling DMX fixtures

- [x] Proof-of-concept automated fixture routine

- [x] Wire up basic circuit on breadboard

---

## Resources

- **Gadget Reboot's Arduino-to-DMX Tutorial** [youtube video](https://www.youtube.com/watch?v=4PjBBBQB2m4&pp=ygUZZ2FkZ2V0IHJlYm9vdCBhcmR1aW5vIGRteNgGMg%3D%3D)

- **DMX512 Protocol** [wikipedia article](https://en.wikipedia.org/wiki/DMX512)

- **ZQ02001 25W Moving Head DJ Lights** [Manual](https://manuals.plus/uking-2/uking-zq02001-25w-moving-head-dj-lights-user-instructions#dmx_addressing) (used in proof of concept)

- **ESP32 3-pack** [Amazon link](https://www.amazon.com/AITRIP-ESP-WROOM-32-Development-Microcontroller-Integrated/dp/B0CR5Y2JVD?s=electronics&sr=1-4)

- **MAX485 Module 10-pack** [Amazon link](https://www.amazon.com/ANMBEST-Transceiver-Arduino-Raspberry-Industrial-Control/dp/B088Q8TD4V?s=electronics&sr=1-2)

- **ShowBuddy Tutorial** [youtube video](https://youtu.be/_q0ZyGS0VWQ)

![ESP32 Pinout](images/esp32_pinout.png)

![XLR Wiring](images/XLR2.png)