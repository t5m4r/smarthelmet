
// Central in BLE-speak is usually the high-capacity smartphone
void blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
  headSenseState = HEADSENSE_WELCOME;
  prettyPrintHeadSenseState(headSenseState);
}

// Central in BLE-speak is usually the high-capacity smartphone
void blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  //Serial.println(central.address());
  headSenseState = HEADSENSE_UNINITIALIZED;
  prettyPrintHeadSenseState(headSenseState);
}

// Triggers when the central device (the phone) sends a command like STARTNAV or ENDNAV to the Arduino (peripheral)
void p2aCommandUpdatedHandler(BLEDevice device, BLECharacteristic characteristic) {
  String command = static_cast<BLEStringCharacteristic *>(&characteristic)->value();
  Serial.print("Command received: ");
  Serial.println(command);

  // Handle NAV::FASTFORWARD command from MWA fast-forward feature
  if (command == "NAV::FASTFORWARD") {
    Serial.println("Fast-forward: advancing to next nav step");
    incrementNavStep();
  }

  prettyPrintHeadSenseState(headSenseState);
}

// Triggers when the central device (the phone) writes new data to either of the two BLESCharacteristics that hold navigation start and destination points
void p2aOriginOrDestinationUpdatedHandler(BLEDevice device, BLECharacteristic characteristic) {
  // central disconnected event handler
  if (characteristic.uuid() == "19B10002-E8F2-537E-4F6C-D104768A1215") {
    Serial.print("Navigation ORIGIN received: ");
    headSenseState = HEADSENSE_TASKED;
    areCachedDirectionsDirty = true;  //  Mark the previously fetched directions as stale
  } else if (characteristic.uuid() == "19B10002-E8F2-537E-4F6C-D104768A1216") {
    Serial.print("Navigation DESTINATION received: ");
    headSenseState = HEADSENSE_TASKED;
    areCachedDirectionsDirty = true;  // Mark the previously fetched directions as stale
  } else {
    Serial.print("WARNING - THIS HANDLER DOES NOT HOW TO HANDLE THIS CHARACTERISTIC!");
  }
  Serial.println(static_cast<BLEStringCharacteristic *>(&characteristic)->value());
  prettyPrintHeadSenseState(headSenseState);
}


void sendCommandToPeer(BLEStringCharacteristic characteristic, String message) {
  characteristic.writeValue(message);  // set initial value for this characteristic
}
