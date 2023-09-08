# CANtastic

:warning: This is work in progress, and not suitable for use outside of testing and prototyping. Come back later!

_A motorcycle accessory controller with BLE HID, integrating with CAN based bikes._

**Currently, the only motorcycle supported is the Triumph Tiger 1200 Gen 3, but the firmware is designed to support other vehicles too.**

Features:

- **Media control via OEM handlebar controls** - Cantastic sets up a Bluetooth LE HID device, allowing you to activate a voice assistant, play / pause, and skip tracks via OEM handlebar controls.
- **Extend your existing controls** - Mappable GPIO buttons, which coexist with CAN inputs.
- **Button Passthrough** - For application support (eg. DMD2).

Planned:

- **Aftermarket TPMS** - Display tyre pressure information on dash.
- **MOSFET Switching Stage** - Switch accessory circuits on / off via input rules.

Future Ideas:

- **Datalogging** - Record control inputs, brake pressure, RPM, speed, gear, G-force for riding analysis.
- **Proximity + Mesh** - Allow Cantastic units to detect other units and pass information between them. This could have a number of group riding uses, such as 'tail-end-charlie' notifications, accident / breakdown detection, map sync, etc.
- **Virtual ACC** - Output a switched accessory +12v line, with programmable timing rules (eg. remain on 30s after ignition off). With power management.
- **Interconnect System** - Allowing integration with other smart accessories, such as heated equipment, cameras, radar, intercoms, etc.

## Getting Started

### Hardware Requirements

- ESP-IDF >5.1
- ESP32-WROVER / S3-WROOM-1
- CAN Transceiver (TJA1050 et al.)

### Circuit Diagram

### Compile and Flash

## Extending to support other vehicles

## API
