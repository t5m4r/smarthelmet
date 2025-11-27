
#include "URLEncoder.h"

// Name of the server we want to connect to
const char kHostname[] = "maps.googleapis.com";

// Google Navigation API path with origin and destination
static String buildGoogleNavigationUrlPath(String startPoint, String endPoint, String apiKey) { 
  return "/maps/api/directions/json?origin=" + startPoint + "&destination=" + endPoint + "&key=" + apiKey;
}