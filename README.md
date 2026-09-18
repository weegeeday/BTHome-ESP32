# BTHome ESP32 Multi-Sensor & NFC Node

This PlatformIO project configures an **ESP32** to act as a **BTHome V2 BLE Sensor & NFC Node**, automatically recognized by **Home Assistant**, `bthome-scan`, and other BTHome BLE integrations.

---

## 📁 Project Structure

```
BTHome-ESP32/
├── platformio.ini         # PlatformIO project configuration & build flags
├── README.md              # Project documentation & setup guide
├── include/
│   ├── config.h           # Central pin definitions, sensor configs & BLE settings
│   ├── bthome_encoder.h   # BTHome V2 advertising packet builder header
│   ├── door_sensor.h      # Door switch input driver header
│   ├── nfc_manager.h      # High-level PN532 NFC reader component header
│   ├── pn532_cxx/         # Core PN532 C++ driver headers (span, transport, transaction, frontend)
│   └── pn532_hal/         # ESP32 hardware HAL drivers (I2C, SPI, HSU UART)
└── src/
    ├── main.cpp           # Main setup, polling loop & NimBLE BLE controller
    ├── bthome_encoder.cpp
    ├── door_sensor.cpp
    ├── nfc_manager.cpp
    ├── pn532_cxx/         # PN532 frontend & transaction logic
    └── esp-hal-pn532/     # ESP32 I2C hardware driver implementation
```

---

## ⚙️ Configuration Guide (`include/config.h`)

All hardware pins, BLE settings, and device definitions are managed in [`include/config.h`](file:///c:/Users/simon/BTHome-ESP32/include/config.h).

### 1. Global BLE Settings

| Setting | Type | Description |
| :--- | :--- | :--- |
| `DEVICE_NAME` | `const char*` | BLE Bluetooth broadcast name (e.g. `"BTHome-ESP32"`). Discovered by Home Assistant. |
| `BTHOME_ADV_INTERVAL_MS` | `uint32_t` | Regular BLE advertisement interval in milliseconds (default: `3000` ms). |
| `BTHOME_FAST_ADV_INTERVAL_MS` | `uint32_t` | Fast advertisement interval triggered instantly when a door sensor state changes or NFC tag is scanned (default: `500` ms). |

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
  - `true`: Door is considered **OPEN** when the GPIO pin reads `LOW` (0V).
  - `false`: Door is considered **OPEN** when the GPIO pin reads `HIGH` (3.3V).
- **`usePullup`**: Set `true` to use the ESP32's built-in internal pull-up resistor. No external pull-up resistor is required.
- **`debounceMs`**: Switch debouncing delay. Prevents false state triggers when magnetic contacts vibrate or bounce.

---

### 3. PN532 NFC Module Configuration (Optional)

Configures an optional **PN532 RFID/NFC reader** connected via **I2C**:

```cpp
#define ENABLE_PN532_NFC true     // Set to false to completely disable NFC module
#define PN532_I2C_SDA 38          // ESP32 I2C SDA data pin
#define PN532_I2C_SCL 39          // ESP32 I2C SCL clock pin
#define PN532_RESET_PIN -1        // Hardware reset pin (-1 if unconnected)
```

#### Supported Protocols & Tags:
- **FeliCa (212 kbps / 424 kbps)**: Suica, Pasmo, Japanese transit cards & FeliCa-enabled mobile devices.
- **Mifare Classic & ISO 14443-3A**: Standard RFID keyfobs, stickers, and cards.
- **ISO 14443-4 (ISO-DEP)**: Smartphones & smartwatches (Apple Wallet, Google Wallet, Host Card Emulation - HCE).

#### Optional Nature:
- **Compile-time**: Setting `ENABLE_PN532_NFC false` excludes all NFC driver code from compilation.
- **Runtime**: If `ENABLE_PN532_NFC true` but the physical PN532 module is not connected, the node gracefully logs a warning on boot and runs standalone without crashing or blocking.

---

### 4. Status LED (`PIN_STATUS_LED`)

- **`PIN_STATUS_LED`**: GPIO pin number for status indication. Flashes briefly when BLE packets are broadcasted. Set to `-1` to disable.

---

## 📡 BTHome V2 Advertising Format

Broadcasted BLE advertisement payloads comply strictly with the **BTHome V2 Specification** (Service UUID `0xFCD2`, unencrypted header `0x40`):

| Object ID | Name | Size | Format & Value |
| :--- | :--- | :--- | :--- |
| `0x00` | Packet ID | 1 byte | Sequence counter (`0..255`) for deduplication |
| `0x1A` | Door Sensor | 1 byte | `0x00` = Closed, `0x01` = Open |
| `0x2E` | NFC Binary Sensor | 1 byte | `0x00` = No tag present, `0x01` = Tag present |
| `0x0C` | Generic uint32 | 4 bytes | 4-byte Tag UID / IDm (encoded in Little Endian) |
| `0x3A` | Button Event | 1 byte | `0x01` (Press event triggered on tag tap) |

---

## 🛠️ How to Add a New Door Sensor

To add a new device, open [`include/config.h`](file:///c:/Users/simon/BTHome-ESP32/include/config.h) and add an entry to `DOOR_SENSOR_CONFIGS[]`:

```cpp
const DoorSensorConfig DOOR_SENSOR_CONFIGS[] = {
    {"Front Door",  19, true, true, 50},
    {"Back Door",   23, true, true, 50},
    {"Garage Door", 15, true, true, 50},
};
```

---

## 🚀 How to Build & Flash

1. Open this workspace folder in **VS Code** with the **PlatformIO** extension installed.
2. Select your board environment (default: `m5stack-atoms3` in `platformio.ini`).
3. Connect your ESP32 board via USB.
4. Click **PlatformIO: Build** (or run `pio run`).
5. Click **PlatformIO: Upload** (or run `pio run -t upload`).
6. Open Serial Monitor at **115200 baud**.

---

## 🏠 Home Assistant & BLE Scanner Integration

1. Enable the **BTHome** integration in Home Assistant (`Settings -> Devices & Services -> Add Integration -> BTHome`).
2. Power on your ESP32 board. Home Assistant will automatically discover **BTHome-ESP32**.
3. Alternatively, test using the command-line scanner:
   ```bash
   bthome-scan -r -v
   ```
