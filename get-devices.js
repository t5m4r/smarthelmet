const DEFAULT_SERVICE_UUID = '19B10001-E8F2-537E-4F6C-D104768A1225';
const DEFAULT_COMMAND_CHARACTERISTIC_UUID = '19B10002-E8F2-537E-4F6C-D104768A1214';
const DEFAULT_NAV_ORIGIN_CHARACTERISTIC_UUID = '19B10002-E8F2-537E-4F6C-D104768A1215';
const DEFAULT_NAV_DEST_CHARACTERISTIC_UUID = '19B10002-E8F2-537E-4F6C-D104768A1216';

// Geolocation configuration for navigation use case
const GEOLOCATION_OPTIONS = {
  enableHighAccuracy: true,
  timeout: 5000,
  maximumAge: 30000
};

// ============ SMART THROTTLE CONFIGURATION ============
// Controls when GPS position updates are sent to the Arduino via BLE.
// Both conditions must be met before a BLE write occurs.
const GPS_BLE_THROTTLE = {
  MIN_DISTANCE_METERS: 25,    // Minimum distance moved since last BLE send (meters)
  MIN_TIME_MS: 3000           // Minimum time since last BLE send (milliseconds)
};

// Recent places localStorage config
const RECENT_INDEX_KEY = 'hs_recent_place_ids';
const RECENT_MAX = 8;

let activeDevice = null;
let commandCharacteristic = null;
let navOriginCharacteristic = null;
let navDestCharacteristic = null;
const textEncoder = new TextEncoder();
let isConnecting = false;
let isScanning = false;
let geolocationWatchId = null;
let isTrackingGpsForOrigin = true; // Default to tracking GPS
let lastProcessedGeoTimestamp = 0;
let placesAutocompleteElement = null;
let selectedPlace = null; // Stores the currently selected place from autocomplete

// Smart throttle state for continuous BLE GPS updates
let lastBleSendTimestamp = 0;
let lastBleSendLat = null;
let lastBleSendLng = null;

// Fast-forward timer state
let fastForwardIntervalId = null;

// ============ Haversine Distance Calculation ============

/**
 * Calculate distance between two GPS coordinates using the Haversine formula.
 * @param {number} lat1 - Latitude of first point (degrees)
 * @param {number} lng1 - Longitude of first point (degrees)
 * @param {number} lat2 - Latitude of second point (degrees)
 * @param {number} lng2 - Longitude of second point (degrees)
 * @returns {number} Distance in meters
 */
function haversineDistance(lat1, lng1, lat2, lng2) {
  const R = 6371000; // Earth's radius in meters
  const toRad = (deg) => deg * (Math.PI / 180);
  
  const dLat = toRad(lat2 - lat1);
  const dLng = toRad(lng2 - lng1);
  const a = Math.sin(dLat / 2) * Math.sin(dLat / 2) +
            Math.cos(toRad(lat1)) * Math.cos(toRad(lat2)) *
            Math.sin(dLng / 2) * Math.sin(dLng / 2);
  const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
  
  return R * c;
}

/**
 * Check smart throttle conditions and send GPS to Arduino via BLE if thresholds met.
 * Requires: BLE connected, navOriginCharacteristic available, GPS tracking enabled.
 * @param {number} latitude - Current GPS latitude
 * @param {number} longitude - Current GPS longitude
 */
async function maybeSendGpsToBle(latitude, longitude) {
  // Guard: Must have BLE connection and origin characteristic
  if (!activeDevice?.gatt?.connected || !navOriginCharacteristic) {
    return;
  }
  
  // Guard: GPS tracking toggle must be enabled
  if (!isTrackingGpsForOrigin) {
    return;
  }
  
  const now = Date.now();
  const timeSinceLastSend = now - lastBleSendTimestamp;
  
  // Time threshold check
  if (timeSinceLastSend < GPS_BLE_THROTTLE.MIN_TIME_MS) {
    return;
  }
  
  // Distance threshold check (skip if first send)
  if (lastBleSendLat !== null && lastBleSendLng !== null) {
    const distance = haversineDistance(lastBleSendLat, lastBleSendLng, latitude, longitude);
    if (distance < GPS_BLE_THROTTLE.MIN_DISTANCE_METERS) {
      return;
    }
  }
  
  // Thresholds met - send GPS to Arduino via BLE
  const payload = latitude.toFixed(6) + ',' + longitude.toFixed(6);
  try {
    // Send origin coordinates
    await navOriginCharacteristic.writeValueWithoutResponse(textEncoder.encode(payload));
    
    // Send command to notify Arduino that origin moved
    if (commandCharacteristic) {
      await commandCharacteristic.writeValueWithoutResponse(textEncoder.encode('NAV_ORIGIN_MOVED'));
    }
    
    lastBleSendTimestamp = now;
    lastBleSendLat = latitude;
    lastBleSendLng = longitude;
    log('[A→H] Origin: ' + payload + ' + NAV_ORIGIN_MOVED');
  } catch (err) {
    log('[A→H] Argh! BLE origin write failed: ' + err);
  }
}

// ============ Recent Places localStorage Helpers ============

function saveRecentPlace(place) {
  if (!place || !place.id) return;
  const key = 'hs_place_' + place.id;
  const payload = {
    id: place.id,
    displayName: place.displayName || place.id,
    lat: place.lat,
    lng: place.lng,
    lastUsed: Date.now()
  };
  try {
    localStorage.setItem(key, JSON.stringify(payload));
    
    let index = [];
    try {
      index = JSON.parse(localStorage.getItem(RECENT_INDEX_KEY)) || [];
    } catch {}
    index = index.filter(id => id !== place.id);
    index.unshift(place.id);
    if (index.length > RECENT_MAX) index = index.slice(0, RECENT_MAX);
    localStorage.setItem(RECENT_INDEX_KEY, JSON.stringify(index));
  } catch (e) {
    console.warn('Failed to save recent place:', e);
  }
}

function loadRecentPlaces() {
  let index = [];
  try {
    index = JSON.parse(localStorage.getItem(RECENT_INDEX_KEY)) || [];
  } catch {}
  return index
    .map(id => {
      try {
        const data = JSON.parse(localStorage.getItem('hs_place_' + id));
        return data && data.id ? data : null;
      } catch {
        return null;
      }
    })
    .filter(Boolean);
}

function clearAllRecentPlaces() {
  try {
    const index = JSON.parse(localStorage.getItem(RECENT_INDEX_KEY)) || [];
    index.forEach(id => localStorage.removeItem('hs_place_' + id));
    localStorage.removeItem(RECENT_INDEX_KEY);
  } catch {}
}

function renderRecentPlaces() {
  const container = document.getElementById('recentDestinations');
  if (!container) return;
  container.innerHTML = '';

  const places = loadRecentPlaces();
  places.forEach(p => {
    const btn = document.createElement('button');
    btn.type = 'button';
    btn.className = 'btn btn-outline-secondary btn-sm rounded-pill recent-dest';
    btn.textContent = p.displayName;
    btn.dataset.placeId = p.id;
    btn.dataset.lat = p.lat || '';
    btn.dataset.lng = p.lng || '';
    btn.addEventListener('click', () => {
      onRecentDestinationClick(p);
    });
    container.appendChild(btn);
  });
}

function onRecentDestinationClick(place) {
  const navDestInput = document.getElementById('navDestInput');
  const navDestPlaceId = document.getElementById('navDestPlaceId');
  const navDestDisplayName = document.getElementById('navDestDisplayName');
  
  // Set hidden fields - prefer Place ID (prefixed), fallback to lat,lng
  if (navDestInput) {
    if (place.id && !place.id.startsWith('manual_')) {
      navDestInput.value = 'place_id:' + place.id;
    } else if (place.lat && place.lng) {
      navDestInput.value = place.lat.toFixed(6) + ',' + place.lng.toFixed(6);
    } else {
      navDestInput.value = place.displayName;
    }
  }
  if (navDestPlaceId) navDestPlaceId.value = place.id || '';
  if (navDestDisplayName) navDestDisplayName.value = place.displayName || '';
  
  // Store as selected place for sending
  selectedPlace = place;
  
  // Update recent usage timestamp
  saveRecentPlace(place);
  
  // Show selected destination indicator
  showSelectedDestination(place.displayName);
  
  updateSendButtonState();
  setNavStatus('');
}

// ============ Selected Destination Indicator ============

function showSelectedDestination(displayName) {
  const indicator = document.getElementById('selectedDestIndicator');
  const textEl = document.getElementById('selectedDestText');
  if (indicator && textEl) {
    textEl.textContent = displayName;
    indicator.style.display = 'block';
  }
}

function clearSelectedDestination() {
  const indicator = document.getElementById('selectedDestIndicator');
  if (indicator) indicator.style.display = 'none';
  
  // Clear hidden fields
  const navDestInput = document.getElementById('navDestInput');
  const navDestPlaceId = document.getElementById('navDestPlaceId');
  const navDestDisplayName = document.getElementById('navDestDisplayName');
  if (navDestInput) navDestInput.value = '';
  if (navDestPlaceId) navDestPlaceId.value = '';
  if (navDestDisplayName) navDestDisplayName.value = '';
  
  selectedPlace = null;
  updateSendButtonState();
}

// ============ Google Places UI Kit Autocomplete ============

async function initPlacesAutocomplete() {
  const container = document.getElementById('autocompleteContainer');
  if (!container) {
    log('[A] Autocomplete container not found');
    return;
  }

  try {
    const { BasicPlaceAutocompleteElement } = await google.maps.importLibrary('places');
    
    // Get current GPS location for initial bias (if available)
    const originInput = document.getElementById('navOriginInput');
    const currentCoords = originInput?.value?.trim();
    let initialLocationBias = null;
    
    if (currentCoords) {
      const match = currentCoords.match(/^(-?\d+\.?\d*),\s*(-?\d+\.?\d*)$/);
      if (match) {
        const lat = parseFloat(match[1]);
        const lng = parseFloat(match[2]);
        // Bias within 5km radius of current location
        initialLocationBias = {
          center: { lat, lng },
          radius: 5000
        };
        log('[A] Autocomplete locationBias set to GPS: ' + lat.toFixed(4) + ',' + lng.toFixed(4));
      }
    }
    
    placesAutocompleteElement = new BasicPlaceAutocompleteElement({
      includedPrimaryTypes: ['geocode', 'establishment'],
      locationBias: initialLocationBias,
    });
    
    container.appendChild(placesAutocompleteElement);
    
    // Handle place selection
    placesAutocompleteElement.addEventListener('gmp-select', async (event) => {
      const place = event.place;
      if (!place || !place.id) {
        log('[A] No place selected');
        return;
      }
      
      log('[A] Place selected: ' + place.id);
      
      // Fetch displayName and location for the selected place
      try {
        await place.fetchFields({ fields: ['displayName', 'location', 'formattedAddress'] });
        
        const displayName = place.displayName || place.formattedAddress || 'Unknown place';
        const lat = place.location?.lat();
        const lng = place.location?.lng();
        
        log('[A] Place details: ' + displayName + ' (' + (lat ? lat.toFixed(6) + ',' + lng.toFixed(6) : 'no coords') + ')');
        
        // Update hidden fields
        const navDestInput = document.getElementById('navDestInput');
        const navDestPlaceId = document.getElementById('navDestPlaceId');
        const navDestDisplayName = document.getElementById('navDestDisplayName');
        
        // BLE payload: prefer Place ID (prefixed) for Google Navigation API
        if (navDestInput) {
          navDestInput.value = 'place_id:' + place.id;
        }
        if (navDestPlaceId) navDestPlaceId.value = place.id;
        if (navDestDisplayName) navDestDisplayName.value = displayName;
        
        // Store selected place
        selectedPlace = {
          id: place.id,
          displayName: displayName,
          lat: lat,
          lng: lng
        };
        
        // Save to recents immediately (using placeID as key)
        saveRecentPlace(selectedPlace);
        renderRecentPlaces();
        
        // Show selected destination indicator
        showSelectedDestination(displayName);
        
        updateSendButtonState();
        setNavStatus('');
      } catch (err) {
        log('[A] Argh! Failed to fetch place details: ' + err);
        setNavStatus('Could not load place details');
      }
    });
    
    log('[A] Places Autocomplete initialized');
  } catch (error) {
    log('[A] Argh! Places UI Kit failed to load: ' + error);
    // Fallback: show a plain text input
    container.innerHTML = '<input type="text" id="navDestFallback" class="form-control form-control-lg" placeholder="Enter address (Places API unavailable)">';
    const fallbackInput = document.getElementById('navDestFallback');
    if (fallbackInput) {
      fallbackInput.addEventListener('input', function() {
        const navDestInput = document.getElementById('navDestInput');
        if (navDestInput) navDestInput.value = this.value.trim();
        selectedPlace = null;
        updateSendButtonState();
      });
    }
  }
}

function setNavStatus(msg) {
  const el = document.getElementById('navStatusMessage');
  if (el) el.textContent = msg;
}

function updateSendButtonState() {
  const sendBtn = document.querySelector('#sendNavigation');
  if (!sendBtn) return;
  
  const dest = document.querySelector('#navDestInput')?.value?.trim();
  const isConnected = !!(activeDevice && activeDevice.gatt && activeDevice.gatt.connected && commandCharacteristic);
  
  sendBtn.disabled = !dest || !isConnected;
}

function updateUiState() {
  const devicesSelect = document.querySelector('#devicesSelect');
  const scanButton = document.querySelector('#requestBluetoothDevice');
  const scanSpinner = document.querySelector('#scanSpinner');
  const scanButtonText = document.querySelector('#scanButtonText');
  const forgetButton = document.querySelector('#forgetBluetoothDevice');
  const connectButton = document.querySelector('#connectBluetoothDevice');
  const connectSpinner = document.querySelector('#connectSpinner');
  const connectButtonText = document.querySelector('#connectButtonText');
  const commandInput = document.querySelector('#commandInput');
  const sendCommandButton = document.querySelector('#sendCommand');
  const navOriginInput = document.querySelector('#navOriginInput');
  const navDestInput = document.querySelector('#navDestInput');
  const sendNavButton = document.querySelector('#sendNavigation');
  const statusBadge = document.querySelector('#connectionStatusBadge');
  const connectionCard = document.querySelector('#connectionCard');
  const trackGpsToggle = document.querySelector('#trackGpsToggle');

  const hasDevices = devicesSelect && devicesSelect.options.length > 0 && devicesSelect.options[0].value !== '';
  const hasSelection = devicesSelect && devicesSelect.value && devicesSelect.value !== '';
  const isConnected = !!(activeDevice && activeDevice.gatt && activeDevice.gatt.connected && commandCharacteristic);

  // Scan button
  if (scanButton) {
    scanButton.disabled = isScanning || isConnecting;
    scanSpinner?.classList.toggle('d-none', !isScanning);
    if (scanButtonText) scanButtonText.textContent = isScanning ? 'Scanning...' : 'Scan for Devices';
  }

  // Devices select
  if (devicesSelect) {
    devicesSelect.disabled = isScanning || isConnecting;
  }

  // Forget button
  if (forgetButton) {
    forgetButton.disabled = !hasSelection || isScanning || isConnecting;
  }

  // Connect button
  if (connectButton) {
    if (isConnected) {
      connectButton.disabled = true;
      connectButton.classList.remove('btn-success');
      connectButton.classList.add('btn-outline-success');
    } else {
      connectButton.disabled = !hasSelection || isConnecting || isScanning;
      connectButton.classList.remove('btn-outline-success');
      connectButton.classList.add('btn-success');
    }
    connectSpinner?.classList.toggle('d-none', !isConnecting);
    if (connectButtonText) {
      if (isConnected) {
        connectButtonText.textContent = 'Connected';
      } else if (isConnecting) {
        connectButtonText.textContent = 'Connecting...';
      } else {
        connectButtonText.textContent = 'Connect';
      }
    }
  }

  // Command input and send button (only in offcanvas, require connection)
  if (commandInput) commandInput.disabled = !isConnected;
  if (sendCommandButton) sendCommandButton.disabled = !isConnected;
  
  // Navigation: origin is always visible (GPS auto-fills), dest is always editable
  // Send button requires connection + destination
  // navOriginInput stays readonly when GPS tracking is on, editable otherwise
  // navDestInput is always enabled for input

  // Status badge
  if (statusBadge) {
    statusBadge.className = 'badge';
    if (isScanning) {
      statusBadge.classList.add('bg-info');
      statusBadge.textContent = 'Scanning...';
    } else if (isConnecting) {
      statusBadge.classList.add('bg-warning', 'text-dark');
      statusBadge.textContent = 'Connecting...';
    } else if (isConnected) {
      statusBadge.classList.add('bg-success');
      statusBadge.textContent = 'Connected';
    } else if (hasDevices) {
      statusBadge.classList.add('bg-secondary');
      statusBadge.textContent = 'Not connected';
    } else {
      statusBadge.classList.add('bg-secondary');
      statusBadge.textContent = 'Idle';
    }
  }

  // Connection card border
  if (connectionCard) {
    connectionCard.classList.toggle('border-success', isConnected);
  }

  // Track GPS toggle: enable if geolocation permission is granted
  if (trackGpsToggle) {
    const geoPermissionBadge = document.querySelector('#geoPermissionBadge');
    const isGeoGranted = geoPermissionBadge && geoPermissionBadge.textContent === 'Granted';
    trackGpsToggle.disabled = !isGeoGranted;
  }
  
  // Update navbar Bluetooth status dot
  updateBluetoothStatusDot(isConnected, isConnecting, isScanning);
  
  // Update send button state
  updateSendButtonState();
}

function updateBluetoothStatusDot(isConnected, isConnecting, isScanning) {
  const dot = document.getElementById('btStatusDot');
  if (!dot) return;
  
  dot.classList.remove('connected', 'connecting', 'disconnected');
  
  if (isConnected) {
    dot.classList.add('connected');
  } else if (isConnecting || isScanning) {
    dot.classList.add('connecting');
  } else {
    dot.classList.add('disconnected');
  }
}

function populateBluetoothDevices() {
  const devicesSelect = document.querySelector('#devicesSelect');
  
  // Check if getDevices() is available (requires Chrome flag)
  if (!navigator.bluetooth || typeof navigator.bluetooth.getDevices !== 'function') {
    log('[A] Note: getDevices() not available. Enable chrome://flags/#enable-web-bluetooth-new-permissions-backend');
    updateUiState();
    return;
  }
  
  log('[A] Getting existing permitted Bluetooth devices...');
  navigator.bluetooth.getDevices()
  .then(devices => {
    log('[A] Got ' + devices.length + ' Bluetooth devices.');
    devicesSelect.textContent = '';
    for (const device of devices) {
      const option = document.createElement('option');
      option.value = device.id;
      option.textContent = device.name || device.id;
      devicesSelect.appendChild(option);
    }
    updateUiState();
  })
  .catch(error => {
    log('[A] Argh! ' + error);
    updateUiState();
  });
}

function getBleConfig() {
  const serviceInput = document.querySelector('#serviceUuid');
  const serviceUuid = (serviceInput?.value || DEFAULT_SERVICE_UUID).trim().toLowerCase();
  const commandUuid = DEFAULT_COMMAND_CHARACTERISTIC_UUID.toLowerCase();
  const navOriginUuid = DEFAULT_NAV_ORIGIN_CHARACTERISTIC_UUID.toLowerCase();
  const navDestUuid = DEFAULT_NAV_DEST_CHARACTERISTIC_UUID.toLowerCase();
  if (serviceUuid !== DEFAULT_SERVICE_UUID.toLowerCase()) {
    log('[A] Using custom Service UUID: ' + serviceUuid);
  }
  return { serviceUuid, commandUuid, navOriginUuid, navDestUuid };
}

function onRequestBluetoothDeviceButtonClick() {
  log('[A] Scanning for T5M4R-ARD peripheral or device with UART service...');
  const { serviceUuid } = getBleConfig();

  const filters = [
    { name: 'SID-ARD', services: [serviceUuid] },
    { services: [serviceUuid] }
  ];

  isScanning = true;
  updateUiState();

  navigator.bluetooth.requestDevice({ filters })
  .then(device => {
    log('[A] Requested ' + (device.name || 'unnamed device') + ' (' + device.id + ')');
    device.addEventListener('gattserverdisconnected', handleDisconnection);
    activeDevice = device;
    populateBluetoothDevices();
  })
  .catch(error => {
    if (error.name === 'NotFoundError') {
      log('[A] No device selected. Confirm the Arduino peripheral is advertising and in range.');
    } else {
      log('[A] Argh! ' + error);
    }
  })
  .finally(() => {
    isScanning = false;
    updateUiState();
  });
}

function onForgetBluetoothDeviceButtonClick() {
  navigator.bluetooth.getDevices()
  .then(devices => {
    const deviceIdToForget = document.querySelector('#devicesSelect').value;
    const device = devices.find((device) => device.id == deviceIdToForget);
    if (!device) {
      throw new Error('No Bluetooth device to forget');
    }
    log('[A] Forgetting ' + device.name + ' Bluetooth device...');
    return device.forget().then(() => ({ deviceIdToForget }));
  })
  .then(({ deviceIdToForget }) => {
    log('[A] Bluetooth device has been forgotten.');
    if (activeDevice && activeDevice.id === deviceIdToForget) {
      resetConnectionState();
    }
    populateBluetoothDevices();
  })
  .catch(error => {
    log('[A] Argh! ' + error);
    updateUiState();
  });
}

async function logGattServerDetails(server, primaryServiceUuid) {
  const services = await server.getPrimaryServices();
  log('[A] GATT Server: found ' + services.length + ' primary service(s).');

  for (const service of services) {
    const isPrimary = (service.isPrimary !== undefined) ? service.isPrimary : 'n/a';
    const isUartService = service.uuid.toLowerCase() === primaryServiceUuid.toLowerCase();

    log('[A] Service: ' + service.uuid + ' (primary: ' + isPrimary + ')' +
        (isUartService ? ' [UART SERVICE]' : ''));

    const characteristics = await service.getCharacteristics();
    log('[A]   Characteristics: ' + characteristics.length);

    for (const characteristic of characteristics) {
      log('[A]    - Characteristic: ' + characteristic.uuid);

      const props = characteristic.properties;
      const propNames = [
        'broadcast', 'read', 'write', 'writeWithoutResponse',
        'notify', 'indicate', 'authenticatedSignedWrites',
        'reliableWrite', 'writableAuxiliaries'
      ];

      const enabledProps = propNames.filter(name => props[name]);
      log('[A]       Properties: ' + (enabledProps.length ? enabledProps.join(', ') : 'none'));
    }
  }
}

async function getSelectedDevice() {
  const deviceId = document.querySelector('#devicesSelect').value;
  if (!deviceId) {
    throw new Error('Please select a Bluetooth device first.');
  }
  const devices = await navigator.bluetooth.getDevices();
  const device = devices.find((candidate) => candidate.id === deviceId);
  if (!device) {
    throw new Error('Selected Bluetooth device is no longer permitted.');
  }
  return device;
}

async function connectSelectedBluetoothDevice() {
  if (isConnecting) {
    throw new Error('Connection attempt already in progress.');
  }
  const { serviceUuid, commandUuid, navOriginUuid, navDestUuid } = getBleConfig();
  const device = await getSelectedDevice();
  if (activeDevice && activeDevice.id !== device.id) {
    resetConnectionState();
  }
  if (device.gatt.connected && commandCharacteristic) {
    activeDevice = device;
    log('[A] Already connected to peripheral: ' + (device.name || device.id));
    updateUiState();
    return;
  }
  isConnecting = true;
  updateUiState();
  try {
    activeDevice = device;
    activeDevice.removeEventListener('gattserverdisconnected', handleDisconnection);
    activeDevice.addEventListener('gattserverdisconnected', handleDisconnection);
    log('[A] Connecting to ' + (device.name || device.id) + '...');
    const server = await device.gatt.connect();
    log('[A] GATT server connected, discovering services...');
    
    log('[A] Looking for service: ' + serviceUuid);
    const service = await server.getPrimaryService(serviceUuid);
    log('[A] Found primary service: ' + service.uuid);
    
    commandCharacteristic = await service.getCharacteristic(commandUuid);
    log('[A] Found Command characteristic: ' + commandCharacteristic.uuid);
    
    // Subscribe to notifications from Command characteristic (for responses from helmet)
    if (commandCharacteristic.properties.notify) {
      commandCharacteristic.addEventListener('characteristicvaluechanged', handleCommandNotification);
      await commandCharacteristic.startNotifications();
      log('[A] Subscribed to Command notifications');
    }
    
    navOriginCharacteristic = await service.getCharacteristic(navOriginUuid);
    log('[A] Found NavOrigin characteristic: ' + navOriginCharacteristic.uuid);
    
    navDestCharacteristic = await service.getCharacteristic(navDestUuid);
    log('[A] Found NavDestination characteristic: ' + navDestCharacteristic.uuid);
    
    log('[A] Connected to peripheral: ' + (device.name || device.id));
    await logGattServerDetails(server, serviceUuid);
  } finally {
    isConnecting = false;
    updateUiState();
  }
}

async function onSendCommandButtonClick() {
  const input = document.querySelector('#commandInput');
  const command = (input.value || 'PING').trim();
  if (!command) {
    throw new Error('Command cannot be blank.');
  }
  await connectSelectedBluetoothDevice();
  if (!commandCharacteristic) {
    throw new Error('Command characteristic not available.');
  }
  await commandCharacteristic.writeValue(textEncoder.encode(command));
  log('[A→H] Command: ' + command);
}

async function onSendNavigationButtonClick() {
  const originInput = document.querySelector('#navOriginInput');
  const destInput = document.querySelector('#navDestInput');
  const destDisplayName = document.querySelector('#navDestDisplayName');
  const origin = (originInput?.value || '').trim();
  const destination = (destInput?.value || '').trim();
  const displayName = (destDisplayName?.value || destination).trim();
  
  if (!destination) {
    setNavStatus('Please enter a destination.');
    return;
  }
  
  // Use GPS origin if available, otherwise show error
  if (!origin && isTrackingGpsForOrigin) {
    setNavStatus('Waiting for GPS location...');
    return;
  }
  
  setNavStatus('Sending to helmet...');
  
  await connectSelectedBluetoothDevice();
  if (!navOriginCharacteristic || !navDestCharacteristic) {
    throw new Error('Navigation characteristics not available.');
  }
  
  await navOriginCharacteristic.writeValue(textEncoder.encode(origin));
  log('[A→H] Origin: ' + origin);
  
  await navDestCharacteristic.writeValue(textEncoder.encode(destination));
  log('[A→H] Destination: ' + destination);
  
  // Save to recents if using fallback text input (Places Autocomplete saves on selection)
  if (!selectedPlace || !selectedPlace.id) {
    // Fallback for manual text entry
    let lat = null, lng = null;
    const coordMatch = destination.match(/^(-?\d+\.?\d*),\s*(-?\d+\.?\d*)$/);
    if (coordMatch) {
      lat = parseFloat(coordMatch[1]);
      lng = parseFloat(coordMatch[2]);
    }
    saveRecentPlace({
      id: 'manual_' + Date.now(),
      displayName: displayName || destination,
      lat: lat,
      lng: lng
    });
    renderRecentPlaces();
  }
  
  // Clear selected place and indicator after sending
  selectedPlace = null;
  clearSelectedDestination();
  
  setNavStatus('✓ Sent to helmet!');
  setTimeout(() => setNavStatus(''), 3000);
}

function handleCommandNotification(event) {
  const value = event.target.value;
  const decoder = new TextDecoder('utf-8');
  const message = decoder.decode(value);
  log('[H→A] Command: ' + message);
  
  // Parse INFO::NAVSTEP messages and display in UI
  // Format: INFO::NAVSTEP:current:total:maneuver:distance:instruction
  if (message.startsWith('INFO::NAVSTEP:')) {
    const parts = message.substring(14).split(':');
    log('[A] Parsing NAVSTEP, parts: ' + parts.length);
    if (parts.length >= 5) {
      const navStep = {
        current: parseInt(parts[0], 10),
        total: parseInt(parts[1], 10),
        maneuver: parts[2],
        distance: parts[3],
        instruction: parts.slice(4).join(':') // instruction may contain colons
      };
      log('[A] NavStep parsed: step ' + (navStep.current + 1) + '/' + navStep.total + ' ' + navStep.maneuver);
      displayNavStep(navStep);
    } else {
      log('[A] NAVSTEP parse failed: not enough parts');
    }
  }
  
  // Handle INFO::NAVCOMPLETED - route finished
  if (message === 'INFO::NAVCOMPLETED') {
    log('[A] Navigation completed!');
    stopFastForward();
    displayNavCompleted();
  }
}

function displayNavCompleted() {
  const container = document.getElementById('helmetNavStatus');
  if (!container) return;
  
  container.style.display = 'block';
  
  const stepCounter = container.querySelector('.nav-step-counter');
  const maneuverEl = container.querySelector('.nav-maneuver');
  const distanceEl = container.querySelector('.nav-distance');
  const instructionEl = container.querySelector('.nav-instruction');
  
  if (stepCounter) stepCounter.textContent = 'Route Complete';
  if (maneuverEl) maneuverEl.textContent = '✓ Arrived';
  if (distanceEl) distanceEl.textContent = '';
  if (instructionEl) instructionEl.innerHTML = 'You have reached your destination.';
}

function displayNavStep(navStep) {
  const container = document.getElementById('helmetNavStatus');
  if (!container) return;
  
  container.style.display = 'block';
  
  const stepCounter = container.querySelector('.nav-step-counter');
  const maneuverEl = container.querySelector('.nav-maneuver');
  const distanceEl = container.querySelector('.nav-distance');
  const instructionEl = container.querySelector('.nav-instruction');
  
  if (stepCounter) stepCounter.textContent = 'Step ' + (navStep.current + 1) + ' of ' + navStep.total;
  if (maneuverEl) maneuverEl.textContent = navStep.maneuver.replace(/-/g, ' ');
  if (distanceEl) distanceEl.textContent = navStep.distance;
  if (instructionEl) instructionEl.innerHTML = navStep.instruction;
}

// ============ Fast-Forward Timer ============

function startFastForward() {
  stopFastForward();
  
  const intervalSelect = document.getElementById('fastForwardInterval');
  const intervalSeconds = parseInt(intervalSelect?.value || '3', 10);
  
  log('[A] Fast-forward started (every ' + intervalSeconds + 's)');
  
  fastForwardIntervalId = setInterval(async () => {
    if (!activeDevice?.gatt?.connected || !commandCharacteristic) {
      log('[A] Fast-forward stopped: not connected');
      stopFastForward();
      return;
    }
    
    try {
      await commandCharacteristic.writeValueWithoutResponse(textEncoder.encode('NAV::FASTFORWARD'));
      log('[A→H] Command: NAV::FASTFORWARD');
    } catch (err) {
      log('[A→H] Argh! Fast-forward write failed: ' + err);
      stopFastForward();
    }
  }, intervalSeconds * 1000);
}

function stopFastForward() {
  if (fastForwardIntervalId !== null) {
    clearInterval(fastForwardIntervalId);
    fastForwardIntervalId = null;
    log('[A] Fast-forward stopped');
  }
  
  const toggle = document.getElementById('fastForwardToggle');
  if (toggle) toggle.checked = false;
}

function initFastForwardControls() {
  const toggle = document.getElementById('fastForwardToggle');
  const intervalSelect = document.getElementById('fastForwardInterval');
  
  if (toggle) {
    log('[A] Fast-forward toggle control initialized');
    toggle.addEventListener('change', function() {
      log('[A] Fast-forward toggle changed: ' + this.checked);
      if (this.checked) {
        startFastForward();
      } else {
        stopFastForward();
      }
    });
  } else {
    log('[A] Fast-forward toggle element not found');
  }
  
  if (intervalSelect) {
    intervalSelect.addEventListener('change', function() {
      if (fastForwardIntervalId !== null) {
        startFastForward();
      }
    });
  }
}

function handleDisconnection(event) {
  const device = event.target;
  log('[H→A] ' + (device.name || device.id) + ' disconnected.');
  resetConnectionState();
}

function resetConnectionState() {
  if (activeDevice) {
    activeDevice.removeEventListener('gattserverdisconnected', handleDisconnection);
  }
  // Remove notification listener from command characteristic
  if (commandCharacteristic) {
    commandCharacteristic.removeEventListener('characteristicvaluechanged', handleCommandNotification);
  }
  // Stop fast-forward timer on disconnect
  stopFastForward();
  
  activeDevice = null;
  commandCharacteristic = null;
  navOriginCharacteristic = null;
  navDestCharacteristic = null;
  updateUiState();
}

async function updateGeolocationPermissionIndicator(state) {
  const badge = document.querySelector('#geoPermissionBadge');
  if (!badge) return;
  badge.className = 'badge ms-2';
  if (state === 'granted') {
    badge.classList.add('bg-success');
    badge.textContent = 'Granted';
  } else if (state === 'prompt') {
    badge.classList.add('bg-warning', 'text-dark');
    badge.textContent = 'Prompt';
  } else if (state === 'denied') {
    badge.classList.add('bg-danger');
    badge.textContent = 'Denied';
  } else if (state === 'unavailable' || state === 'unknown') {
    badge.classList.add('bg-secondary');
    badge.textContent = 'Unavailable';
  } else {
    badge.classList.add('bg-secondary');
    badge.textContent = state || 'Unknown';
  }
  
  updateUiState();
}

function updateGeolocationCoordinates(latitude, longitude) {
  const coordsStr = latitude.toFixed(6) + ', ' + longitude.toFixed(6);
  const coordsPayload = latitude.toFixed(6) + ',' + longitude.toFixed(6);
  
  // Update Google Maps link
  const mapsLink = document.querySelector('#geoMapsLink');
  if (mapsLink) {
    const encodedCoords = encodeURIComponent(coordsPayload);
    mapsLink.href = 'https://www.google.com/maps/search/?api=1&query=' + encodedCoords;
    mapsLink.style.display = 'inline-block';
  }

  // Always update Origin field with GPS (consumer UX - GPS is primary source)
  const navOriginInput = document.querySelector('#navOriginInput');
  if (navOriginInput) {
    navOriginInput.value = coordsPayload;
  }
  
  // Update origin summary text
  const originSummary = document.getElementById('originSummaryText');
  if (originSummary) {
    originSummary.textContent = 'GPS: ' + coordsStr;
  }
  
  // Update Places Autocomplete locationBias with new GPS coordinates
  if (placesAutocompleteElement) {
    placesAutocompleteElement.locationBias = {
      center: { lat: latitude, lng: longitude },
      radius: 5000
    };
  }
  
  // Smart throttle: send GPS to Arduino via BLE if thresholds met
  maybeSendGpsToBle(latitude, longitude);
}

function startGeolocationWatch() {
  if (!navigator.geolocation) {
    log('[A] Geolocation API not available.');
    return;
  }

  // Stop any existing watch
  if (geolocationWatchId !== null) {
    navigator.geolocation.clearWatch(geolocationWatchId);
  }

  geolocationWatchId = navigator.geolocation.watchPosition(
    (position) => {
      // Mobile Chrome queues geolocation events when app is backgrounded.
      // Filter out stale/duplicate positions using timestamp.
      if (position.timestamp <= lastProcessedGeoTimestamp) {
        return; // Skip: already processed this or newer position
      }
      
      const { latitude, longitude } = position.coords;
      const ageMilliseconds = (Date.now() - position.timestamp);
      
      // Skip positions that are too stale (older than maximumAge + 10s grace period)
      if (ageMilliseconds > GEOLOCATION_OPTIONS.maximumAge + 10000) {
        log('[A] Skipping stale geolocation fix (' + ageMilliseconds + 'ms old)');
        return;
      }
      
      lastProcessedGeoTimestamp = position.timestamp;
      updateGeolocationCoordinates(latitude, longitude);
      log('[A] Geolocation updated: ' + latitude.toFixed(6) + ', ' + longitude.toFixed(6) + ' (' + ageMilliseconds + 'ms ago)');
    },
    (error) => {
      log('[A] Geolocation watch error: ' + error.message);
    },
    GEOLOCATION_OPTIONS
  );
}

async function requeryGeolocationPermission() {
  if (!navigator.permissions) return;
  try {
    const status = await navigator.permissions.query({ name: 'geolocation' });
    updateGeolocationPermissionIndicator(status.state);
    // Start watch if permission is granted
    if (status.state === 'granted') {
      startGeolocationWatch();
    }
    return status;
  } catch (err) {
    log('[A] Argh! ' + err);
    updateGeolocationPermissionIndicator('unknown');
  }
}

function startPermissionPoll(maxAttempts = 6, intervalMs = 1000) {
  if (!navigator.permissions) return;
  let attempts = 0;
  const pollId = setInterval(async () => {
    attempts++;
    try {
      const status = await navigator.permissions.query({ name: 'geolocation' });
      updateGeolocationPermissionIndicator(status.state);
      if (status.state === 'granted') {
        startGeolocationWatch();
      }
      if (status.state === 'granted' || status.state === 'denied' || attempts >= maxAttempts) {
        clearInterval(pollId);
      }
    } catch (e) {
      clearInterval(pollId);
    }
  }, intervalMs);
  return pollId;
}

async function requestGeolocationPermissionOnLoad() {
  if (!navigator.permissions || !navigator.geolocation) {
    log('[A] Geolocation or Permissions API not available in this browser.');
    await updateGeolocationPermissionIndicator('unavailable');
    return;
  }
  try {
    const status = await navigator.permissions.query({ name: 'geolocation' });
    updateGeolocationPermissionIndicator(status.state);

    // Attach onchange where supported
    try {
      status.onchange = () => {
        log('[A] Geolocation permission changed to ' + status.state);
        updateGeolocationPermissionIndicator(status.state);
        if (status.state === 'granted') {
          startGeolocationWatch();
        } else {
          // Stop watching if permission was revoked
          if (geolocationWatchId !== null) {
            navigator.geolocation.clearWatch(geolocationWatchId);
            geolocationWatchId = null;
          }
        }
      };
    } catch (e) {
      // ignore if not supported
    }

    // If state is "prompt", trigger prompt and re-query after response.
    if (status.state === 'prompt') {
      const pollId = startPermissionPoll();

      navigator.geolocation.getCurrentPosition(
        (pos) => {
          log('[A] Geolocation allowed. coords: ' + pos.coords.latitude + ',' + pos.coords.longitude);
          updateGeolocationCoordinates(pos.coords.latitude, pos.coords.longitude);
          requeryGeolocationPermission();
          if (pollId) clearInterval(pollId);
        },
        (err) => {
          log('[A] Geolocation error: ' + err.message);
          requeryGeolocationPermission();
          if (pollId) clearInterval(pollId);
        },
        GEOLOCATION_OPTIONS
      );
    } else if (status.state === 'granted') {
      // If already granted, start watching immediately
      startGeolocationWatch();
    }

    // Re-check when page becomes visible
    // Also restart watch to clear queued events from Chrome mobile battery management
    document.addEventListener('visibilitychange', () => {
      if (document.visibilityState === 'visible') {
        requeryGeolocationPermission();
        // Restart watch to avoid burst of queued geolocation events on mobile
        if (geolocationWatchId !== null) {
          navigator.geolocation.clearWatch(geolocationWatchId);
          geolocationWatchId = null;
          startGeolocationWatch();
        }
      }
    });

    // Re-check on window focus
    // Also restart watch to avoid burst of queued events
    window.addEventListener('focus', () => {
      requeryGeolocationPermission();
      if (geolocationWatchId !== null) {
        navigator.geolocation.clearWatch(geolocationWatchId);
        geolocationWatchId = null;
        startGeolocationWatch();
      }
    });
  } catch (error) {
    log('[A] Argh! ' + error);
    updateGeolocationPermissionIndicator('unknown');
  }
}

window.onload = () => {
  requestGeolocationPermissionOnLoad();
  populateBluetoothDevices();
  renderRecentPlaces();
  initPlacesAutocomplete();
  initFastForwardControls();
  updateUiState();

  // Clear selected destination button handler
  const clearSelectedBtn = document.getElementById('clearSelectedDest');
  if (clearSelectedBtn) {
    clearSelectedBtn.addEventListener('click', clearSelectedDestination);
  }

  // Setup GPS tracking toggle (default checked)
  const trackGpsToggle = document.querySelector('#trackGpsToggle');
  if (trackGpsToggle) {
    trackGpsToggle.checked = true;
    isTrackingGpsForOrigin = true;
    
    trackGpsToggle.addEventListener('change', function() {
      isTrackingGpsForOrigin = this.checked;
      const navOriginInput = document.querySelector('#navOriginInput');
      const originSummary = document.getElementById('originSummaryText');
      
      if (isTrackingGpsForOrigin) {
        log('[A] GPS tracking for Origin enabled.');
        navOriginInput.setAttribute('readonly', 'readonly');
        if (originSummary) originSummary.textContent = 'Current location';
      } else {
        log('[A] GPS tracking for Origin disabled.');
        navOriginInput.value = '';
        navOriginInput.removeAttribute('readonly');
        if (originSummary) originSummary.textContent = 'Manual entry';
      }
    });
  }
};
