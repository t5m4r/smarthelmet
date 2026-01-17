
#include "URLEncoder.h"

// Send navigation step info to the MWA via BLE notification
// Format: INFO::NAVSTEP:current:total:maneuver:distance:instruction
static void sendNavStepToPeer(BLEStringCharacteristic &characteristic, int currentStep, int totalSteps, const char *maneuver, const char *distance, const char *instruction) {
  String informationalCommand = "INFO::NAVSTEP:" + String(currentStep) + ":" + String(totalSteps) + ":" + String(maneuver) + ":" + String(distance) + ":" + String(instruction);
  sendCommandToPeer(characteristic, informationalCommand);
}

static void sendNavCompleted(BLEStringCharacteristic &characteristic) {
  String informationalCommand = "INFO::NAVCOMPLETED";
  sendCommandToPeer(characteristic, informationalCommand);
}


// Name of the server we want to connect to
const char kHostname[] = "maps.googleapis.com";
// Number of milliseconds to wait without receiving any data before we give up
const int kNetworkTimeout = 30 * 1000;

// Google Navigation API path with origin and destination
static String buildGoogleNavigationUrlPath(String startPoint, String endPoint, String apiKey) {
  return "/maps/api/directions/json?alternatives=false&origin=" + startPoint + "&destination=" + endPoint + "&key=" + apiKey;
}

static JsonDocument refreshDirections(const String &origin, const String &destination, const String &apiKey) {
  JsonDocument jsonDoc;
  // This is the official project's ArduinoHttpClient library from the Library Manager
  HttpClient httpsClient = HttpClient(wifiSslClient, kHostname, HttpClient::kHttpsPort);
  const String fullUrl = buildGoogleNavigationUrlPath(origin, destination, apiKey);
  Serial.println(fullUrl);
  int err = httpsClient.get(fullUrl);
  if (err == 0) {
    int httpResponseCode = httpsClient.responseStatusCode();
    if (httpResponseCode >= 0) {
      Serial.println("HTTP response code: " + String(httpResponseCode));

      // Usually you'd check that the response code is 200 or a
      // similar "success" code (200-299) before carrying on,
      // but we'll print out whatever response we get
      // If you are interesting in the response headers, you
      // can read them here:
      // while(http.headerAvailable())
      //{
      //  String headerName = http.readHeaderName();
      //  String headerValue = http.readHeaderValue();
      //}
      int bodyLen = httpsClient.contentLength();
      Serial.println("HTTP response's content length is: " + String(bodyLen) + " bytes");
      unsigned long timeoutStart = millis();
      // Whilst we haven't timed out & haven't reached the end of the body
      while ((httpsClient.connected() || httpsClient.available()) && (!httpsClient.endOfBodyReached()) && ((millis() - timeoutStart) < kNetworkTimeout)) {
        if (httpsClient.available()) {
          JsonDocument filter;
          JsonObject filter_routes_0_legs_0_steps_0 = filter["routes"][0]["legs"][0]["steps"].add<JsonObject>();
          filter_routes_0_legs_0_steps_0["html_instructions"] = true;
          filter_routes_0_legs_0_steps_0["maneuver"] = true;
          filter_routes_0_legs_0_steps_0["distance"]["text"] = true;
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
    } else {
      Serial.println("Error getting HTTP response, code " + httpResponseCode);
    }
  } else {
    Serial.println(String("HTTP request couldn't be sent out: ") + err);
  }
  httpsClient.stop();
  return jsonDoc;
}
