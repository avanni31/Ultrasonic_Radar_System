# 🛰️ Ultrasonic Radar System

> A real-time object detection radar built on the STM32F303RET6 microcontroller, combining ultrasonic sensing, servo-based angular scanning, I2C LCD feedback, buzzer alerting, and dual visualization via Pygame and a Web Serial dashboard.

---

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Hardware Components](#hardware-components)
- [Pin Configuration](#pin-configuration)
- [System Architecture](#system-architecture)
- [Project Structure](#project-structure)
- [Working Principle](#working-principle)
- [Software Design](#software-design)
- [Getting Started](#getting-started)
- [Visualization](#visualization)
- [Observations & Results](#observations--results)
- [Challenges & Solutions](#challenges--solutions)
- [Future Enhancements](#future-enhancements)
- [Team](#team)
- [References](#references)

---

## Overview

This project implements a compact, low-cost radar system using an STM32F303RET6 (ARM Cortex-M4) microcontroller. An HC-SR04 ultrasonic sensor is mounted on an MG90 servo motor that continuously sweeps 0°–180°, capturing distance readings at each angular step. The system provides:

- Real-time distance display on a 16×2 I2C LCD
- Auditory proximity alerts via a piezoelectric buzzer
- Angle–distance data streamed over UART for PC-side visualization
- A Pygame-based glowing radar visualizer and an HTML5 Web Serial dashboard

The result is a functional, radar-style prototype that demonstrates practical embedded-system design — ultrasonic sensing, PWM servo control, I2C communication, interrupt-driven input capture, and non-blocking firmware architecture — all on a single MCU.

---

## Features

- **180° Angular Scanning** — MG90 servo sweeps in smooth incremental steps
- **Accurate Distance Measurement** — HC-SR04 with TIM1 Input Capture (microsecond precision)
- **Non-Blocking Firmware** — State-machine loop; no `HAL_Delay` in main logic
- **Real-Time LCD Feedback** — Distance and detection status via I2C (PCF8574 backpack)
- **Proximity Buzzer Alert** — Activates when object detected within 20 cm
- **UART Data Streaming** — Angle–distance pairs transmitted at 9600 baud for PC visualization
- **Dual Visualizers** — Pygame desktop radar + HTML5 Web Serial dashboard

---

## Hardware Components

| Component | Role |
|---|---|
| STM32F303RET6 (Nucleo-F303RE) | Central MCU — timing, control, processing |
| HC-SR04 Ultrasonic Sensor | Distance measurement (2 cm – 400 cm) |
| MG90 Servo Motor | 180° angular scanning platform |
| 16×2 LCD (PCF8574 I2C backpack) | Real-time distance and status display |
| Piezoelectric Buzzer | Proximity audio alert |
| ST-Link (on-board) | UART-to-USB (USART2 → Virtual COM Port) |

---

## Pin Configuration

Tested and verified on the **STM32F303RE Nucleo** board:

| Peripheral | Component | MCU Pin | Pin Mode | Details |
|---|---|---|---|---|
| **USART2** | ST-Link Virtual COM | PA2 (TX), PA3 (RX) | Alternate Function | 9600 baud, 8N1 |
| **TIM2** | MG90 Servo Motor | PA0 (PWM) | TIM2_CH1 PWM | 50 Hz, 1–2 ms pulse |
| **TIM1** | HC-SR04 Echo pin | PA8 (Echo) | TIM1_CH1 Input Capture | Interrupt on both edges |
| **GPIO** | HC-SR04 Trig pin | PA9 (Trig) | GPIO Output | 10 µs trigger pulse |
| **I2C1** | 16×2 LCD (PCF8574) | PB8 (SCL), PB9 (SDA) | I2C Alternate Function | 100 kHz Standard Mode |
| **GPIO** | Piezo Buzzer | PB0 (Buzzer) | GPIO Output | Active high alert |

---

## System Architecture

```
┌──────────────────────────────────────────────────────────┐
│                  STM32F303RET6 MCU                       │
│                                                          │
│  TIM2 PWM ──────────────────────► MG90 Servo Motor      │
│  GPIO OUT (PA9) ────────────────► HC-SR04 TRIG           │
│  TIM1 Input Capture (PA8) ◄────── HC-SR04 ECHO           │
│  I2C1 (PB8/PB9) ───────────────► 16×2 LCD (PCF8574)     │
│  GPIO OUT (PB0) ────────────────► Piezo Buzzer           │
│  USART2 (PA2) ──────────────────► ST-Link → PC           │
└──────────────────────────────────────────────────────────┘
                                         │
                              ┌──────────┴──────────┐
                              │                     │
                    radar_visualizer.py        web_dashboard/
                    (Pygame – desktop)     (HTML5 Web Serial)
```

---

## Project Structure

```
.
├── .gitignore
├── README.md
├── Firmware/
│   └── Core/
│       ├── Inc/
│       │   ├── main.h
│       │   ├── lcd_i2c.h          # I2C LCD 16×2 driver
│       │   ├── ultrasonic.h       # HC-SR04 driver (TIM1 Input Capture & DWT)
│       │   └── servo.h            # MG90 PWM controller
│       ├── Src/
│       │   ├── main.c             # Main setup, non-blocking state machine loop
│       │   ├── lcd_i2c.c          # I2C LCD driver implementation
│       │   ├── ultrasonic.c       # HC-SR04 input capture configuration
│       │   ├── servo.c            # MG90 PWM servo sweep logic
│       │   └── stm32f3xx_it.c     # ISRs for Echo TIM1
│       └── Drivers/               # Placeholder headers for compilation context
└── Visualizer/
    ├── requirements.txt
    ├── radar_visualizer.py        # Pygame-based glowing radar visualizer
    └── web_dashboard/
        ├── index.html             # Web Serial API dashboard
        ├── styles.css             # Glassmorphism visual styles
        └── app.js                 # HTML5 Canvas radar sweep & serial parser
```

---

## Working Principle

### 1. Servo Scanning
The STM32 generates 50 Hz PWM on TIM2_CH1 (PA0). Pulse width is mapped to angle — 1 ms = 0°, 2 ms = 180°. The servo sweeps from 0° to 180° and reverses, pausing briefly at each step for mechanical stability.

### 2. Ultrasonic Triggering
At each servo position, a 10 µs HIGH pulse is sent on PA9 (TRIG). The HC-SR04 responds by emitting a burst of 40 kHz ultrasonic waves.

### 3. Echo Capture
TIM1 Input Capture (PA8) triggers an interrupt on the rising edge (start) and falling edge (end) of the ECHO pulse. The difference gives echo duration in microseconds.

### 4. Distance Calculation

```
Distance (cm) = (Echo Time µs × 0.0343) / 2
```

Sound speed ≈ 0.0343 cm/µs; divided by 2 for the round trip.

### 5. Output & Alerts
- Distance + angle displayed on LCD via I2C
- Buzzer activates if distance < 20 cm
- `"angle,distance\n"` sent over USART2 at 9600 baud

### 6. Continuous Mapping
The cycle repeats at every angle step, building a real-time 2D radial map of the environment.

---

## Software Design

The firmware is written using the **STM32 HAL framework** and follows a **non-blocking state-machine architecture** — no `HAL_Delay()` in the main loop.

### Modules

| Module | File | Description |
|---|---|---|
| Ultrasonic | `ultrasonic.c/.h` | DWT-based 10 µs trigger; TIM1 dual-edge input capture ISR; distance formula |
| Servo Control | `servo.c/.h` | TIM2 PWM; angle-to-CCR mapping; `HAL_GetTick`-based step timing |
| LCD Driver | `lcd_i2c.c/.h` | PCF8574 I2C backpack; 4-bit HD44780 commands; `lcd_print`, `lcd_set_cursor` |
| Buzzer | `main.c` | GPIO digital output; ON when distance < threshold |
| UART | `main.c` | USART2 DMA/polling; formatted `"angle,distance\n"` output |
| Main Loop | `main.c` | Coordinates all modules; ISR callbacks update shared state |

---

## Getting Started

### Prerequisites

- STM32CubeIDE (or any STM32 HAL-compatible toolchain)
- STM32CubeMX (optional, for re-generating HAL init)
- Python 3.8+ (for Pygame visualizer)
- A modern browser supporting the **Web Serial API** (Chrome/Edge) for the web dashboard

### Firmware

1. Clone the repository:
   ```bash
   git clone https://github.com/<your-username>/ultrasonic-radar-system.git
   cd ultrasonic-radar-system/Firmware
   ```

2. Open in **STM32CubeIDE** → Import existing project → Build & Flash to Nucleo-F303RE.

3. Connect the hardware per the [Pin Configuration](#pin-configuration) table above.

4. Open a serial monitor at **9600 baud** on the Nucleo's virtual COM port to verify angle–distance output.

### Python Visualizer

```bash
cd Visualizer
pip install -r requirements.txt
python radar_visualizer.py
```

Make sure to set the correct COM port inside `radar_visualizer.py` before running.

### Web Dashboard

1. Open `Visualizer/web_dashboard/index.html` in **Chrome or Edge**.
2. Click **Connect** → select the Nucleo's virtual COM port → set baud rate to **9600**.
3. The radar sweep will begin animating in real time.

> **Note:** Web Serial API requires a secure context (localhost or HTTPS) and is supported in Chromium-based browsers only.

---

## Visualization

### Pygame Radar (`radar_visualizer.py`)
- Glowing green sweep line on a dark background
- Detected objects rendered as bright blips with persistence fade
- Angle and distance overlaid as text

### Web Dashboard (`web_dashboard/`)
- HTML5 Canvas radar with animated sweep
- Glassmorphism UI (dark theme, frosted panels)
- Parses `"angle,distance\n"` from Web Serial API in real time
- No server required — runs entirely in the browser

---

## Observations & Results

- Reliable object detection up to **200–250 cm** with stable, repeatable readings across surface types and angles
- Servo motion was smooth and jitter-free under PWM control, ensuring consistent angular positioning
- LCD updated in real time with negligible latency; detection status messages were clearly visible
- No missed detections observed within the effective range during testing
- The non-blocking firmware maintained consistent scanning frequency throughout all test runs

---

## Challenges & Solutions

| Challenge | Solution |
|---|---|
| Accurate echo timing | Used TIM1 hardware Input Capture with dual-edge ISR for microsecond precision |
| Servo jitter | Optimized PWM pulse timing parameters; ensured stable 50 Hz period |
| Power stability / voltage fluctuations | Added proper voltage regulation and decoupling on the power rail |
| Angled sensor causing incorrect readings | Mounted HC-SR04 on a 3D-printed frame for perpendicular alignment |
| Blocking delays disrupting scan rate | Replaced all `HAL_Delay` with `HAL_GetTick`-based non-blocking timing |

---

## Future Enhancements

- **360° Coverage** — Multi-sensor array for full surround detection
- **Advanced Object Classification** — ML algorithms to classify objects by size and shape
- **Wireless Streaming** — Replace UART with ESP8266/ESP32 for Wi-Fi data transmission
- **Improved Visualization** — 3D polar plot with object tracking and history trails
- **Autonomous Response** — Integration with mobile robot platform for obstacle avoidance

---

## Team

> B.Tech ECE — Shiv Nadar Institute of Eminence, Delhi NCR

| Name | Roll Number | Contribution |
|---|---|---|
| Siddharth Saini | 2410110328 | LCD Driver |
| Ajitesh Singh | 2410110033 | I2C & Presentation |
| Avanni Thakur | 2410110091 | Ultrasonic Sensor |
| Gauri Sunil | 2410110135 | Report & Ultrasonic Sensor |
| Sravan Varma Vatshavai | 2410110334 | Servo Motor & UART |
| Anad Sinha | 2410110047 | UART & Python Visualizer |
| Ramneek Singh | 2410110264 | Integration & Testing |

---

## References

1. STM32F303 Reference Manual (RM0316) — STMicroelectronics
2. HC-SR04 Ultrasonic Distance Sensor Technical Datasheet — Elecfreaks
3. TowerPro MG90 Micro Servo Product Specifications — TowerPro
4. Donald Norris, *Programming with STM32* — McGraw-Hill Education
5. STM32 HAL Driver Documentation — STMicroelectronics
