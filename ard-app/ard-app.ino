// Wifi and HTTP(S) client
#include <WiFiS3.h>
// install "ArduinoHttpClient" from Library Manager - official library
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <ArduinoBLE.h>

// local header file for secrets stashing; keep this last in the header list
#include "arduino_secrets.h"

#include "configurations.h"

int wifiConnectionStatus = WL_IDLE_STATUS;

BLEService tusharHeadsUpDisplayService("19B10001-E8F2-537E-4F6C-D104768A1225");  // Bluetooth® Low Energy LED Service
// Bluetooth® Low Energy LED Switch Characteristic - custom 128-bit UUID, read and writable by central
BLEStringCharacteristic textCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite | BLENotify, 100);

// WiFiSSLClient is a Minimilist TLS client, but is difficult to use with Google APIs since it doesn't support HTTP helper functions/features.
WiFiSSLClient wifiSslClient;
// API hostname is actually set in 03_map_navigation.ino
extern const char kHostname[];

long previousMillis = 0;  // last time the repeat work was done


void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(115200);  // Let it rip
  while (!Serial)
    ;

  // BT-LE init
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1)
      ;
  }
  BLE.setLocalName("T5M4R-ARD");
  BLE.setDeviceName("T5M4R-ARD");

  BLE.setAdvertisedService(tusharHeadsUpDisplayService);  // add the service UUID

  textCharacteristic.writeValue("UNINIT VALUE");  // set initial value for this characteristic
  tusharHeadsUpDisplayService.addCharacteristic(textCharacteristic);
  BLE.addService(tusharHeadsUpDisplayService);  // Add the battery service
  // assign event handlers for connected, disconnected to peripheral
  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // Characteristic-level events can be one of BLESubscribed, BLEUnsubscribed, BLERead, BLEWritten
  //textCharacteristic.setEventHandler(BLESubscribed, bleCharacteristicChangedHandler);
  textCharacteristic.setEventHandler(BLEWritten, bleCharacteristicWrittenHandler);
  textCharacteristic.setEventHandler(BLEUpdated, bleCharacteristicUpdatedHandler);

  // BT-LE init end

  /* Start advertising Bluetooth® Low Energy.  It will start continuously transmitting Bluetooth® Low Energy
     advertising packets and will be visible to remote Bluetooth® Low Energy central devices
     until it receives a new connection */

  // start advertising
  BLE.advertise();

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }
  Serial.println("Board running Wifi firmware version: " + fv);

  // attempt to connect to WiFi network:
  while (wifiConnectionStatus != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    wifiConnectionStatus = WiFi.begin(ssid, pass);
    // wait 5 seconds for connection:
    delay(5000);
  }

  if (wifiConnectionStatus == WL_CONNECTED) {
    printWifiStatus();
  }

  Serial.println(buildGoogleNavigationUrlPath("560102", "560103"));
}

void loop() {
  // put your main code here, to run repeatedly:
  if (wifiConnectionStatus != WL_CONNECTED) {
    // scan for existing networks:
    Serial.println("Scanning available networks...");
    listNetworks();
  }

  byte mac[6];
  WiFi.macAddress(mac);
  Serial.println();
  Serial.print("Board's MAC Address: ");
  printMacAddress(mac);
  if (wifiConnectionStatus == WL_CONNECTED) {
    printWifiStatus();
  }


  // BT-LE repeat work
  // wait for a Bluetooth® Low Energy central
  BLEDevice central = BLE.central();
  // if a central is connected to the peripheral:
  if (central) {
    Serial.print("Connected to central: ");
    // print the central's BT address:
    Serial.println(central.address());
    textCharacteristic.writeValue("YOLO TUSHAR! FIRST");
    // check the battery level every 200 ms
    // while the central is connected:
    while (central.connected()) {
      long currentMillis = millis();
      // if 200 ms have passed, check the battery level:
      if (currentMillis - previousMillis >= 200) {
        previousMillis = currentMillis;
        textCharacteristic.writeValue("YOLO TUSHAR! AGAIN :)");
      }
    }
    // when the central disconnects, turn off the LED:
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
  // BT-LE end of repeat work

  // Add real work below this line

  int err = 0;
  bool doHttpWork = true;

  if (doHttpWork) {
    // This is the official project's ArduinoHttpClient library from the Library Manager
    HttpClient httpsClient = HttpClient(wifiSslClient, kHostname, HttpClient::kHttpsPort);
    err = httpsClient.get(buildGoogleNavigationUrlPath("560102", "560103"));
    if (err == 0) {
      int httpResponseCode = httpsClient.responseStatusCode();
      if (httpResponseCode >= 0) {
        Serial.println("HTTP response code: " + String(httpResponseCode));

        // Usually you'd check that the response code is 200 or a
        // similar "success" code (200-299) before carrying on,
        // but we'll print out whatever response we get
        // If you are interesting in the response headers, you
        // can read them here:
        //while(http.headerAvailable())
        //{
        //  String headerName = http.readHeaderName();
        //  String headerValue = http.readHeaderValue();
        //}
        int bodyLen = httpsClient.contentLength();
        Serial.println("HTTP response's content length is: " + String(bodyLen) + " bytes");

        // Now we've got to the body, so we can print it out
        unsigned long timeoutStart = millis();
        char c;
        // Whilst we haven't timed out & haven't reached the end of the body
        while ((httpsClient.connected() || httpsClient.available()) && (!httpsClient.endOfBodyReached()) && ((millis() - timeoutStart) < kNetworkTimeout)) {
          if (httpsClient.available()) {
            JsonDocument jsonDoc;
            DeserializationError error = deserializeJson(jsonDoc, httpsClient);
            switch (error.code()) {
              case DeserializationError::Ok:
                Serial.println(String("Deserialization of JSON succeeded. ") + String("API Response: ") + String(jsonDoc["ip"].as<const char*>()));
                httpsClient.stop();  // This marks as this request-request pair as successfully done, and effectively helps us break out from the while() loop above.
                break;
              case DeserializationError::InvalidInput:
                Serial.println("Deserialization of JSON - Invalid input!");
                break;
              case DeserializationError::NoMemory:
                Serial.println("Deserialization of JSON - Not enough memory");
                httpsClient.stop();  // This marks as this request-request pair as successfully done, and effectively helps us break out from the while() loop above.
                break;
              case DeserializationError::EmptyInput:
                Serial.println("Deserialization of JSON - No data to parse");  // We don't .stop() the HTTP request in this case, as we may need to wait for data to be available.
                break;
              default:
                Serial.println("Deserialization failed");
                httpsClient.stop();  // This marks as this request-request pair as successfully done, and effectively helps us break out from the while() loop above.
                break;
            }
            // We read something, reset the timeout counter
            timeoutStart = millis();
          } else {
            // We haven't got any data, so let's pause to allow some to arrive
            Serial.println("Http data stream isn't ready yet, sleeping for " + kNetworkDelay / 1000);
            delay(kNetworkDelay);
          }
        }  // While loop for waiting for, plus reading the response of a single API request/response
      } else {
        Serial.println("Error getting HTTP response, code " + httpResponseCode);
      }
    } else {
      Serial.println(String("HTTP request couldn't be sent out: ") + err);
    }
    httpsClient.stop();
  }  // end of single HTTP/API request block

  // Add real work ABOVE THIS LINE

  // Idle time
  const unsigned long sleepTime = 10000;
  Serial.println("Me lazy, sleeping for " + String(sleepTime / 1000) + "s");
  delay(sleepTime);
}
