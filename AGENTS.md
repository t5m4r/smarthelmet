# AGENTS.md - Smart Helmet Project

## Build/Run Commands

- **Web app**: Open `index.html` and `get-devices.js` in Chrome with Web Bluetooth enabled (see the settings flags to enable in `index.html`)

- **Arduino app**: Use the official Arduino IDE to compile and upload `ard-app/ard-app.ino` and associated files.


## Architecture
- **Single-page Web App** : Chrome-only Single Page App that uses the semi-standardized  [Web Bluetooth Web API.](https://developer.mozilla.org/en-US/docs/Web/API/Web_Bluetooth_API) Two source files: `index.html`, `get-devices.js`.
- **Arduino Firmware** (`ard-app/`): Arduino UNO R4 WiFi with BLE + WiFi for navigation
  - `ard-app.ino` - Main entry, BLE service setup, WiFi connection
  - `01_bt-le-helpers.ino` - BLE event handlers
  - `02_httpnetworking.ino` - WiFi/network utilities
  - `03_map_navigation.ino` - Google Maps API integration
  - `arduino_secrets.h` - WiFi credentials, API keys (gitignored)

## Code Style
- **JS**: Vanilla ES6+, Promise chains (`.then()`), global functions, `camelCase`
- **Arduino**: C++, `camelCase` for functions, `kPascalCase` for constants
- BLE UUIDs: uppercase hex with dashes (e.g., `19B10001-E8F2-537E-4F6C-D104768A1225`)
- Error handling: Log with `log('Argh! ' + error)` pattern in JS that gets appended to the `div` with id `log` in the `index.html` file.

## Product requirements

In BT-LE terminology, the "T54MR-ARD" Arduino device is the "peripheral" and the web app is the "central".

1. WEB-APP-REQ-01
  - Functional: Scan for BLE devices, with focus on the BT device called "T54MR-ARD" Arduino device as that's what this web app wants to talk to. If BT-LE association by hostname is not possible, use the service UUID to connect, see WEB-APP-REQ-02 below for how to know the service UUID for this project. 

2. WEB-APP-REQ-02
  - Connect using the GATT-related Web Bluetooth API classes to connect to the service UUID hard-coded in `ard-app.ino`, and hence copied to `index.html` or `get-devices.js` (wherever appropriate).

3. WEB-APP-REQ-03
  * List the service-related metadata, and characteristic(s) associated with the service after service discovery and/or connection establishment. These should be logged to the `log` div in the `index.html` file. Enumerate all properties and characteristics that the Bluetooth Web API allows as per the [Mozilla Developer Network docs](https://developer.mozilla.org/en-US/docs/Web/API/Web_Bluetooth_API)

4. WEB-APP-REQ-04
  - Use Bootstrap v.53 CSS and JS to make buttons and other UI elements look nice and state transitions be reflected in the control enablement/disablement or perhaps a progress bar: https://getbootstrap.com/docs/5.3/getting-started/introduction/
