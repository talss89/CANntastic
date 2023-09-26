# Cantastic

:warning: This is work in progress, and not suitable for use outside of testing and prototyping. Come back later!

_A motorcycle accessory controller with BLE HID, integrating with CAN based bikes._

**Currently, the only motorcycle supported is the Triumph Tiger 1200 Gen 3, but the firmware is designed to support other vehicles too.**

Features (already implemented):

- **Media control via OEM handlebar controls** - Cantastic sets up a Bluetooth LE HID device, allowing you to activate a voice assistant, play / pause, and skip tracks via OEM handlebar controls.
- **Aftermarket TPMS** - Display tyre pressure information on dash using cheap Bluetooth LE tyre-pressure sensors.
- **Extend your existing controls** - Mappable GPIO buttons, which coexist with CAN inputs.
- **Button Passthrough** - For application support (eg. DMD2).

Planned:

- **MOSFET Switching Stage** - Switch accessory circuits on / off via input rules.

Future Ideas:

- **Datalogging** - Record control inputs, brake pressure, RPM, speed, gear, G-force for riding analysis.
- **Proximity + Mesh** - Allow Cantastic units to detect other units and pass information between them. This could have a number of group riding uses, such as 'tail-end-charlie' notifications, accident / breakdown detection, map sync, etc.
- **Virtual ACC** - Output a switched accessory +12v line, with programmable timing rules (eg. remain on 30s after ignition off). With power management.
- **Interconnect System** - Allowing integration with other smart accessories, such as heated equipment, cameras, radar, intercoms, etc.

### A Getting Started guide will follow shortly!

### Project Update

The prototype board is now running on my bike, and seems to work well. It provides tyre pressures to the dash via cheap BLE TPMS sensors as well as phone control via my indicator (turn signal) cancel button. It mounts to the ESP32 S3 Devkit C (which is on the back).

![Prototype 1 PCB](https://github.com/talss89/cantastic/blob/main/prototype-1.jpeg?raw=true)

![Prototype 1 PCB Layout](https://github.com/talss89/cantastic/blob/main/prototype-1-pcb.png?raw=true)

I am also now working on a production prototype board, which will not rely on development boards or power modules. Here's the work-in-progress schematic:

![Prototype 1 PCB Layout](https://github.com/talss89/cantastic/blob/main/r1-schematic-pcb.png?raw=true)