# ESP32 Examples

A collection of hands-on ESP32 projects built with the Arduino framework, progressing from basic GPIO control to Bluetooth HID devices with OLED feedback.

---

## 📁 Project Overview

| Folder | Description |
|--------|-------------|
| [`ledblink`](./ledblink) | Blink the onboard LED — the classic "Hello World" of embedded systems |
| [`btn_led`](./btn_led) | Control an external LED with a push button (digital input/output) |
| [`btn_oled`](./btn_oled) | Display button state on a 0.96" SH1106 OLED screen |
| [`custom_oled`](./custom_oled) | Render custom text and graphics on the OLED display |
| [`btn_ble`](./btn_ble) | Send button press events over BLE to a connected device |
| [`btn_ble_oled`](./btn_ble_oled) | Combine BLE transmission with real-time OLED status display |
| [`btn_ble_oled_rev1`](./btn_ble_oled_rev1) | Revised version of `btn_ble_oled` with improvements |
| [`ble_keyboard_oled`](./ble_keyboard_oled) | Emulate a Bluetooth HID keyboard; display connection status on OLED |
| [`ble_keyboard_oled_rev1`](./ble_keyboard_oled_rev1) | Refined version of the BLE keyboard with OLED integration |

---

## 🛠️ Requirements

**Hardware**
- ESP32 development board (e.g. ESP32-DevKitC, ESP32-WROOM-32)
- SH1106 0.96" I2C OLED display (for `*_oled` projects)
- Push button + 10kΩ pull-down resistor (for `btn_*` projects)
- Jumper wires, breadboard

**Software**
- [Arduino IDE](https://www.arduino.cc/en/software) 1.8+ or 2.x
- ESP32 board support package — add this URL in *File → Preferences → Additional Boards Manager URLs*:
  ```
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  ```
- Required libraries (install via *Sketch → Include Library → Manage Libraries*):
  - `Adafruit SH1106`
  - `Adafruit GFX Library`
  - `ESP32 BLE Arduino` (included in the ESP32 core)
  - [`ESP32-BLE-Keyboard`](https://github.com/T-vK/ESP32-BLE-Keyboard) (for `ble_keyboard_*` projects)

---

## 🚀 Getting Started

1. Clone the repository:
   ```bash
   git clone https://github.com/bartuyzc/ESP32-Examples.git
   ```
2. Open the desired project folder in the Arduino IDE.
3. Select your board under *Tools → Board → ESP32 Arduino* and the correct COM port.
4. Upload the sketch and open *Serial Monitor* at **115200 baud** if debug output is expected.

---

## 🔌 Wiring

### OLED (I2C)
| OLED Pin | ESP32 Pin |
|----------|-----------|
| VCC      | 3.3V      |
| GND      | GND       |
| SDA      | GPIO 21   |
| SCL      | GPIO 22   |

### Push Button
| Button Pin | ESP32 Pin |
|------------|-----------|
| One leg    | GPIO 15 (or as defined in sketch) |
| Other leg  | GND       |
| 10kΩ resistor | Between GPIO and 3.3V (pull-up) |

> ⚠️ Exact GPIO assignments may vary per project — check the `#define` pins at the top of each `.ino` file.

---

## 📐 Project Progression

```
ledblink → btn_led → btn_oled → custom_oled
                                     ↓
                               btn_ble → btn_ble_oled → btn_ble_oled_rev1
                                                              ↓
                                                   ble_keyboard_oled → ble_keyboard_oled_rev1
```

Each project builds on concepts from the previous one, making this a good step-by-step learning path.

---

## 📄 License

This repository is for educational purposes. Feel free to use and adapt the code for your own projects.

---

*Developed by [bartuyzc](https://github.com/bartuyzc)*
