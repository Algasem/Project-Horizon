# Project Horizon

A custom-built FPV quadcopter with a fully hand-made 2.4 GHz radio link, onboard FPV video, and a tactical heads-up display - designed and built from scratch as a Grade 12 TEJ4M Computer Engineering capstone.

**Authors:** Algasem Zabarah, Zyad Hossameldien
**Course:** TEJ4M Computer Engineering - Capstone 2026
**School:** Earl of March Secondary School

---

## Overview

Project Horizon is a 5-inch FPV quadcopter where every layer of the control chain was custom-built rather than bought off the shelf. Instead of using a commercial radio system, the drone is flown with a hand-soldered controller running a custom Arduino + NRF24L01 protocol, a hand-built receiver, and an ESP32-CAM streaming a live video feed into a custom web-based HUD. The flight controller runs Betaflight, fed through a PWM-to-SBUS converter.

The goal was to demonstrate a complete, controllable, stable hover using a control system built end to end by the team.

## How it works

The control signal flows through five stages:

1. **Pilot Controller (Transmitter)** - An Arduino Nano reads two analog joysticks (throttle, yaw, roll, pitch) and two switches, packs them into a compact 7-byte packet with an integrity checksum, and broadcasts it over an NRF24L01 PA+LNA radio module.
2. **Radio Link** - A one-way 2.4 GHz link at 250 kbps with 16-bit CRC and a custom XOR checksum for packet integrity.
3. **Drone Receiver** - A second Arduino Nano receives and validates each packet, rejecting corrupt ones and holding the last good values through noise bursts, then outputs six standard 1000–2000 µs PWM channels.
4. **PWM-to-SBUS Converter (T1P)** - Converts the receiver's PWM channels into a single SBUS stream for the flight controller.
5. **Flight Controller** - A JHEMCU GF30F405 running Betaflight reads SBUS, runs the PID stabilization loops, and drives the four ESCs and motors.

Separately, an **ESP32-CAM** streams live MJPEG video into a custom tactical HUD web page (radar, telemetry panels, altitude graph, threat overlay) viewable at `http://drone.local`.

## Hardware

| Component | Part |
|---|---|
| Flight controller | JHEMCU GF30F405 (STM32F405), Betaflight |
| Motors | RS2205 2300KV brushless (×4) |
| ESCs | BLHeli 20A (PWM / OneShot125) |
| Props | 5045 tri-blade, props-out |
| Battery | 3S 11.1V 3000mAh LiPo (XT60) |
| Transmitter MCU | Arduino Nano |
| Receiver MCU | Arduino Nano |
| Radio | NRF24L01 PA+LNA (2.4 GHz) on HW-200 adapter |
| PWM→SBUS | T1P converter |
| FPV / HUD | ESP32-CAM |
| Custom PCBs | JLCPCB transmitter board + receiver board |

## Repository contents

| File | Description |
|---|---|
| `TRANSMITTER` | Pilot controller firmware - reads sticks/switches, builds and broadcasts the radio packet |
| `RECEIVER` | Drone receiver firmware - validates packets and outputs PWM channels to the flight controller |
| `CAM HUD SYSTEM` | ESP32-CAM firmware - live MJPEG video stream and the tactical HUD web interface |

## Radio protocol

The transmitter and receiver share an identical configuration, which must match exactly for the link to work:

- **Pipe address:** `0xABCDABCD71`
- **Channel:** 100
- **Data rate:** 250 kbps
- **PA level:** MAX
- **CRC:** 16-bit
- **AutoAck:** OFF (one-way broadcast)

Each packet is 7 bytes: throttle, pitch, roll, yaw, aux1, aux2, and a checksum. The checksum XORs all control bytes together with a constant salt (`0xA5`); the receiver recomputes it and discards any packet that doesn't match.

## Reliability features

Building a radio link from scratch surfaced real engineering problems that the firmware was designed to solve:

- **Checksum packet rejection** - Motor electrical noise (EMI) was corrupting radio packets and causing random disarms. Adding an XOR checksum lets the receiver throw out corrupted packets instead of acting on garbage data.
- **Tiered failsafe** - Short signal gaps are ridden out on the last-known-good values; only a sustained dropout (>1.2 s) triggers a full failsafe that zeroes the throttle.
- **EMI hardening** - The SBUS signal line is twisted with its ground and routed away from the high-current motor wiring to reject motor-induced noise.

## Build and flash

1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. Install the **RF24** library (Library Manager → search "RF24" by TMRh20) for the transmitter and receiver.
3. For the camera, install **ESP32 board support** (Boards Manager → "esp32") and select the AI-Thinker ESP32-CAM board.
4. Open the relevant `.ino` file, select the correct board and port, and upload.
5. The flight controller is configured separately in [Betaflight Configurator](https://betaflight.com/).

## Safety

This is a powerful 5-inch quadcopter with exposed propellers. Always remove props before bench testing, keep clear of the rotor disc when armed, use locking nuts on the motors (props-out can loosen standard nuts), and keep a finger on the disarm switch during every flight.

## Status

Achieves stable, controllable hover. Throttle, yaw, pitch, and roll all respond to the custom controller, arming is switch-controlled, and the EMI-induced disarms have been resolved.

## Acknowledgements

Built by Algasem Zabarah and Zyad Hossameldien for TEJ4M at Earl of March Secondary School, 2026. 
