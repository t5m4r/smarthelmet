
#include "URLEncoder.h"

// Name of the server we want to connect to
const char kHostname[] = "maps.googleapis.com";

// Google Navigation API path with origin and destination
static String buildGoogleNavigationUrlPath(String startPoint, String endPoint) { 
  return "/maps/api/directions/json?origin=" + startPoint + "&destination=" + endPoint + "&key=AIzaSyBn8t8P5AIw05zOd3O2r1YePbyXbbkHn4U";
}