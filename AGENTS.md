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

In BT-LE terminology, the **T5M4R-ARD** Arduino device is the *peripheral* and the web app is the *central*.

### WEB-APP-REQ-01 – Scan for BLE devices ✅
- Scan for BLE devices, focusing on "T5M4R-ARD" device name + service UUID
- **Status**: Implemented. Uses `requestDevice({ filters })` with name + service UUID filters

### WEB-APP-REQ-02 – GATT connection ✅
- Connect using Web Bluetooth GATT API to the service UUID from `ard-app.ino`
- **Status**: Implemented. Discovers 3 write-only characteristics: Command, NavOrigin, NavDestination

### WEB-APP-REQ-03 – Log service metadata ✅
- List service metadata and characteristics after connection, log to `#log` div
- **Status**: Implemented. `logGattServerDetails()` enumerates all services, characteristics, and properties

### WEB-APP-REQ-04 – Bootstrap 5.3 UI ✅
- Use Bootstrap 5.3 for styling, button states, progress indicators
- **Status**: Implemented. Cards, badges, spinners, `updateUiState()` for enable/disable states

### WEB-APP-REQ-05 – Manage permitted devices ✅ (new)
- Display/select previously-permitted devices, support forgetting devices
- **Status**: Implemented. `populateBluetoothDevices()`, device dropdown, Forget button

### WEB-APP-REQ-06 – Command transmission ✅ (new)
- Send command strings via write-only Command characteristic
- **Status**: Implemented. Command input + Send button, writes to `commandCharacteristic`

### WEB-APP-REQ-07 – Navigation origin/destination ✅ (new)
- Send navigation origin/destination via dedicated characteristics
- **Status**: Implemented. Origin/Destination inputs, `onSendNavigationButtonClick()`

### WEB-APP-REQ-08 – Advanced BLE config ✅ (new)
- Allow Service UUID override for testing/firmware variants
- **Status**: Implemented. Collapsible "Advanced BLE configuration" section

### WEB-APP-REQ-09 – Chrome flags guidance ✅ (new)
- Display Web Bluetooth availability check and Chrome flag URLs (mobile-friendly)
- **Status**: Implemented. Copyable `<code>` blocks for flag URLs

### WEB-APP-REQ-10 - Chrome for Android installability as a Web App/PWA ✅
- Implement Chrome for Android installability as a Web App/PWA, ask the developers questions about data fields and feature flags while implementing: https://web.dev/articles/install-criteria
- Configure hooks such as `beforeinstallprompt` so that in-browser visitor can be prompted to install the app: https://web.dev/articles/codelab-make-installable
- **Status**: Implemented. `manifest.json` with required fields, `sw.js` service worker, `beforeinstallprompt` handler with "Install App" button

### Not implemented
- **Auto-reconnection**: User must manually reconnect after disconnect
- **Response handling**: No acknowledgement from peripheral at Web Bluetooth layer
