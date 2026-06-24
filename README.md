# Ultrasonic Radar System
An intelligent object detection and mapping system built using an **STM32F303RET6** microcontroller, an **HC-SR04** Ultrasonic Distance Sensor, an **MG90** Servo Motor, a **16x2 I2C LCD**, and a active piezoelectric **Buzzer**. 
The system continuously sweeps a $180^\circ$ field of view, detecting obstacles in real-time, displaying distance telemetry on a local LCD display, and sounding audio alarms when objects breach a safety threshold ($20\text{ cm}$). The telemetry data is streamed over UART (USART2) to a PC, which can be visualized using either a Pygame-based vintage radar screen or a modern browser-based Web Serial dashboard.
---
## Repository Structure
```text
├── .gitignore
├── README.md               # Project documentation
├── Firmware/               # STM32 C Firmware
│   └── Core/
│       ├── Inc/
│       │   ├── main.h      # Peripheral definitions and pin map
│       │   ├── lcd_i2c.h   # 16x2 LCD I2C driver
│       │   ├── ultrasonic.h# HC-SR04 ultrasonic distance sensor driver
│       │   └── servo.h     # MG90 PWM servo sweep driver
│       └── Src/
│           ├── main.c      # Setup and non-blocking timing loop
│           ├── lcd_i2c.c   # LCD driver functions
│           ├── ultrasonic.c# HC-SR04 Input Capture time calculation
│           ├── servo.c     # Servo PWM step control
│           ├── stm32f3xx_it.c # TIM1 Echo edge interrupt handler
│           └── system_stm32f3xx.c # System clock configuration
└── Visualizer/             # PC Visualization Dashboards
    ├── requirements.txt    # Python dependencies
    ├── radar_visualizer.py # Pygame Vintage Radar application
    └── web_dashboard/      # Browser-based dashboard (Web Serial API)
        ├── index.html      # Glassmorphic user interface
        ├── styles.css      # CSS styling and animations
        └── app.js          # Canvas rendering, audio sweep synthesis, serial parsing
```
---
## Hardware Architecture & Pin Connections
The components are connected to the **STM32F303RE Nucleo** board as follows:
|
 Peripheral 
|
 Component 
|
 MCU Pin 
|
 Pin Mode 
|
 Details 
|
|
:---
|
:---
|
:---
|
:---
|
:---
|
|
**
USART2
**
|
 ST-Link (Virtual COM) 
|
`PA2`
 (TX), 
`PA3`
 (RX) 
|
 Alternate Function 
|
 9600 baud, 8N1 serial telemetry stream 
|
|
**
TIM2
**
|
 MG90 Servo Motor 
|
`PA0`
 (PWM) 
|
 TIM2_CH1 PWM 
|
 50Hz PWM signal, 1-2 ms pulse width 
|
|
**
TIM1
**
|
 HC-SR04 Echo pin 
|
`PA8`
 (Echo) 
|
 TIM1_CH1 Input Capture 
|
 Interrupt on rising & falling edges 
|
|
**
GPIO
**
|
 HC-SR04 Trig pin 
|
`PA9`
 (Trig) 
|
 GPIO Output 
|
 Sends 10 µs trigger pulses to start reading 
|
|
**
I2C1
**
|
 16x2 LCD (PCF8574) 
|
`PB8`
 (SCL), 
`PB9`
 (SDA) 
|
 I2C Alternate Function 
|
 100 kHz Standard Mode, 4-bit command mapping 
|
|
**
GPIO
**
|
 Piezo Buzzer 
|
`PB0`
 (Buzzer) 
|
 GPIO Output 
|
 High level triggers warning tone when object < 20cm 
|
---
## System Work Flow & Software Design
1. **Ultrasonic Driver**:
   - Sends a $10\mu\text{s}$ pulse to the sensor `TRIG` pin using a high-precision `DWT` (Data Watchpoint and Trace) cycle counter delay.
   - The sensor emits a $40\text{ kHz}$ acoustic burst.
   - The Echo pin goes high. **Timer 1 Input Capture** detects the rising edge, fires an interrupt, and saves the timer counter.
   - On the falling edge of the Echo pin, the interrupt fires again, and the elapsed count is measured.
   - **Distance Calculation**:
     $$\text{Distance (cm)} = \frac{\text{Echo Time }(\mu\text{s}) \times 0.0343}{2}$$
2. **Servo Sweeper**:
   - Timer 2 operates in PWM mode generating a $50\text{ Hz}$ frequency.
   - The compare register value increments/decrements (mapping $0^\circ$ to $180^\circ$ to $1\text{ ms}$ to $2\text{ ms}$ pulse width) in small steps of $1^\circ$ to $2^\circ$.
   - Incorporates a non-blocking check against `HAL_GetTick()` to execute movements every $30\text{ ms}$ without suspending execution, allowing sensors to sample and UART to transmit simultaneously.
3. **Local display & buzzer**:
   - The I2C-expandable LCD displays current angle, distance, and a `"WARN: CLOSE"` flashing tag.
   - If distance drops below $20\text{ cm}$, the buzzer GPIO goes high, sounding an alert.
4. **Serial Telemetry Stream**:
   - Sends formatted packets over `USART2` to the PC: `"angle,distance\n"`. If no obstacle is detected, it sends `"angle,0"` to represent an empty zone.
---
## PC Visualization
You can visualize the sweeps using either the standalone Python script or the web dashboard.
### 1. Python Pygame Radar
An interactive vintage military-radar screen that maps distance readings as glowing trails.
- **Setup**:
  ```bash
  cd Visualizer
  pip install -r requirements.txt
  python radar_visualizer.py
  ```
- **Simulated Mode**: If the hardware is not connected, the script will automatically fallback to simulation mode so you can preview the visualization.
### 2. Browser-Based Web Serial Dashboard
A cutting-edge glassmorphic dashboard that reads from the STM32 serial port directly via the browser (no Python required).
- **Features**:
  - Web Serial API serial connect panel.
  - Interactive Canvas radar sweeper.
  - Synthesized sound sweep (Web Audio API) pitch-matching the scan angle.
  - Alarm sound warnings.
- **How to open**: Open `Visualizer/web_dashboard/index.html` in Chrome or Edge, click **Connect to Radar**, and select the COM port.
