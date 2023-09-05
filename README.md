# Cantastic

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
- :warning: **CAN Nullifier** - Nullify control inputs, so controls can be fully repurposed. Eg. Stop OEM joystick affecting dashboard settings in certain modes, and allow us to repurpose the control for other uses. See CAN Nullifier section in this readme.

## Getting Started

### Hardware Requirements

- ESP-IDF >5.1
- ESP32-WROVER / S3-WROOM-1
- CAN Transceiver (TJA1050 et al.)

### Circuit Diagram

### Compile and Flash

## Extending to support other vehicles

## CAN 'Nullifier'

**This is just an idea, and a place to store my thoughts. It may not be possible at all. I will update this section with findings once I test.**

> :warning: This is experimental, and does some naughty things. In certain applications, it could cause safety-critical systems to bus-off, resulting in possible loss of control, **injury** or **death**.

The purpose of CAN 'Nullification' is to nullify control inputs, so we can stop the dashboard reacting to button presses, and use those buttons for another purpose.

The idea behind this is fairly simple: If we detect a button press, we copy the CAN packet and send the inverse of the control that is to be inhibited. We leave the rest of the packet intact.

We only transmit this 'nullifier' packet in single-shot mode, as we expect we may cause a bus error, and do not want to push any OEM device off the bus via failed arbitration. In reality, if the CAN packet we are sending is being transmitted at a fairly low frequency, we are unlikely to cause a collision, but we must expect that we could collide, and must not force the OEM sender into error passive or bus-off.

### Sending the 'inverse'?

More research needs to be done on this, and is likely to be vehicle specific. If the reciever (eg. dashboard) effectively tolerates high-frequency noise (ie. the button is low-pass filtered / debounced), we can just quickly send a `0` in response to a `1` to nullify the input. I think this is unlikely.

The other approach is to send an inverse control press - this will only work for certain directional controls like the joystick. If we see a `Joystick Left` bit, we immediately send `Joystick Right`. The hope is that this will cancel out the original input.

## API
