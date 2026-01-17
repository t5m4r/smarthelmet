// Wifi and HTTP(S) client
#include <WiFiS3.h>
// install "ArduinoHttpClient" from Library Manager - official library
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <ArduinoBLE.h>
#include "OLED_1in51/OLED_Driver.h"  //fine
#include "OLED_1in51/GUI_paint.h"
#include "OLED_1in51/DEV_Config.h"
#include "OLED_1in51/Debug.h"
#include "OLED_1in51/ImageData.h"
// local header file for secrets stashing; keep this last in the header list
#include "arduino_secrets.h"
#include "configurations.h"
#include "navigation.h"
#include "nav_graphics.h"

int wifiConnectionStatus = WL_IDLE_STATUS;

BLEService smartHelmetService("19B10001-E8F2-537E-4F6C-D104768A1225");  // Bluetooth® Low Energy LED Service
// Bluetooth® Low Energy LED Switch Characteristic - custom 128-bit UUID, read and writable by central
BLEStringCharacteristic p2aCommandCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLEWrite | BLEWriteWithoutResponse | BLENotify, 512);
BLEStringCharacteristic p2aNavOrigin("19B10002-E8F2-537E-4F6C-D104768A1215", BLEWrite, 128);
BLEStringCharacteristic p2aNavDestination("19B10002-E8F2-537E-4F6C-D104768A1216", BLEWrite, 128);
// WiFiSSLClient is a Minimilist TLS client, but is difficult to use with Google APIs since it doesn't support HTTP helper functions/features.
WiFiSSLClient wifiSslClient;
// API hostname is actually set in 03_map_navigation.ino
extern const char kHostname[];

long previousMillis = 0;  // last time the repeat work was done
int current_step = 0;
int last_rendered_step = -1;  // Track which step was last sent/rendered

// Marks a flag that we need to (refresh) directions from the Google API web service
bool areCachedDirectionsDirty = true;
HeadSenseState headSenseState = HEADSENSE_UNINITIALIZED;
JsonDocument routingJson;
JsonArray navSteps;

// OLED orientation: true = landscape (128x64, Rotate=270), false = portrait (64x128, Rotate=0)
bool isLandscapeOLED = false;

extern UBYTE *BlackImage;  // From OLED_1in51.ino

void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(115200);  // Let it rip

  // BT-LE init
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1)
      ;
  }
  BLE.setLocalName("SID-ARD");
  BLE.setDeviceName("SID-ARD");

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
  Serial.println("Board running firmware version: " + fv);

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
  oled_setup(isLandscapeOLED);
  
  // Show welcome screen after OLED init
  drawWelcomeScreen(BlackImage, isLandscapeOLED);
  OLED_1IN51_Display(BlackImage);
  
  prettyPrintHeadSenseState(headSenseState);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (wifiConnectionStatus != WL_CONNECTED) {
    // scan for existing networks:
    Serial.println("Scanning available networks...");
    listNetworks();
  }

  if (wifiConnectionStatus == WL_CONNECTED) {
    printWifiStatus();
  }

  // BT-LE repeat work
  // wait for a Bluetooth® Low Energy central
  BLEDevice central = BLE.central();
  // BT-LE end of repeat work

  if (headSenseState == HEADSENSE_TASKED_COMPLETE) {
    // Show navigation complete screen (only once)
    static bool completeScreenShown = false;
    if (!completeScreenShown) {
      drawCompleteScreen(BlackImage, isLandscapeOLED);
      OLED_1IN51_Display(BlackImage);
      completeScreenShown = true;
      Serial.println("Navigation COMPLETED - showing arrival screen");
    }
    return;
  }

  // Add real work below this line
  // TODO: Figure out where to do .clear() on this presumably large JSON object when it's invalidated as cached data
  const String origin = p2aNavOrigin.value();
  const String destination = p2aNavDestination.value();
  //Serial.println("Origin: " + origin + ", Destination: " + destination);
  //prettyPrintHeadSenseState(headSenseState);
  //Serial.println(areCachedDirectionsDirty);
  if (
    (headSenseState == HEADSENSE_TASKED || headSenseState == HEADSENSE_TASKED_RECALCULATING)
    && (origin != "UNINITIALIZED" && destination != "UNINITIALIZED")
    && areCachedDirectionsDirty) {

    prettyPrintHeadSenseState(headSenseState);

    Serial.println("Refresh directions - deciding to call");
    routingJson = refreshDirections(origin, destination, googleMapsApiKey);
    if (routingJson.isNull()) {
      Serial.println("Refresh directions - NULL results JSON");
    } else {
      Serial.println("Refresh directions - HAPPY JSON results :)");
      areCachedDirectionsDirty = false;
      current_step = 0;
      last_rendered_step = -1;  // Force re-render of step 0
      // Processing of JSON document from HTTP API response
      navSteps = routingJson["routes"][0]["legs"][0]["steps"];
    }
  }

  if (!(headSenseState == HEADSENSE_TASKED || headSenseState == HEADSENSE_TASKED_RECALCULATING)) {
    return;
  }

  // We may be waiting for other user events or Bluetooth communications to fall into place; so we may
  // not have useful work to do in each loop iteration. That's OK - don't fret.
  if (routingJson.isNull()) {
    Serial.println("NOT GOOD: EMPTY ROUTING DIRECTIONS JSON :(");
    return;
  }

  //prettyPrintHeadSenseState(headSenseState);

  if (navSteps.size() == 0) {
    //Serial.println("navSteps ARE ZERO SIZED");
    return;
  }

  //Serial.println("Number of steps is: " + String(navSteps.size()));
  if (current_step >= navSteps.size()) {
    current_step = 0;
    if (headSenseState != HEADSENSE_TASKED_COMPLETE) {
      headSenseState = HEADSENSE_TASKED_COMPLETE;
      sendNavCompleted(p2aCommandCharacteristic);
    }
    return;
  }

  // Only send/render if step changed since last time
  if (current_step == last_rendered_step) {
    return;
  }

  // Extract navigation data from current step
  JsonObject step = navSteps[current_step];

  // Get maneuver (may not exist for first step "Head southwest")
  const char *maneuver = "continue";  // Default if no maneuver specified
  if (step["maneuver"].is<String>()) {
    maneuver = step["maneuver"].as<const char *>();
  }

  // Get distance text
  const char *distanceText = "";
  if (step["distance"]["text"].is<String>()) {
    distanceText = step["distance"]["text"].as<const char *>();
  }

  // Get html_instructions
  const char *htmlInstructions = "";
  if (step["html_instructions"].is<String>()) {
    htmlInstructions = step["html_instructions"].as<const char *>();
  }

  // Log extracted data
  Serial.print("STEP ");
  Serial.print(current_step);
  Serial.print(" -> ");
  Serial.print(maneuver);
  Serial.print(" | ");
  Serial.print(distanceText);
  Serial.print(" | ");
  Serial.println(stripHtmlTags(htmlInstructions));

  sendNavStepToPeer(p2aCommandCharacteristic, current_step, navSteps.size(), maneuver, distanceText, htmlInstructions);

  // Render to OLED using quadrant layout with full navigation info
  drawNavFromApi(maneuver, distanceText, htmlInstructions, BlackImage, isLandscapeOLED);
  OLED_1IN51_Display(BlackImage);

  last_rendered_step = current_step;
  // Add real work ABOVE THIS LINE
}

void incrementNavStep() {
  current_step++;
  Serial.println("NAV STEP SHOULD NOW SHOW: " + String(current_step));
}