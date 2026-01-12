// 04_smarthelmet_utils.ino - General utilities for SmartHelmet project
// Text processing, formatting, and helper functions

#include <Arduino.h>
#include <string.h>

// =============================================================================
// HTML/TEXT PROCESSING UTILITIES
// =============================================================================

// Strip HTML tags from Google Directions API html_instructions
// Handles: <b>, </b>, <div...>, </div>, Unicode escapes (\u003c = <, \u003e = >)
// Returns: Cleaned plain text string
String stripHtmlTags(const char* html) {
  if (html == NULL) return "";
  
  String result = "";
  bool inTag = false;
  int len = strlen(html);
  
  for (int i = 0; i < len; i++) {
    char c = html[i];
    
    // Handle Unicode escape sequences (\u003c = <, \u003e = >)
    if (c == '\\' && i + 5 < len && html[i+1] == 'u') {
      char code[5] = {html[i+2], html[i+3], html[i+4], html[i+5], '\0'};
      if (strcmp(code, "003c") == 0 || strcmp(code, "003C") == 0) {
        // \u003c = '<' - start of tag
        inTag = true;
        i += 5;
        continue;
      } else if (strcmp(code, "003e") == 0 || strcmp(code, "003E") == 0) {
        // \u003e = '>' - end of tag
        inTag = false;
        i += 5;
        continue;
      }
    }
    
    // Handle literal < and >
    if (c == '<') {
      inTag = true;
      continue;
    }
    if (c == '>') {
      inTag = false;
      continue;
    }
    
    // Skip characters inside tags
    if (inTag) continue;
    
    // Handle &amp; -> &
    if (c == '&' && i + 4 < len) {
      if (strncmp(&html[i], "&amp;", 5) == 0) {
        result += '&';
        i += 4;
        continue;
      }
    }
    
    // Append regular characters
    result += c;
  }
  
  // Trim leading/trailing whitespace
  result.trim();
  
  return result;
}

// Format distance string: remove space between number and unit
// "0.6 km" -> "0.6km", "76 m" -> "76m"
String formatDistance(const char* distText) {
  if (distText == NULL) return "";
  
  String result = "";
  int len = strlen(distText);
  
  for (int i = 0; i < len; i++) {
    char c = distText[i];
    // Skip spaces
    if (c != ' ') {
      result += c;
    }
  }
  
  return result;
}

// Word-wrap text into two lines with max character limit per line
// Breaks on word boundaries when possible
void wrapText(const char* text, char* line1, char* line2, int maxChars) {
  if (text == NULL || line1 == NULL || line2 == NULL) return;
  
  line1[0] = '\0';
  line2[0] = '\0';
  
  int len = strlen(text);
  if (len == 0) return;
  
  // If text fits on one line, just copy it
  if (len <= maxChars) {
    strncpy(line1, text, maxChars);
    line1[maxChars] = '\0';
    return;
  }
  
  // Find best break point (last space before maxChars)
  int breakPoint = maxChars;
  for (int i = maxChars - 1; i > 0; i--) {
    if (text[i] == ' ') {
      breakPoint = i;
      break;
    }
  }
  
  // Copy first line
  strncpy(line1, text, breakPoint);
  line1[breakPoint] = '\0';
  
  // Skip the space at break point
  int line2Start = breakPoint;
  while (text[line2Start] == ' ' && line2Start < len) {
    line2Start++;
  }
  
  // Copy second line (truncate if needed)
  int remaining = len - line2Start;
  int copyLen = (remaining > maxChars) ? maxChars : remaining;
  strncpy(line2, &text[line2Start], copyLen);
  line2[copyLen] = '\0';
}

// Abbreviate common road terms to save display space
// "Road" -> "Rd", "Street" -> "St", etc.
String abbreviateRoadName(const char* text) {
  if (text == NULL) return "";
  
  String result = String(text);
  
  // Common abbreviations
  result.replace("Road", "Rd");
  result.replace("Street", "St");
  result.replace("Avenue", "Ave");
  result.replace("Boulevard", "Blvd");
  result.replace("Highway", "Hwy");
  result.replace("Expressway", "Expy");
  result.replace("toward", "to");
  result.replace("continue", "cont");
  result.replace("Continue", "Cont");
  
  return result;
}
