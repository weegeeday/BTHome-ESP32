# BTHome ESP32 Multi-Sensor & Controller Node

This PlatformIO project configures an **ESP32** to act as a **BTHome V2 BLE Sensor & Controller Node**, automatically recognized by **Home Assistant** and other BTHome BLE integrations.

---

## 📁 Project Structure

```
BTHome-ESP32/
├── platformio.ini         # PlatformIO project configuration & dependencies
├── README.md              # Project documentation & setup guide
├── include/
│   ├── config.h           # Central pin definitions, device configs & BLE settings
│   ├── bthome_encoder.h   # BTHome V2 advertising packet builder header
│   ├── door_sensor.h      # Door switch input driver header
│   ├── light_control.h    # Light relay output driver header
│   └── nfc_pn532.h        # PN532 I2C NFC module driver header
└── src/
    ├── main.cpp           # Main loop & NimBLE BLE controller
    ├── bthome_encoder.cpp
    ├── door_sensor.cpp
    ├── light_control.cpp
    └── nfc_pn532.cpp
```

---

## ⚙️ Configuration Guide (`include/config.h`)

All hardware pins, BLE settings, and device definitions are managed in [`include/config.h`](file:///c:/Users/simon/BTHome-ESP32/include/config.h).

### 1. Global BLE Settings

| Setting | Type | Description |
| :--- | :--- | :--- |
| `DEVICE_NAME` | `const char*` | BLE Bluetooth broadcast name (e.g. `"BTHome-ESP32"`). Discovered by Home Assistant. |
| `BTHOME_ADV_INTERVAL_MS` | `uint32_t` | Regular BLE advertisement interval in milliseconds (default: `3000` ms). |
| `BTHOME_FAST_ADV_INTERVAL_MS` | `uint32_t` | Fast advertisement interval triggered instantly when a sensor state changes or NFC tag is scanned (default: `500` ms). |

---

### 2. Door Sensor Array (`DOOR_SENSOR_CONFIGS[]`)

Supports an **arbitrary (infinite) number of door/window sensors**. Each element in `DOOR_SENSOR_CONFIGS[]` is a `DoorSensorConfig` structure:

```cpp
struct DoorSensorConfig {
    const char* name;       // Descriptive name printed in Serial logs
    uint8_t pin;            // ESP32 GPIO pin connected to reed switch / magnetic contact
    bool activeLow;         // true = LOW pin reading corresponds to Door OPEN (contact open)
    bool usePullup;         // true = enable ESP32 internal pullup resistor (INPUT_PULLUP)
    uint32_t debounceMs;    // Time in ms to filter physical contact switch noise (e.g. 50 ms)
};
```

#### Field Details:
- **`name`**: Friendly name used in serial logs (e.g., `"Front Door"`, `"Garage Door"`).
- **`pin`**: ESP32 GPIO pin number (e.g., `19`, `23`, `15`).
- **`activeLow`**:
  - `true`: Door is considered **OPEN** when the GPIO pin reads `LOW` (0V). (Common when reed switch connects pin to GND when closed).
  - `false`: Door is considered **OPEN** when the GPIO pin reads `HIGH` (3.3V).
- **`usePullup`**: Set `true` to use the ESP32's built-in internal pull-up resistor. No external pull-up resistor is required.
- **`debounceMs`**: Switch debouncing delay. Prevents false state triggers when magnetic contacts vibrate or bounce.

---

### 3. Light Control Array (`LIGHT_CONFIGS[]`)

Supports an **arbitrary (infinite) number of lights/relays**. Each element in `LIGHT_CONFIGS[]` is a `LightConfig` structure:

```cpp
struct LightConfig {
    const char* name;       // Descriptive name printed in Serial logs
    uint8_t relayPin;       // ESP32 GPIO pin connected to relay / MOSFET gate / LED driver
    bool activeHigh;        // true = HIGH pin state turns light ON
    int8_t buttonPin;       // Optional physical toggle push button pin (-1 to disable)
    bool buttonPullup;      // true = enable internal pullup for physical push button
};
```

#### Field Details:
- **`name`**: Friendly name used in serial logs (e.g., `"Living Room Light"`).
- **`relayPin`**: ESP32 GPIO pin driving the relay module or transistor switch (e.g., `18`, `5`).
- **`activeHigh`**:
  - `true`: Writing `HIGH` (3.3V) to `relayPin` turns the light **ON**.
  - `false`: Writing `LOW` (0V) to `relayPin` turns the light **ON** (active-low relay module).
- **`buttonPin`**: GPIO pin connected to a optional physical wall switch or tactile button to toggle the light locally. Set to `-1` if no local physical button is connected.
- **`buttonPullup`**: Set `true` to enable internal pull-up resistor for the push button.

---

### 4. PN532 NFC Module Configuration (`NFC_MODULE_CONFIG`)

Configures a single **PN532 RFID/NFC reader** connected via **I2C**:

```cpp
struct NfcConfig {
    bool enabled;           // Enable/disable the PN532 NFC module
    int8_t sdaPin;          // ESP32 I2C SDA data pin (GPIO 21)
    int8_t sclPin;          // ESP32 I2C SCL clock pin (GPIO 22)
    int8_t resetPin;        // PN532 hardware reset pin (-1 if unconnected)
    int8_t irqPin;          // PN532 hardware IRQ pin (-1 if unconnected)
    uint32_t pollIntervalMs;// Time in ms between NFC tag polling attempts (e.g. 200 ms)
};
```

#### Field Details:
- **`enabled`**: Set `true` to activate PN532 polling; `false` to disable NFC reading.
- **`sdaPin` / `sclPin`**: ESP32 I2C bus pins. Default: `SDA = GPIO 21`, `SCL = GPIO 22`.
- **`resetPin`**: GPIO pin connected to PN532 RST pin for hardware reset. Set to `-1` if RST is tied to ESP32 EN / 3.3V.
- **`irqPin`**: GPIO pin connected to PN532 IRQ pin. Set to `-1` if polling without IRQ line.
- **`pollIntervalMs`**: Delay between card/tag scan checks. `200` ms provides responsive tag detection.

---

### 5. Status LED (`PIN_STATUS_LED`)

- **`PIN_STATUS_LED`**: GPIO pin number for status indication (default: `2` for built-in ESP32 LED). Flashes briefly when BLE packets are broadcasted. Set to `-1` to disable.

---

## 🛠️ How to Add a New Door Sensor or Light

To add a new device, simply open [`include/config.h`](file:///c:/Users/simon/BTHome-ESP32/include/config.h) and add a line to the corresponding array:

### Example: Adding a 4th Door Sensor on GPIO 14
```cpp
const DoorSensorConfig DOOR_SENSOR_CONFIGS[] = {
    {"Front Door",  19, true, true, 50},
    {"Back Door",   23, true, true, 50},
    {"Garage Door", 15, true, true, 50},
    {"Patio Door",  14, true, true, 50},  // <--- New Door Sensor on GPIO 14!
};
```

### Example: Adding a 3rd Light on GPIO 27 with Button on GPIO 32
```cpp
const LightConfig LIGHT_CONFIGS[] = {
    {"Living Room Light", 18, true, 4,  true},
    {"Kitchen Light",     5,  true, -1, true},
    {"Bedroom Light",     27, true, 32, true}, // <--- New Light on GPIO 27 & Button on GPIO 32!
};
```

---

## 🚀 How to Build & Flash

1. Open this workspace folder in **VS Code** with the **PlatformIO** extension installed.
2. Select your board environment (default: `esp32dev` in `platformio.ini`).
3. Connect your ESP32 board via USB.
4. Click **PlatformIO: Build** (or run `pio run`).
5. Click **PlatformIO: Upload** (or run `pio run -t upload`).
6. Open Serial Monitor at **115200 baud**.

---

## 🏠 Home Assistant Integration

1. Enable the **BTHome** integration in Home Assistant (`Settings -> Devices & Services -> Add Integration -> BTHome`).
2. Power on your ESP32 board. Home Assistant will automatically discover **BTHome-ESP32**.
