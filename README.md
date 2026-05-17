
<div align="center">
  
# 🔥 GATE GUARDIAN 🔥

### *Your Ultimate Study Companion for GATE CSE Preparation*

[![License: GNU GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Platform](https://img.shields.io/badge/Platform-ESP32-red.svg)](https://www.espressif.com/)
[![Made with](https://img.shields.io/badge/Made%20with-C%2B%2B-orange.svg)](https://github.com/rahul-the-phoenix)
[![GitHub stars](https://img.shields.io/github/stars/rahul-the-phoenix/GATE-GUARDIAN.svg)](https://github.com/rahul-the-phoenix/GATE-GUARDIAN/stargazers)

</div>

---

## 📱 Project Overview

**GATE GUARDIAN** is a feature-rich, ESP32-based smart productivity device designed specifically for GATE CSE aspirants. It combines a real-time clock, study timer, motivational display, and alarm system into a single compact device with an LCD interface and Bluetooth control capabilities.

### ✨ Why GATE GUARDIAN?

> *"Success is an O(1) operation"*

This device isn't just a clock—it's your personal study companion that keeps you motivated, tracks your study sessions, and ensures you never miss your study targets. Built with love for every GATE aspirant who dreams of that IIT tag.

---

## 🎯 Features

| Feature | Description |
|---------|-------------|
| 🕐 **Multi-Mode Display** | Clock (12/24hr), Clock2 (with AM/PM), Alarm, Timer, Motivation, Study Mode |
| 📚 **Smart Study Mode** | Configurable study sessions with automated break reminders |
| 💪 **Motivation Mode** | 600+ GATE-specific motivational messages with 6 animation styles |
| ⏰ **Alarm System** | Set once, persistent storage (EEPROM), auto-repeat |
| ⏱️ **Timer Function** | Countdown timer with pause/resume capability |
| 🌙 **Auto-Brightness** | Day/Night automatic backlight adjustment |
| 🔊 **Audio Feedback** | Click sounds, tick-tock effects, alarm buzzer |
| 📱 **Bluetooth Control** | Full device control via Bluetooth Serial |
| 🔘 **Physical Buttons** | 6-button interface for standalone operation |
| 🌡️ **Environmental** | DHT11 temperature & humidity sensor |
| 💾 **Persistent Storage** | Saves alarm settings across power cycles |

---

## 🎮 Modes Explained

```
┌─────────────┐     ┌──────────────┐     ┌─────────────┐
│  CLOCK MODE │────▶│  CLOCK2 MODE │────▶│ ALARM MODE  │
└─────────────┘     └──────────────┘     └─────────────┘
       ▲                                       │
       │                                       ▼
┌─────────────┐     ┌──────────────┐     ┌─────────────┐
│ STUDY MODE  │◀────│MOTIVATION MOD│◀────│ TIMER MODE  │
└─────────────┘     └──────────────┘     └─────────────┘
```

### 🕐 Clock Mode
- Current time with blinking colon
- Temperature (℃) and Humidity (%) display
- Date with day and month

### 🕑 Clock2 Mode
- 12-hour format with AM/PM
- Full date format (JAN 01 SUN)
- Temperature & Humidity

### ⏰ Alarm Mode
- Set alarm time (HH:MM)
- Persistent storage
- Continuous buzzer until dismissed

### ⏱️ Timer Mode
- Configurable hours, minutes, seconds
- Pause/Resume functionality
- "TIME UP!" notification with buzzer

### 💪 Motivation Mode
- 600+ curated motivational messages
- 6 animation styles (typewriter, scroll, blink, etc.)
- Auto-rotation every 25 seconds

### 📚 Study Mode
- Configurable study duration (0-9 hours + 0/20/30/40 min)
- Configurable break interval (20-90 min)
- Configurable break duration (2-30 min)
- Automatic break reminders with buzzer
- Study progress tracking

---

## 🔧 Hardware Requirements

| Component | Specification |
|-----------|---------------|
| **Microcontroller** | ESP32 (any variant) |
| **Display** | LCD 16x2 with I2C (0x27) |
| **RTC** | DS3231 (I2C) |
| **Sensor** | DHT11 |
| **Bluetooth** | Built-in ESP32 |
| **Backlight Control** | PWM (GPIO5) |
| **Buzzer** | Passive buzzer (GPIO12) |
| **Buttons** | 6x Tactile switches |

### Pin Connections

| Component | Pin | Purpose |
|-----------|-----|---------|
| I2C SDA | 21 | LCD & RTC |
| I2C SCL | 22 | LCD & RTC |
| DHT11 | 4 | Temperature/Humidity |
| Backlight PWM | 5 | LCD brightness |
| Buzzer | 12 | Audio output |
| Mode Select | 32 | Cycle through modes |
| Set/Confirm | 33 | Enter settings/Confirm |
| Increase | 25 | Increment values |
| Decrease | 26 | Decrement values |
| Discard | 27 | Cancel/Quick Settings |

---

## 📡 Bluetooth Commands

Connect via any Bluetooth Serial terminal (password: none, device name: `ESP32_Clock`)

### Basic Commands

| Command | Action |
|---------|--------|
| `home` | Return to clock mode |
| `offme` | System OFF (backlight off) |
| `onme` | System ON |
| `buzzeron` | Enable buzzer |
| `buzzeroff` | Disable buzzer |

### Display Commands

| Command | Action |
|---------|--------|
| `bright(0-1000)` | Set brightness |
| `speed(1-1000)` | Set animation speed |
| `nightbright(0-1000)` | Night brightness level |
| `daybright(0-1000)` | Day brightness level |
| `nightstart(0-23)` | Night mode start hour |
| `morningend(0-23)` | Morning mode end hour |
| `autobright on/off` | Toggle auto-brightness |

### Audio Commands

| Command | Action |
|---------|--------|
| `tickfreq(freq)` | Tick sound frequency |
| `tockfreq(freq)` | Tock sound frequency |
| `tickdur(ms)` | Tick duration in ms |
| `tockdur(ms)` | Tock duration in ms |

### Message Commands

| Command | Action |
|---------|--------|
| `mv(style)` | Start motivation mode (style 2,4,5,6) |
| `text(style)Your@Message` | Display custom message with @ for line break |
| `Your custom message` | Simple text display (centered) |

### Time Commands

| Command | Action |
|---------|--------|
| `SET:HH,MM,SS,DD,MM,YYYY` | Set RTC time |
| `setalarm:HH,MM,SS` | Set alarm |
| `alarmon` | Enable alarm |
| `alarmoff` | Disable alarm |
| `settimer:HH,MM,SS` | Set timer |
| `timerstop` | Stop timer |

---

## 🎨 Display Styles

| Style | Description |
|-------|-------------|
| **1** | Left-aligned static |
| **2** | Centered static |
| **3** | Scrolling marquee |
| **4** | Blinking centered |
| **5** | Typewriter animation |
| **6** | Character-by-character reveal |

---

## 📂 Project Structure

```
GATE-GUARDIAN/
├── GATE_GUARDIAN.ino    # Main Arduino sketch
├── README.md            # This file
└── LICENSE              # GNU GPL v3
```

---

## 🚀 Installation

### Prerequisites

1. **Arduino IDE** (or PlatformIO)
2. **ESP32 Board Package** installed
3. **Required Libraries:**
   - Wire.h (built-in)
   - LiquidCrystal_I2C
   - RTClib
   - DHT sensor library
   - BluetoothSerial (built-in)
   - EEPROM (built-in)

### Library Installation (Arduino IDE)

```
1. Sketch → Include Library → Manage Libraries
2. Install:
   - "LiquidCrystal I2C" by Frank de Brabander
   - "RTClib" by Adafruit
   - "DHT sensor library" by Adafruit
3. Select ESP32 board from Tools → Board → ESP32 Arduino
4. Upload the code
```

### Building

```bash
# Clone the repository
git clone https://github.com/rahul-the-phoenix/GATE-GUARDIAN.git

# Open GATE_GUARDIAN.ino in Arduino IDE
# Select your ESP32 board and port
# Click Upload (Ctrl+U)
```

---

## 🎮 Button Operations

| Button | Short Press | Long Press (1s) |
|--------|-------------|-----------------|
| **Mode Select** | Cycle through modes | - |
| **Set/Confirm** | Enter settings / Confirm | - |
| **Increase** | Increment value | - |
| **Decrease** | Decrement value | - |
| **Discard** | Cancel / Clear alarm / Stop study | Quick Settings Menu |

### Quick Settings Menu
Long-press DISCARD button to access:
- Brightness adjustment
- Buzzer on/off toggle

---

## 🔄 Mode-Specific Operations

| Mode | SET Button Action | Discard Button Action |
|------|-------------------|----------------------|
| **Clock** | - | - |
| **Clock2** | - | - |
| **Alarm** | Enter alarm setup | Disable alarm |
| **Timer** | Enter timer setup | Stop timer |
| **Motivation** | - | Exit to clock |
| **Study** | Configure/Start study | Stop study session |

---

## 📊 Technical Specifications

| Parameter | Value |
|-----------|-------|
| **Power Supply** | 5V USB |
| **Current Consumption** | ~100-200mA (depends on backlight) |
| **Brightness Levels** | 0-1000 (mapped to 0-255 PWM) |
| **PWM Frequency** | 5000 Hz |
| **EEPROM Size** | 8 bytes |
| **Debounce Delay** | 200ms |
| **Auto-brightness Check** | Every hour change |
| **DHT Read Interval** | 2 seconds |
| **Animation Speed Range** | 10-2000ms |

---

## 💡 Motivation Messages Preview

> "Focus On Logic Build The Future"
> 
> "Crack The GATE Reach The IIT Tag"
> 
> "Code Your Way To The Top Tier"
> 
> "Master Discrete Math For The Win"
> 
> "Success Is An O(1) Operation"

*...and 600+ more!*

### Change Default Values

```cpp
// In the source code, modify:
int nightBrightness       = 300;  // Night brightness (0-1000)
int dayBrightness         = 1000; // Day brightness (0-1000)
int nightStartHour        = 21;   // 9 PM
int morningEndHour        = 6;    // 6 AM
int brightnessLevel       = 500;  // Default brightness
bool buzzerEnabled        = true; // Default buzzer state
```

---

## 🐛 Troubleshooting

| Issue | Solution |
|-------|----------|
| LCD not displaying | Check I2C address (run I2C scan), verify connections |
| RTC not working | Check CR2032 battery, verify I2C address (0x68) |
| DHT11 reading fails | Check pull-up resistor, try different pin |
| Bluetooth not connecting | Restart ESP32, check device name "ESP32_Clock" |
| Buttons unresponsive | Check pull-up resistors, debounce delays |
| Alarm not saving | Verify EEPROM initialization, magic byte (0xA5) |

---

## 📈 Future Roadmap

- [ ] Pomodoro technique integration
- [ ] Progress statistics tracking
- [ ] Subject-wise study tracking
- [ ] Export study data via Bluetooth
- [ ] Web-based configuration interface
- [ ] Battery level monitoring
- [ ] External button matrix for more controls
- [ ] RGB LED status indicators

---

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

---

## 📝 License

This project is licensed under the **GNU General Public License v3.0** - see the [LICENSE](LICENSE) file for details.

```
GATE GUARDIAN - Smart Study Companion for GATE CSE Aspirants
Copyright (C) 2024 RAHUL MANNA (rahul-the-phoenix)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
```

---

## 👨‍💻 Author

**RAHUL MANNA** (rahul-the-phoenix)

- 🔭 GATE CSE 2027 Aspirant
- 💻 Embedded Systems Enthusiast
- 🎯 Goal: IIT with AIR < 100

---

## 🙏 Acknowledgments

- All GATE aspirants who never give up
- Open source community for amazing libraries
- The ESP32 team for this powerful microcontroller

---

## 📞 Connect

<div align="center">
  
[![GitHub](https://img.shields.io/badge/GitHub-rahul--the--phoenix-181717?style=for-the-badge&logo=github)](https://github.com/rahul-the-phoenix)
[![GATE](https://img.shields.io/badge/Target-GATE%202027-red?style=for-the-badge)]()

</div>

---

<div align="center">
  
### *"Rank Under 100 Is The Target — Don't Stop Till You Reach IIT"*

**Made with ❤️ for every GATE Warrior**

</div>
