// Wifi and HTTP(S) client
#include <WiFiS3.h>
// install "ArduinoHttpClient" from Library Manager - official library
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <ArduinoBLE.h>
#include "OLED_1in51/OLED_Driver.h" //fine
#include "OLED_1in51/GUI_paint.h"
#include "OLED_1in51/DEV_Config.h"
#include "OLED_1in51/Debug.h"
#include "OLED_1in51/ImageData.h"
// local header file for secrets stashing; keep this last in the header list
#include "arduino_secrets.h"
#include "configurations.h"

int wifiConnectionStatus = WL_IDLE_STATUS;

BLEService smartHelmetService("19B10001-E8F2-537E-4F6C-D104768A1225");  // Bluetooth® Low Energy LED Service
// Bluetooth® Low Energy LED Switch Characteristic - custom 128-bit UUID, read and writable by central
BLEStringCharacteristic p2aCommandCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLEWrite, 128);
BLEStringCharacteristic p2aNavOrigin("19B10002-E8F2-537E-4F6C-D104768A1215", BLEWrite, 128);
BLEStringCharacteristic p2aNavDestination("19B10002-E8F2-537E-4F6C-D104768A1216", BLEWrite, 128);
// WiFiSSLClient is a Minimilist TLS client, but is difficult to use with Google APIs since it doesn't support HTTP helper functions/features.
WiFiSSLClient wifiSslClient;
// API hostname is actually set in 03_map_navigation.ino
extern const char kHostname[];

long previousMillis = 0;  // last time the repeat work was done
int current_step = 0;

bool doHttpWork = false;

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

  BLE.setAdvertisedService(smartHelmetService);  // add the service UUID

  p2aCommandCharacteristic.writeValue("UNINITIALIZED");  // set initial value for this characteristic
  p2aNavOrigin.writeValue("UNINITIALIZED");
  p2aNavDestination.writeValue("UNINITIALIZED");

  smartHelmetService.addCharacteristic(p2aCommandCharacteristic);
  smartHelmetService.addCharacteristic(p2aNavOrigin);
  smartHelmetService.addCharacteristic(p2aNavDestination);

  BLE.addService(smartHelmetService);  // Add the battery service
  // assign event handlers for connected, disconnected to peripheral
  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // Characteristic-level events can be one of BLESubscribed, BLEUnsubscribed, BLERead, BLEWritten
  p2aCommandCharacteristic.setEventHandler(BLEUpdated, p2aCommandUpdatedHandler);
  // Is it OK to use the same event handler for both Origin and Destination characteristics? How do tell which one was written to?
  p2aNavOrigin.setEventHandler(BLEUpdated, p2aOriginOrDestinationUpdatedHandler);
  p2aNavDestination.setEventHandler(BLEUpdated, p2aOriginOrDestinationUpdatedHandler);
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
  oled_setup();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (wifiConnectionStatus != WL_CONNECTED) {
    // scan for existing networks:
    Serial.println("Scanning available networks...");
    listNetworks();
  }

  //byte mac[6];
  //WiFi.macAddress(mac);
  //Serial.print("Board's MAC Address: ");
  //printMacAddress(mac);
  if (wifiConnectionStatus == WL_CONNECTED) {
    printWifiStatus();
  }

  // BT-LE repeat work
  // wait for a Bluetooth® Low Energy central
  BLEDevice central = BLE.central();
  // BT-LE end of repeat work

  // Add real work below this line

  int err = 0;

  if (doHttpWork) {
    // This is the official project's ArduinoHttpClient library from the Library Manager
    HttpClient httpsClient = HttpClient(wifiSslClient, kHostname, HttpClient::kHttpsPort); 
    const String origin = p2aNavOrigin.value();
    const String destination = p2aNavDestination.value();
    err = httpsClient.get(buildGoogleNavigationUrlPath(origin, destination, googleMapsApiKey));
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

        JsonDocument jsonDoc;
        unsigned long timeoutStart = millis();
        // Whilst we haven't timed out & haven't reached the end of the body
        while ((httpsClient.connected() || httpsClient.available()) && (!httpsClient.endOfBodyReached()) && ((millis() - timeoutStart) < kNetworkTimeout)) {
          if (httpsClient.available()) {
            JsonDocument filter;
            JsonObject filter_routes_0_legs_0_steps_0 = filter["routes"][0]["legs"][0]["steps"].add<JsonObject>();
            filter_routes_0_legs_0_steps_0["html_instructions"] = true;
            filter_routes_0_legs_0_steps_0["maneuver"] = true;
            // Now we've got to the HTTP response body, so we can feed it to a JSON processor/parser.
            DeserializationError error = deserializeJson(jsonDoc, httpsClient, DeserializationOption::Filter(filter));
            switch (error.code()) {
              case DeserializationError::Ok:
                Serial.println(String("Deserialization of JSON succeeded. "));
                httpsClient.stop();  // This marks as this request-request pair as successfully done, and effectively helps us break out from the while() loop above.
                break;
              case DeserializationError::EmptyInput:
                Serial.println("Deserialization of JSON - No data to parse");  // We don't .stop() the HTTP request in this case, as we may need to wait for data to be available.
                break;
              case DeserializationError::InvalidInput:
              case DeserializationError::NoMemory:
                Serial.print("Deserializarion error: ");
                Serial.println(error.code());
              default:
                Serial.println("Deserialization failed! " + error.code());
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

        // Processing of JSON document from HTTP API response
        JsonArray navSteps = jsonDoc["routes"][0]["legs"][0]["steps"];
        Serial.println("Number of steps is: " + String(navSteps.size()));
        if (current_step >= navSteps.size()) {
          doHttpWork = false;
          //Serial.println("All steps traversed, stopping more HTTP navigation");
          current_step = 0;
          return;
        }
        Serial.print("STEP " + String(current_step) + " -> HTML instruction ");
        const char* html_step = navSteps[current_step]["html_instructions"].as<const char*>();
        const char* instructions_text = stripHtmlTags(html_step).c_str();
        Serial.println(instructions_text);
        draw_step(instructions_text);

        if (navSteps[current_step]["maneuver"].is<String>()) {
          const char* maneuver = navSteps[current_step]["maneuver"];
          Serial.print("STEP " + String(current_step) + " -> Maneuver ");
          Serial.println(maneuver);
        }
        current_step++;
        navSteps.clear();
        jsonDoc.clear();
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
  //const unsigned long sleepTime = 3000;
  //Serial.println("Me lazy, sleeping for " + String(sleepTime / 1000) + "s");
  //delay(sleepTime);
}
