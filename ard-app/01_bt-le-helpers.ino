
// Central in BLE-speak is usually the high-capacity smartphone
void blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
  textCharacteristic.writeValue("YOU ARE NOW CONNECTED DEAR CENTRAL! :)");
}

// Central in BLE-speak is usually the high-capacity smartphone
void blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
}

// Characteristic event occurred (e.g Read or Write)
void bleCharacteristicWrittenHandler(BLEDevice device, BLECharacteristic characteristic) {
  // central disconnected event handler
  Serial.print("Data written on Characteristic: ");
  Serial.println(static_cast<BLEStringCharacteristic *>(&characteristic)->value());
}

// Characteristic event occurred (e.g Read or Write)
void bleCharacteristicUpdatedHandler(BLEDevice device, BLECharacteristic characteristic) {
  // central disconnected event handler
  Serial.print("Characteristic updated to value : ");
  Serial.println(static_cast<BLEStringCharacteristic *>(&characteristic)->value());
}