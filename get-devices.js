const DEFAULT_SERVICE_UUID = '19B10001-E8F2-537E-4F6C-D104768A1225';
const DEFAULT_COMMAND_CHARACTERISTIC_UUID = '19B10002-E8F2-537E-4F6C-D104768A1214';
const DEFAULT_NAV_ORIGIN_CHARACTERISTIC_UUID = '19B10002-E8F2-537E-4F6C-D104768A1215';
const DEFAULT_NAV_DEST_CHARACTERISTIC_UUID = '19B10002-E8F2-537E-4F6C-D104768A1216';

let activeDevice = null;
let commandCharacteristic = null;
let navOriginCharacteristic = null;
let navDestCharacteristic = null;
const textEncoder = new TextEncoder();
let isConnecting = false;
let isScanning = false;

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

  // Command input and send button
  if (commandInput) commandInput.disabled = !isConnected;
  if (sendCommandButton) sendCommandButton.disabled = !isConnected;
  
  // Navigation inputs and send button
  if (navOriginInput) navOriginInput.disabled = !isConnected;
  if (navDestInput) navDestInput.disabled = !isConnected;
  if (sendNavButton) sendNavButton.disabled = !isConnected;

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
}

function populateBluetoothDevices() {
  const devicesSelect = document.querySelector('#devicesSelect');
  log('Getting existing permitted Bluetooth devices...');
  navigator.bluetooth.getDevices()
  .then(devices => {
    log('> Got ' + devices.length + ' Bluetooth devices.');
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
    log('Argh! ' + error);
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
    log('Using custom Service UUID: ' + serviceUuid);
  }
  return { serviceUuid, commandUuid, navOriginUuid, navDestUuid };
}

function onRequestBluetoothDeviceButtonClick() {
  log('Scanning for T5M4R-ARD peripheral or device with UART service...');
  const { serviceUuid } = getBleConfig();

  const filters = [
    { name: 'T5M4R-ARD', services: [serviceUuid] },
    { services: [serviceUuid] }
  ];

  isScanning = true;
  updateUiState();

  navigator.bluetooth.requestDevice({ filters })
  .then(device => {
    log('> Requested ' + (device.name || 'unnamed device') + ' (' + device.id + ')');
    device.addEventListener('gattserverdisconnected', handleDisconnection);
    activeDevice = device;
    populateBluetoothDevices();
  })
  .catch(error => {
    if (error.name === 'NotFoundError') {
      log('No device selected. Confirm the Arduino peripheral is advertising and in range.');
    } else {
      log('Argh! ' + error);
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
    log('Forgetting ' + device.name + ' Bluetooth device...');
    return device.forget().then(() => ({ deviceIdToForget }));
  })
  .then(({ deviceIdToForget }) => {
    log('  > Bluetooth device has been forgotten.');
    if (activeDevice && activeDevice.id === deviceIdToForget) {
      resetConnectionState();
    }
    populateBluetoothDevices();
  })
  .catch(error => {
    log('Argh! ' + error);
    updateUiState();
  });
}

async function logGattServerDetails(server, primaryServiceUuid) {
  const services = await server.getPrimaryServices();
  log('GATT Server: found ' + services.length + ' primary service(s).');

  for (const service of services) {
    const isPrimary = (service.isPrimary !== undefined) ? service.isPrimary : 'n/a';
    const isUartService = service.uuid.toLowerCase() === primaryServiceUuid.toLowerCase();

    log('Service: ' + service.uuid + ' (primary: ' + isPrimary + ')' +
        (isUartService ? ' [UART SERVICE]' : ''));

    const characteristics = await service.getCharacteristics();
    log('  Characteristics: ' + characteristics.length);

    for (const characteristic of characteristics) {
      log('   - Characteristic: ' + characteristic.uuid);

      const props = characteristic.properties;
      const propNames = [
        'broadcast', 'read', 'write', 'writeWithoutResponse',
        'notify', 'indicate', 'authenticatedSignedWrites',
        'reliableWrite', 'writableAuxiliaries'
      ];

      const enabledProps = propNames.filter(name => props[name]);
      log('      Properties: ' + (enabledProps.length ? enabledProps.join(', ') : 'none'));
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
    log('Already connected to peripheral: ' + (device.name || device.id));
    updateUiState();
    return;
  }
  isConnecting = true;
  updateUiState();
  try {
    activeDevice = device;
    activeDevice.removeEventListener('gattserverdisconnected', handleDisconnection);
    activeDevice.addEventListener('gattserverdisconnected', handleDisconnection);
    log('Connecting to ' + (device.name || device.id) + '...');
    const server = await device.gatt.connect();
    log('  GATT server connected, discovering services...');
    
    log('  Looking for service: ' + serviceUuid);
    const service = await server.getPrimaryService(serviceUuid);
    log('  Found primary service: ' + service.uuid);
    
    commandCharacteristic = await service.getCharacteristic(commandUuid);
    log('  Found Command characteristic: ' + commandCharacteristic.uuid);
    
    navOriginCharacteristic = await service.getCharacteristic(navOriginUuid);
    log('  Found NavOrigin characteristic: ' + navOriginCharacteristic.uuid);
    
    navDestCharacteristic = await service.getCharacteristic(navDestUuid);
    log('  Found NavDestination characteristic: ' + navDestCharacteristic.uuid);
    
    log('Connected to peripheral: ' + (device.name || device.id));
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
  log('>> Command: ' + command);
}

async function onSendNavigationButtonClick() {
  const originInput = document.querySelector('#navOriginInput');
  const destInput = document.querySelector('#navDestInput');
  const origin = (originInput.value || '').trim();
  const destination = (destInput.value || '').trim();
  
  if (!origin || !destination) {
    throw new Error('Both origin and destination are required.');
  }
  
  await connectSelectedBluetoothDevice();
  if (!navOriginCharacteristic || !navDestCharacteristic) {
    throw new Error('Navigation characteristics not available.');
  }
  
  await navOriginCharacteristic.writeValue(textEncoder.encode(origin));
  log('>> Origin: ' + origin);
  
  await navDestCharacteristic.writeValue(textEncoder.encode(destination));
  log('>> Destination: ' + destination);
}

function handleDisconnection(event) {
  const device = event.target;
  log('> ' + (device.name || device.id) + ' disconnected.');
  resetConnectionState();
}

function resetConnectionState() {
  if (activeDevice) {
    activeDevice.removeEventListener('gattserverdisconnected', handleDisconnection);
  }
  activeDevice = null;
  commandCharacteristic = null;
  navOriginCharacteristic = null;
  navDestCharacteristic = null;
  updateUiState();
}

async function updateGeolocationPermissionIndicator(state) {
  const badge = document.querySelector('#geoPermissionBadge');
  if (!badge) return;
  // reset classes then add appropriate ones
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
}

async function requeryGeolocationPermission() {
  if (!navigator.permissions) return;
  try {
    const status = await navigator.permissions.query({ name: 'geolocation' });
    updateGeolocationPermissionIndicator(status.state);
    return status;
  } catch (err) {
    log('Argh! ' + err);
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
    log('Geolocation or Permissions API not available in this browser.');
    await updateGeolocationPermissionIndicator('unavailable');
    return;
  }
  try {
    const status = await navigator.permissions.query({ name: 'geolocation' });
    updateGeolocationPermissionIndicator(status.state);

    // Attach onchange where supported
    try {
      status.onchange = () => {
        log('Geolocation permission changed to ' + status.state);
        updateGeolocationPermissionIndicator(status.state);
      };
    } catch (e) {
      // ignore if not supported
    }

    // If state is "prompt", trigger prompt and re-query after response.
    if (status.state === 'prompt') {
      // Start a short poll as a fallback in case onchange is not fired
      const pollId = startPermissionPoll();

      navigator.geolocation.getCurrentPosition(
        (pos) => {
          log('Geolocation allowed. coords: ' + pos.coords.latitude + ',' + pos.coords.longitude);
          // Re-query to ensure Permissions API state is updated
          requeryGeolocationPermission();
          if (pollId) clearInterval(pollId);
        },
        (err) => {
          log('Geolocation error: ' + err.message);
          // Re-query to ensure Permissions API state is updated
          requeryGeolocationPermission();
          if (pollId) clearInterval(pollId);
        },
        { timeout: 10000 }
      );
    }

    // Re-check when page becomes visible (user may change permission via UI)
    document.addEventListener('visibilitychange', () => {
      if (document.visibilityState === 'visible') {
        requeryGeolocationPermission();
      }
    });

    // Also re-check on window focus as another fallback
    window.addEventListener('focus', () => {
      requeryGeolocationPermission();
    });
  } catch (error) {
    log('Argh! ' + error);
    updateGeolocationPermissionIndicator('unknown');
  }
}

window.onload = () => {
  requestGeolocationPermissionOnLoad();
  populateBluetoothDevices();
  updateUiState();
};
