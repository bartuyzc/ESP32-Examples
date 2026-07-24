# ESP32 Examples

A collection of hands-on ESP32 projects built with the Arduino framework — progressing from basic GPIO control, to BLE HID devices, to WiFi-connected OLED displays and a mini oscilloscope.

---

## 📁 Project Overview

| Folder | Description |
|--------|--------------|
| [`ledblink`](./ledblink) | Blink the onboard LED — the classic "Hello World" of embedded systems |
| [`btn_led`](./btn_led) | Control an external LED with a push button (digital input/output) |
| [`btn_oled`](./btn_oled) | Display button state on a 0.96" SH1106 OLED screen |
| [`custom_oled`](./custom_oled) | Render custom text and graphics on the OLED display |
| [`btn_ble`](./btn_ble) | Send button press events over BLE to a connected device |
| [`btn_ble_oled`](./btn_ble_oled) | Combine BLE transmission with real-time OLED status display *(includes a `rev1` revision)* |
| [`btn_ble_oled_rev1`](./btn_ble_oled_rev1) | Revised standalone version of `btn_ble_oled` |
| [`ble_keyboard_oled`](./ble_keyboard_oled) | Emulate a Bluetooth HID keyboard; display connection status on OLED *(includes a `rev1` revision)* |
| [`ble_keyboard_oled_rev1`](./ble_keyboard_oled_rev1) | Refined standalone version of the BLE keyboard with OLED integration |
| [`wifi_test`](./wifi_test) | Connect the ESP32 to WiFi and show connection status / IP on the OLED |
| [`wifi_spotify_oled`](./wifi_spotify_oled) | Show the currently playing Spotify track (artist, title, progress) on the OLED via a small HTTP server, fed by a companion Python script *(includes a `rev1` revision)* |
| [`oscilloscope_oled`](./oscilloscope_oled) | Turn the ESP32 + OLED into a mini oscilloscope: ADC sampling via timer ISR, adjustable time/div, volt/div, and trigger level |

`notes.txt` at the repo root has quick-reference notes (Bluetooth pairing commands via `bluetoothctl`, built-in pin assignments, OLED wiring, and a link to the [Lopaka OLED UI designer](https://lopaka.app/)).

---

## 🛠️ Requirements

**Hardware**
- ESP32 development board (e.g. ESP32-DevKitC, ESP32-WROOM-32)
- SH1106/SSD1306 0.96" I2C OLED display (for `*_oled` projects)
- Push button + 10kΩ resistor (for `btn_*` projects)
- Jumper wires, breadboard
- For `oscilloscope_oled`: a signal source within 0–3.3V (use a voltage divider/attenuator for anything larger — do **not** connect directly to mains or high-voltage signals)

**Software**
- [Arduino IDE](https://www.arduino.cc/en/software) 1.8+ or 2.x
- ESP32 board support package — add this URL under *File → Preferences → Additional Boards Manager URLs*:
  ```
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  ```
- Required libraries (via *Sketch → Include Library → Manage Libraries*):
  - `Adafruit SH1106` / `Adafruit SH110X` (used by `oscilloscope_oled`)
  - `Adafruit GFX Library`
  - `ESP32 BLE Arduino` (bundled with the ESP32 core)
  - [`ESP32-BLE-Keyboard`](https://github.com/T-vK/ESP32-BLE-Keyboard) (for `ble_keyboard_*` projects)
  - `ArduinoJson` (for `wifi_spotify_oled`)
- For `wifi_spotify_oled`'s companion script: Python 3, `requests`, `pillow` (see `wifi_spotify_oled/rev1/requirements.txt`), and [`playerctl`](https://github.com/altdesktop/playerctl) on the Linux machine running Spotify

---

## 🚀 Getting Started

1. Clone the repository:
   ```bash
   git clone https://github.com/bartuyzc/ESP32-Examples.git
   ```
2. Open the desired project folder in the Arduino IDE.
3. Select your board under *Tools → Board → ESP32 Arduino* and the correct COM port.
4. Upload the sketch and open the *Serial Monitor* at **115200 baud** if debug output is expected.
5. For WiFi-based projects (`wifi_test`, `wifi_spotify_oled`), edit the `ssid` and `password` constants at the top of the `.ino` file before flashing — **don't commit real credentials to the repo**.
6. For `wifi_spotify_oled`, also set `ESP32_IP` at the top of `spotify_to_esp32.py` to your board's IP (shown on the OLED / Serial Monitor after connecting), then run the companion script from the project folder:
   ```bash
   ./run.sh
   ```
   (creates a venv and installs dependencies automatically on first run).

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
| Button Pin     | ESP32 Pin |
|----------------|-----------|
| One leg        | GPIO 15 (or as defined in sketch) |
| Other leg      | GND       |
| 10kΩ resistor  | Between GPIO and 3.3V (pull-up) |

### Oscilloscope (`oscilloscope_oled`)
| Signal        | ESP32 Pin |
|---------------|-----------|
| Analog input  | GPIO 34 (ADC1, input-only) |
| Time/div ↑ / ↓ | GPIO 32 / GPIO 33 |
| Trigger ↑ / ↓ | GPIO 25 / GPIO 26 |
| Volt/div ↑ / ↓ | GPIO 27 / GPIO 14 |

> ⚠️ Exact GPIO assignments may vary per project — check the `#define` pins at the top of each `.ino` file.

---

## 📐 Project Progression

```
ledblink → btn_led → btn_oled → custom_oled
                                     │
                    ┌────────────────┼───────────────────┐
                    ▼                ▼                    ▼
              btn_ble → btn_ble_oled → btn_ble_oled_rev1   wifi_test → wifi_spotify_oled
                                │                                │
                                ▼                                ▼
                   ble_keyboard_oled → ble_keyboard_oled_rev1   oscilloscope_oled
```

Each project builds on concepts from an earlier one — GPIO → OLED rendering → BLE → WiFi/HTTP → timer-driven ADC sampling — making this a step-by-step learning path.

---

## 📄 License

This repository is for educational purposes. Feel free to use and adapt the code for your own projects.

---

*Developed by [bartuyzc](https://github.com/bartuyzc)*
