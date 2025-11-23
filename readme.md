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

### basic circuit

![alt text](images/basic-circuit-1.png)

---

### Parts List

- ESP32 Development Board (~$3)
- MAX485 Module (~$3)
- Jumper Wires/Breadboard (~$5)
- USB-C Cable and Wall Adapter (assumed on-hand)

---

## Resources

- **Gadget Reboot's Arduino-to-DMX Tutorial** [youtube video](https://www.youtube.com/watch?v=4PjBBBQB2m4&pp=ygUZZ2FkZ2V0IHJlYm9vdCBhcmR1aW5vIGRteNgGMg%3D%3D)

- **DMX512 Protocol** [wikipedia article](https://en.wikipedia.org/wiki/DMX512)

- **ZQ02001 25W Moving Head DJ Lights** [Manual](https://manuals.plus/uking-2/uking-zq02001-25w-moving-head-dj-lights-user-instructions#dmx_addressing) (used in proof of concept)

- **ESP32 3-pack** [Amazon link](https://www.amazon.com/AITRIP-ESP-WROOM-32-Development-Microcontroller-Integrated/dp/B0CR5Y2JVD?s=electronics&sr=1-4)

- **MAX485 Module 10-pack** [Amazon link](https://www.amazon.com/ANMBEST-Transceiver-Arduino-Raspberry-Industrial-Control/dp/B088Q8TD4V?s=electronics&sr=1-2)

- **ShowBuddy Tutorial** [youtube video](https://youtu.be/_q0ZyGS0VWQ)