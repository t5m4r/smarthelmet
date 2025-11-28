const DEFAULT_UART_SERVICE_UUID = '19B10001-E8F2-537E-4F6C-D104768A1225';
const DEFAULT_UART_TX_CHARACTERISTIC_UUID = '19B10002-E8F2-537E-4F6C-D104768A1214';
const DEFAULT_UART_RX_CHARACTERISTIC_UUID = '19B10002-E8F2-537E-4F6C-D104768A1214';
let activeDevice = null;
let uartTxCharacteristic = null;
let uartRxCharacteristic = null;
const textEncoder = new TextEncoder();
const textDecoder = new TextDecoder();
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
  const messageInput = document.querySelector('#messageInput');
  const sendButton = document.querySelector('#sendMessage');
  const statusBadge = document.querySelector('#connectionStatusBadge');
  const connectionCard = document.querySelector('#connectionCard');

  const hasDevices = devicesSelect && devicesSelect.options.length > 0 && devicesSelect.options[0].value !== '';
  const hasSelection = devicesSelect && devicesSelect.value && devicesSelect.value !== '';
  const isConnected = !!(activeDevice && activeDevice.gatt && activeDevice.gatt.connected && uartTxCharacteristic);

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

  // Message input and send button
  if (messageInput) messageInput.disabled = !isConnected;
  if (sendButton) sendButton.disabled = !isConnected;

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
  const txInput = document.querySelector('#txUuid');
  const rxInput = document.querySelector('#rxUuid');
  const serviceUuid = (serviceInput?.value || DEFAULT_UART_SERVICE_UUID).trim().toLowerCase();
  const txUuid = (txInput?.value || DEFAULT_UART_TX_CHARACTERISTIC_UUID).trim().toLowerCase();
  const rxUuid = (rxInput?.value || DEFAULT_UART_RX_CHARACTERISTIC_UUID).trim().toLowerCase();
  if (serviceUuid !== DEFAULT_UART_SERVICE_UUID) {
    log('Using custom UART Service UUID: ' + serviceUuid);
  }
  return { serviceUuid, txUuid, rxUuid };
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
  const { serviceUuid, txUuid, rxUuid } = getBleConfig();
  const device = await getSelectedDevice();
  if (activeDevice && activeDevice.id !== device.id) {
    resetConnectionState();
  }
  if (device.gatt.connected && uartTxCharacteristic) {
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
    
    // Debug: list ALL services the peripheral exposes
    const allServices = await server.getPrimaryServices();
    log('  Peripheral exposes ' + allServices.length + ' service(s):');
    for (const svc of allServices) {
      log('    - ' + svc.uuid);
    }
    
    if (uartRxCharacteristic) {
      uartRxCharacteristic.removeEventListener('characteristicvaluechanged', handleIncomingMessage);
    }
    log('  Looking for service: ' + serviceUuid);
    const service = await server.getPrimaryService(serviceUuid);
    log('  Found primary service: ' + service.uuid);
    uartRxCharacteristic = await service.getCharacteristic(rxUuid);
    log('  Found RX characteristic: ' + uartRxCharacteristic.uuid);
    uartTxCharacteristic = await service.getCharacteristic(txUuid);
    log('  Found TX characteristic: ' + uartTxCharacteristic.uuid);
    await uartRxCharacteristic.startNotifications();
    log('  Notifications started on RX characteristic');
    uartRxCharacteristic.addEventListener('characteristicvaluechanged', handleIncomingMessage);
    log('Connected to peripheral: ' + (device.name || device.id));
    await logGattServerDetails(server, serviceUuid);
  } finally {
    isConnecting = false;
    updateUiState();
  }
}

async function onSendMessageButtonClick() {
  const input = document.querySelector('#messageInput');
  const message = (input.value || 'PING').trim();
  if (!message) {
    throw new Error('Message cannot be blank.');
  }
  await connectSelectedBluetoothDevice();
  if (!uartTxCharacteristic) {
    throw new Error('UART write characteristic not available.');
  }
  await uartTxCharacteristic.writeValue(textEncoder.encode(message));
  log('>> ' + message);
}

function handleIncomingMessage(event) {
  const view = event.target.value;
  const data = new Uint8Array(view.buffer, view.byteOffset, view.byteLength);
  const message = textDecoder.decode(data);
  log('<< ' + message.trim());
}

function handleDisconnection(event) {
  const device = event.target;
  log('> ' + (device.name || device.id) + ' disconnected.');
  resetConnectionState();
}

function resetConnectionState() {
  if (uartRxCharacteristic) {
    uartRxCharacteristic.removeEventListener('characteristicvaluechanged', handleIncomingMessage);
  }
  if (activeDevice) {
    activeDevice.removeEventListener('gattserverdisconnected', handleDisconnection);
  }
  activeDevice = null;
  uartTxCharacteristic = null;
  uartRxCharacteristic = null;
  updateUiState();
}

window.onload = () => {
  populateBluetoothDevices();
  updateUiState();
};
