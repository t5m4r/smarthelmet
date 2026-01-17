// 06_nav_graphics_landscape.ino - Navigation graphics for LANDSCAPE mode
// Waveshare OLED_1in51 library (128x64 logical with Rotate=270)
// Landscape = 128 wide × 64 tall
//
// Dependencies (must compile before this file):
//   04_smarthelmet_utils.ino - Text processing utilities
//   05_nav_data.ino - Shared nav arrays and parseNavInstruction()

#include "OLED_1in51/GUI_Paint.h"
#include "navigation.h"

// =============================================================================
// LANDSCAPE MODE DISPLAY CONSTANTS
// =============================================================================

// Display dimensions (landscape: wide × short)
#define LANDSCAPE_WIDTH   128
#define LANDSCAPE_HEIGHT  64

// Font dimensions
#define FONT16_WIDTH  11
#define FONT16_HEIGHT 16
#define FONT12_WIDTH  7
#define FONT12_HEIGHT 12
#define FONT8_WIDTH   5
#define FONT8_HEIGHT  8

// -----------------------------------------------------------------------------
// Quadrant Layout Constants (50/50 vertical split, 1/3-2/3 top horizontal)
// With divider lines - fonts reduced by one notch to fit
// -----------------------------------------------------------------------------
// Top half: 0-30 (31px) + 1px divider at y=31
//   Left 1/3 (0-42): Arrow icon + 1px divider at x=43
//   Right 2/3 (44-127): Distance + label
// Bottom half: 32-63 (32px)
//   Full width: 2-line instruction text

#define LANDSCAPE_TOP_HEIGHT     31
#define LANDSCAPE_DIVIDER_Y      31   // Horizontal divider line
#define LANDSCAPE_BOTTOM_Y       33   // Start of bottom section (after divider)

// Top-left quadrant (arrow) - now using Font12
#define LANDSCAPE_ARROW_X        0
#define LANDSCAPE_ARROW_WIDTH    42
#define LANDSCAPE_DIVIDER_X      43   // Vertical divider line
#define LANDSCAPE_ARROW_ROW1_Y   2
#define LANDSCAPE_ARROW_ROW2_Y   15   // Font12 is 12px tall

// Top-right quadrant (distance + label) - now using Font12/Font8
#define LANDSCAPE_INFO_X         46
#define LANDSCAPE_DISTANCE_Y     2
#define LANDSCAPE_LABEL_Y        16

// Bottom section (full-width instructions) - now using Font8
#define LANDSCAPE_INSTR_X        2
#define LANDSCAPE_INSTR_LINE1_Y  35
#define LANDSCAPE_INSTR_LINE2_Y  48
#define LANDSCAPE_INSTR_MAX_CHARS 24  // More chars with Font8 (5px wide)

// =============================================================================
// EXTERN DECLARATIONS (from 05_nav_data.ino)
// =============================================================================

extern const char* NAV_ROW1[];
extern const char* NAV_ROW2[];
extern const char* NAV_ROW1_COMPACT[];
extern const char* NAV_ROW2_COMPACT[];
extern const char* NAV_LABELS[];
extern const char* NAV_NAMES[];

// Status screen ASCII art
extern const char* HEADSENSE_ROW1;
extern const char* HEADSENSE_ROW2;
extern const char* HEADSENSE_ROW3;
extern const char* COMPLETE_ROW1;
extern const char* COMPLETE_ROW2;
extern const char* COMPLETE_ROW3;

// =============================================================================
// LANDSCAPE MODE HELPER FUNCTIONS
// =============================================================================

// Center text horizontally within full display width
UWORD landscapeCenterX(const char* text, UWORD fontWidth) {
  UWORD textWidth = fontWidth * strlen(text);
  if (textWidth >= LANDSCAPE_WIDTH) return 0;
  return (LANDSCAPE_WIDTH - textWidth) / 2;
}

// Center text within left 43px region (arrow quadrant)
UWORD landscapeCenterLeftX(const char* text, UWORD fontWidth) {
  UWORD textWidth = fontWidth * strlen(text);
  if (textWidth >= LANDSCAPE_ARROW_WIDTH) return 0;
  return (LANDSCAPE_ARROW_WIDTH - textWidth) / 2;
}

// =============================================================================
// LANDSCAPE MODE DRAWING FUNCTIONS
// =============================================================================

// Simple centered layout: Arrow + Label (original implementation)
void landscapeDrawNavInstruction(NavInstruction nav, UBYTE* image) {
  if (nav >= NAV_COUNT) nav = NAV_NONE;
  
  Serial.print("[NAV] landscapeDrawNavInstruction nav=");
  Serial.print(nav);
  Serial.print(" label='");
  Serial.print(NAV_LABELS[nav]);
  Serial.print("' image=0x");
  Serial.println((unsigned long)image, HEX);
  
  if (image == NULL) {
    Serial.println("[NAV] ERROR: image buffer is NULL!");
    return;
  }
  
  Paint_SelectImage(image);
  Paint_Clear(BLACK);
  
  // Layout: centered arrow + label
  UWORD arrowX = landscapeCenterX(NAV_ROW1[nav], FONT16_WIDTH);
  UWORD labelX = landscapeCenterX(NAV_LABELS[nav], FONT12_WIDTH);
  
  Paint_DrawString_EN(arrowX, 4, NAV_ROW1[nav], &Font16, WHITE, WHITE);
  Paint_DrawString_EN(arrowX, 22, NAV_ROW2[nav], &Font16, WHITE, WHITE);
  Paint_DrawString_EN(labelX, 44, NAV_LABELS[nav], &Font12, WHITE, WHITE);
}

// Compact arrow only (no label)
void landscapeDrawNavArrowOnly(NavInstruction nav, UBYTE* image) {
  if (nav >= NAV_COUNT) nav = NAV_NONE;
  
  Paint_SelectImage(image);
  Paint_Clear(BLACK);
  
  UWORD arrowX = landscapeCenterX(NAV_ROW1[nav], FONT16_WIDTH);
  
  Paint_DrawString_EN(arrowX, 14, NAV_ROW1[nav], &Font16, WHITE, WHITE);
  Paint_DrawString_EN(arrowX, 34, NAV_ROW2[nav], &Font16, WHITE, WHITE);
}

// Arrow + distance at bottom
void landscapeDrawNavWithDistance(NavInstruction nav, const char* distance, UBYTE* image) {
  if (nav >= NAV_COUNT) nav = NAV_NONE;
  
  Paint_SelectImage(image);
  Paint_Clear(BLACK);
  
  UWORD arrowX = landscapeCenterX(NAV_ROW1[nav], FONT16_WIDTH);
  UWORD distX = landscapeCenterX(distance, FONT16_WIDTH);
  
  Paint_DrawString_EN(arrowX, 2, NAV_ROW1[nav], &Font16, WHITE, WHITE);
  Paint_DrawString_EN(arrowX, 20, NAV_ROW2[nav], &Font16, WHITE, WHITE);
  Paint_DrawString_EN(distX, 44, distance, &Font16, WHITE, WHITE);
}

// -----------------------------------------------------------------------------
// QUADRANT LAYOUT: 50/50 vertical, 1/3-2/3 top horizontal
// With divider lines - fonts reduced by one notch
// -----------------------------------------------------------------------------
// Top-left: Arrow icon (compact, Font12)
// Top-right: Distance (Font12) + label (Font8)
// Dividers: Solid lines at x=43 (vertical) and y=31 (horizontal)
// Bottom: Full-width 2-line instruction (Font8)

void landscapeDrawNavQuadrant(
    NavInstruction nav,
    const char* distance,
    const char* instrLine1,
    const char* instrLine2,
    UBYTE* image
) {
  if (nav >= NAV_COUNT) nav = NAV_NONE;
  
  if (image == NULL) {
    Serial.println("[NAV] ERROR: image buffer is NULL!");
    return;
  }
  
  Paint_SelectImage(image);
  Paint_Clear(BLACK);
  
  // --- Top-left quadrant: Compact arrow (Font12) ---
  UWORD arrowX = landscapeCenterLeftX(NAV_ROW1_COMPACT[nav], FONT12_WIDTH);
  Paint_DrawString_EN(arrowX, LANDSCAPE_ARROW_ROW1_Y, NAV_ROW1_COMPACT[nav], &Font12, WHITE, WHITE);
  Paint_DrawString_EN(arrowX, LANDSCAPE_ARROW_ROW2_Y, NAV_ROW2_COMPACT[nav], &Font12, WHITE, WHITE);
  
  // --- Top-right quadrant: Distance (Font12) + Label (Font8) ---
  Paint_DrawString_EN(LANDSCAPE_INFO_X, LANDSCAPE_DISTANCE_Y, distance, &Font12, WHITE, WHITE);
  Paint_DrawString_EN(LANDSCAPE_INFO_X, LANDSCAPE_LABEL_Y, NAV_LABELS[nav], &Font8, WHITE, WHITE);
  
  // --- Divider lines ---
  // Vertical divider between top-left and top-right (x=43, from y=0 to y=30)
  Paint_DrawLine(LANDSCAPE_DIVIDER_X, 0, LANDSCAPE_DIVIDER_X, LANDSCAPE_TOP_HEIGHT - 1, 
                 WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
  // Horizontal divider between top and bottom (y=31, from x=0 to x=127)
  Paint_DrawLine(0, LANDSCAPE_DIVIDER_Y, LANDSCAPE_WIDTH - 1, LANDSCAPE_DIVIDER_Y, 
                 WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
  
  // --- Bottom section: Full-width instructions (Font8) ---
  if (instrLine1 && strlen(instrLine1) > 0) {
    Paint_DrawString_EN(LANDSCAPE_INSTR_X, LANDSCAPE_INSTR_LINE1_Y, instrLine1, &Font8, WHITE, WHITE);
  }
  if (instrLine2 && strlen(instrLine2) > 0) {
    Paint_DrawString_EN(LANDSCAPE_INSTR_X, LANDSCAPE_INSTR_LINE2_Y, instrLine2, &Font8, WHITE, WHITE);
  }
  
  Serial.print("[NAV] landscapeDrawNavQuadrant: ");
  Serial.print(NAV_LABELS[nav]);
  Serial.print(" dist=");
  Serial.println(distance);
}

// -----------------------------------------------------------------------------
// HIGH-LEVEL CONVENIENCE FUNCTION
// -----------------------------------------------------------------------------
// Processes raw Google Directions API data and renders full quadrant layout

void landscapeDrawNavFromApi(
    const char* maneuver,         // "turn-left", "keep-right", etc.
    const char* distanceText,     // "0.6 km"
    const char* htmlInstructions, // Raw HTML instructions from API
    UBYTE* image
) {
  // Parse maneuver to enum
  NavInstruction nav = parseNavInstruction(maneuver);
  
  // Format distance (remove space)
  String dist = formatDistance(distanceText);
  
  // Strip HTML and abbreviate instructions
  String cleanInstr = stripHtmlTags(htmlInstructions);
  cleanInstr = abbreviateRoadName(cleanInstr.c_str());
  
  // Word-wrap into two lines
  char line1[LANDSCAPE_INSTR_MAX_CHARS + 1];
  char line2[LANDSCAPE_INSTR_MAX_CHARS + 1];
  wrapText(cleanInstr.c_str(), line1, line2, LANDSCAPE_INSTR_MAX_CHARS);
  
  // Draw the quadrant layout
  landscapeDrawNavQuadrant(nav, dist.c_str(), line1, line2, image);
}

// =============================================================================
// LANDSCAPE STATUS SCREENS
// =============================================================================

// Welcome screen: Namaste hands + "Waiting" message
void landscapeDrawWelcomeScreen(UBYTE* image) {
  if (image == NULL) {
    Serial.println("[NAV] ERROR: image buffer is NULL!");
    return;
  }
  
  Paint_SelectImage(image);
  Paint_Clear(BLACK);
  
  // HeadSense ASCII art box - centered (Font12 for 14 chars)
  UWORD brandX = landscapeCenterX(HEADSENSE_ROW1, FONT12_WIDTH);
  Paint_DrawString_EN(brandX, 4, HEADSENSE_ROW1, &Font12, WHITE, WHITE);
  Paint_DrawString_EN(brandX, 18, HEADSENSE_ROW2, &Font12, WHITE, WHITE);
  Paint_DrawString_EN(brandX, 32, HEADSENSE_ROW3, &Font12, WHITE, WHITE);
  
  // "Waiting.." text at bottom
  const char* msg = "Waiting..";
  UWORD msgX = landscapeCenterX(msg, FONT8_WIDTH);
  Paint_DrawString_EN(msgX, 50, msg, &Font8, WHITE, WHITE);
  
  Serial.println("[NAV] landscapeDrawWelcomeScreen");
}

// Destination prompt screen: "What's your Destination?" after BLE pairing
void landscapeDrawDestinationPrompt(UBYTE* image) {
  if (image == NULL) {
    Serial.println("[NAV] ERROR: image buffer is NULL!");
    return;
  }
  
  Paint_SelectImage(image);
  Paint_Clear(BLACK);
  
  // Three lines centered
  const char* line1 = "What's";
  const char* line2 = "your";
  const char* line3 = "Destination?";
  
  UWORD x1 = landscapeCenterX(line1, FONT16_WIDTH);
  UWORD x2 = landscapeCenterX(line2, FONT16_WIDTH);
  UWORD x3 = landscapeCenterX(line3, FONT12_WIDTH);
  
  Paint_DrawString_EN(x1, 4, line1, &Font16, WHITE, WHITE);
  Paint_DrawString_EN(x2, 24, line2, &Font16, WHITE, WHITE);
  Paint_DrawString_EN(x3, 46, line3, &Font12, WHITE, WHITE);
  
  Serial.println("[NAV] landscapeDrawDestinationPrompt");
}

// Navigation complete screen: HeadSense branding + "Arrived!" message
void landscapeDrawCompleteScreen(UBYTE* image) {
  if (image == NULL) {
    Serial.println("[NAV] ERROR: image buffer is NULL!");
    return;
  }
  
  Paint_SelectImage(image);
  Paint_Clear(BLACK);
  
  // HeadSense ASCII art box - centered (Font12 for 14 chars)
  UWORD brandX = landscapeCenterX(HEADSENSE_ROW1, FONT12_WIDTH);
  Paint_DrawString_EN(brandX, 4, HEADSENSE_ROW1, &Font12, WHITE, WHITE);
  Paint_DrawString_EN(brandX, 18, HEADSENSE_ROW2, &Font12, WHITE, WHITE);
  Paint_DrawString_EN(brandX, 32, HEADSENSE_ROW3, &Font12, WHITE, WHITE);
  
  // "Arrived!" text at bottom
  const char* msg = "Arrived!";
  UWORD msgX = landscapeCenterX(msg, FONT8_WIDTH);
  Paint_DrawString_EN(msgX, 50, msg, &Font8, WHITE, WHITE);
  
  Serial.println("[NAV] landscapeDrawCompleteScreen");
}

// =============================================================================
// ORIENTATION-AWARE WRAPPER FUNCTIONS
// =============================================================================
// Pass isLandscape=true for 128x64, false for 64x128

void drawNavInstruction(NavInstruction nav, UBYTE* image, bool isLandscape) {
  if (isLandscape) {
    landscapeDrawNavInstruction(nav, image);
  } else {
    portraitDrawNavInstruction(nav, image);
  }
}

void drawNavArrowOnly(NavInstruction nav, UBYTE* image, bool isLandscape) {
  if (isLandscape) {
    landscapeDrawNavArrowOnly(nav, image);
  } else {
    portraitDrawNavArrowOnly(nav, image);
  }
}

void drawNavWithDistance(NavInstruction nav, const char* distance, UBYTE* image, bool isLandscape) {
  if (isLandscape) {
    landscapeDrawNavWithDistance(nav, distance, image);
  } else {
    portraitDrawNavWithDistance(nav, distance, image);
  }
}

// High-level API wrapper with orientation flag
void drawNavFromApi(
    const char* maneuver,
    const char* distanceText,
    const char* htmlInstructions,
    UBYTE* image,
    bool isLandscape
) {
  if (isLandscape) {
    landscapeDrawNavFromApi(maneuver, distanceText, htmlInstructions, image);
  } else {
    portraitDrawNavFromApi(maneuver, distanceText, htmlInstructions, image);
  }
}

// Welcome screen wrapper
void drawWelcomeScreen(UBYTE* image, bool isLandscape) {
  if (isLandscape) {
    landscapeDrawWelcomeScreen(image);
  } else {
    portraitDrawWelcomeScreen(image);
  }
}

// Navigation complete screen wrapper
void drawCompleteScreen(UBYTE* image, bool isLandscape) {
  if (isLandscape) {
    landscapeDrawCompleteScreen(image);
  } else {
    portraitDrawCompleteScreen(image);
  }
}

// Destination prompt screen wrapper
void drawDestinationPrompt(UBYTE* image, bool isLandscape) {
  if (isLandscape) {
    landscapeDrawDestinationPrompt(image);
  } else {
    portraitDrawDestinationPrompt(image);
  }
}
