// 08_nav_graphics_portrait.ino - Navigation graphics for PORTRAIT mode
// Waveshare OLED_1in51 library (64x128 logical with Rotate=0 or 180)
// Portrait = 64 wide × 128 tall
//
// Dependencies (must compile before this file):
//   04_smarthelmet_utils.ino - Text processing utilities
//   05_nav_data.ino - Shared nav arrays and parseNavInstruction()

#include "OLED_1in51/GUI_Paint.h"
#include "navigation.h"

// =============================================================================
// PORTRAIT MODE DISPLAY CONSTANTS
// =============================================================================

// Display dimensions (portrait: narrow × tall)
#define PORTRAIT_WIDTH    64
#define PORTRAIT_HEIGHT   128

// Font dimensions (same as landscape, repeated for clarity)
#define FONT16_WIDTH  11
#define FONT16_HEIGHT 16
#define FONT12_WIDTH  7
#define FONT12_HEIGHT 12
#define FONT8_WIDTH   5
#define FONT8_HEIGHT  8

// -----------------------------------------------------------------------------
// Vertical Stacking Layout Constants
// -----------------------------------------------------------------------------
// Top band (0-39): Arrow icon (40px)
// Divider at y=40
// Middle band (42-77): Distance + Label (36px)
// Divider at y=78
// Bottom band (80-127): 2-line instructions (48px)

#define PORTRAIT_ARROW_HEIGHT    40
#define PORTRAIT_DIVIDER1_Y      40    // First horizontal divider

#define PORTRAIT_ARROW_ROW1_Y    4     // Arrow row 1 (Font16)
#define PORTRAIT_ARROW_ROW2_Y    22    // Arrow row 2 (Font16)

#define PORTRAIT_DISTANCE_Y      46    // Distance text (Font16)
#define PORTRAIT_LABEL_Y         64    // Maneuver label (Font8 - reduced to fit)
#define PORTRAIT_DIVIDER2_Y      78    // Second horizontal divider

#define PORTRAIT_INSTR_LINE1_Y   86    // Instruction line 1 (Font8)
#define PORTRAIT_INSTR_LINE2_Y   102   // Instruction line 2 (Font8)
#define PORTRAIT_INSTR_MAX_CHARS 10    // Max chars with Font8 (5px wide) in 64px - aggressive wrap

// =============================================================================
// EXTERN DECLARATIONS (from 05_nav_data.ino)
// =============================================================================

extern const char* NAV_ROW1[];
extern const char* NAV_ROW2[];
extern const char* NAV_ROW1_COMPACT[];
extern const char* NAV_ROW2_COMPACT[];
extern const char* NAV_LABELS[];
extern const char* NAV_NAMES[];

// Status screen ASCII art (compact versions for portrait)
extern const char* HEADSENSE_ROW1_COMPACT;
extern const char* HEADSENSE_ROW2_COMPACT;
extern const char* HEADSENSE_ROW3_COMPACT;
extern const char* HEADSENSE_ROW4_COMPACT;
extern const char* COMPLETE_ROW1_COMPACT;
extern const char* COMPLETE_ROW2_COMPACT;
extern const char* COMPLETE_ROW3_COMPACT;

// =============================================================================
// PORTRAIT MODE HELPER FUNCTIONS
// =============================================================================

// Center text horizontally within portrait display width (64px)
UWORD portraitCenterX(const char* text, UWORD fontWidth) {
  UWORD textWidth = fontWidth * strlen(text);
  if (textWidth >= PORTRAIT_WIDTH) return 0;
  return (PORTRAIT_WIDTH - textWidth) / 2;
}

// =============================================================================
// PORTRAIT MODE DRAWING FUNCTIONS
// =============================================================================

// Simple centered layout: Arrow + Label (basic implementation)
void portraitDrawNavInstruction(NavInstruction nav, UBYTE* image) {
  if (nav >= NAV_COUNT) nav = NAV_NONE;
  
  Serial.print("[NAV] portraitDrawNavInstruction nav=");
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
  
  // Use compact arrows (5 chars) - fits better in 64px width
  UWORD arrowX = portraitCenterX(NAV_ROW1_COMPACT[nav], FONT16_WIDTH);
  UWORD labelX = portraitCenterX(NAV_LABELS[nav], FONT8_WIDTH);
  
  // Arrow in top section
  Paint_DrawString_EN(arrowX, 20, NAV_ROW1_COMPACT[nav], &Font16, WHITE, WHITE);
  Paint_DrawString_EN(arrowX, 38, NAV_ROW2_COMPACT[nav], &Font16, WHITE, WHITE);
  
  // Label in middle
  Paint_DrawString_EN(labelX, 70, NAV_LABELS[nav], &Font8, WHITE, WHITE);
}

// Compact arrow only (no label)
void portraitDrawNavArrowOnly(NavInstruction nav, UBYTE* image) {
  if (nav >= NAV_COUNT) nav = NAV_NONE;
  
  Paint_SelectImage(image);
  Paint_Clear(BLACK);
  
  UWORD arrowX = portraitCenterX(NAV_ROW1_COMPACT[nav], FONT16_WIDTH);
  
  // Center arrow vertically
  Paint_DrawString_EN(arrowX, 48, NAV_ROW1_COMPACT[nav], &Font16, WHITE, WHITE);
  Paint_DrawString_EN(arrowX, 66, NAV_ROW2_COMPACT[nav], &Font16, WHITE, WHITE);
}

// Arrow + distance below
void portraitDrawNavWithDistance(NavInstruction nav, const char* distance, UBYTE* image) {
  if (nav >= NAV_COUNT) nav = NAV_NONE;
  
  Paint_SelectImage(image);
  Paint_Clear(BLACK);
  
  UWORD arrowX = portraitCenterX(NAV_ROW1_COMPACT[nav], FONT16_WIDTH);
  UWORD distX = portraitCenterX(distance, FONT16_WIDTH);
  
  // Arrow in top third
  Paint_DrawString_EN(arrowX, 20, NAV_ROW1_COMPACT[nav], &Font16, WHITE, WHITE);
  Paint_DrawString_EN(arrowX, 38, NAV_ROW2_COMPACT[nav], &Font16, WHITE, WHITE);
  
  // Distance in middle
  Paint_DrawString_EN(distX, 80, distance, &Font16, WHITE, WHITE);
}

// -----------------------------------------------------------------------------
// FULL VERTICAL STACKING LAYOUT
// -----------------------------------------------------------------------------
// Top: Arrow icon (Font16, compact 5-char)
// Middle: Distance (Font16) + Label (Font8)
// Dividers: Solid horizontal lines
// Bottom: 2-line instruction (Font8)

void portraitDrawNavQuadrant(
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
  
  // --- Top section: Arrow (Font16, compact) ---
  UWORD arrowX = portraitCenterX(NAV_ROW1_COMPACT[nav], FONT16_WIDTH);
  Paint_DrawString_EN(arrowX, PORTRAIT_ARROW_ROW1_Y, NAV_ROW1_COMPACT[nav], &Font16, WHITE, WHITE);
  Paint_DrawString_EN(arrowX, PORTRAIT_ARROW_ROW2_Y, NAV_ROW2_COMPACT[nav], &Font16, WHITE, WHITE);
  
  // --- First divider line ---
  Paint_DrawLine(0, PORTRAIT_DIVIDER1_Y, PORTRAIT_WIDTH - 1, PORTRAIT_DIVIDER1_Y,
                 WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
  
  // --- Middle section: Distance (Font16) + Label (Font8) ---
  UWORD distX = portraitCenterX(distance, FONT16_WIDTH);
  UWORD labelX = portraitCenterX(NAV_LABELS[nav], FONT8_WIDTH);
  
  Paint_DrawString_EN(distX, PORTRAIT_DISTANCE_Y, distance, &Font16, WHITE, WHITE);
  Paint_DrawString_EN(labelX, PORTRAIT_LABEL_Y, NAV_LABELS[nav], &Font8, WHITE, WHITE);
  
  // --- Second divider line ---
  Paint_DrawLine(0, PORTRAIT_DIVIDER2_Y, PORTRAIT_WIDTH - 1, PORTRAIT_DIVIDER2_Y,
                 WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
  
  // --- Bottom section: Instructions (Font8, centered) ---
  if (instrLine1 && strlen(instrLine1) > 0) {
    UWORD instr1X = portraitCenterX(instrLine1, FONT8_WIDTH);
    Paint_DrawString_EN(instr1X, PORTRAIT_INSTR_LINE1_Y, instrLine1, &Font8, WHITE, WHITE);
  }
  if (instrLine2 && strlen(instrLine2) > 0) {
    UWORD instr2X = portraitCenterX(instrLine2, FONT8_WIDTH);
    Paint_DrawString_EN(instr2X, PORTRAIT_INSTR_LINE2_Y, instrLine2, &Font8, WHITE, WHITE);
  }
  
  Serial.print("[NAV] portraitDrawNavQuadrant: ");
  Serial.print(NAV_LABELS[nav]);
  Serial.print(" dist=");
  Serial.println(distance);
}

// -----------------------------------------------------------------------------
// HIGH-LEVEL CONVENIENCE FUNCTION
// -----------------------------------------------------------------------------
// Processes raw Google Directions API data and renders full portrait layout

void portraitDrawNavFromApi(
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
  
  // Word-wrap into two lines (narrower width = fewer chars)
  char line1[PORTRAIT_INSTR_MAX_CHARS + 1];
  char line2[PORTRAIT_INSTR_MAX_CHARS + 1];
  wrapText(cleanInstr.c_str(), line1, line2, PORTRAIT_INSTR_MAX_CHARS);
  
  // Draw the portrait layout
  portraitDrawNavQuadrant(nav, dist.c_str(), line1, line2, image);
}

// =============================================================================
// PORTRAIT STATUS SCREENS
// =============================================================================

// Welcome screen: HeadSense ASCII art box + "Waiting" message
void portraitDrawWelcomeScreen(UBYTE* image) {
  if (image == NULL) {
    Serial.println("[NAV] ERROR: image buffer is NULL!");
    return;
  }
  
  Paint_SelectImage(image);
  Paint_Clear(BLACK);
  
  // HeadSense ASCII art box - centered (Font8 for 8 chars)
  UWORD artX = portraitCenterX(HEADSENSE_ROW1_COMPACT, FONT8_WIDTH);
  Paint_DrawString_EN(artX, 30, HEADSENSE_ROW1_COMPACT, &Font8, WHITE, WHITE);
  Paint_DrawString_EN(artX, 42, HEADSENSE_ROW2_COMPACT, &Font8, WHITE, WHITE);
  Paint_DrawString_EN(artX, 54, HEADSENSE_ROW3_COMPACT, &Font8, WHITE, WHITE);
  Paint_DrawString_EN(artX, 66, HEADSENSE_ROW4_COMPACT, &Font8, WHITE, WHITE);
  
  // "Waiting.." text below
  const char* msg = "Waiting..";
  UWORD msgX = portraitCenterX(msg, FONT8_WIDTH);
  Paint_DrawString_EN(msgX, 90, msg, &Font8, WHITE, WHITE);
  
  Serial.println("[NAV] portraitDrawWelcomeScreen");
}

// Destination prompt screen: "What's your Destination?" after BLE pairing
void portraitDrawDestinationPrompt(UBYTE* image) {
  if (image == NULL) {
    Serial.println("[NAV] ERROR: image buffer is NULL!");
    return;
  }
  
  Paint_SelectImage(image);
  Paint_Clear(BLACK);
  
  // Three lines centered vertically
  const char* line1 = "What's";
  const char* line2 = "your";
  const char* line3 = "Dest?";
  
  UWORD x1 = portraitCenterX(line1, FONT12_WIDTH);
  UWORD x2 = portraitCenterX(line2, FONT12_WIDTH);
  UWORD x3 = portraitCenterX(line3, FONT12_WIDTH);
  
  Paint_DrawString_EN(x1, 40, line1, &Font12, WHITE, WHITE);
  Paint_DrawString_EN(x2, 58, line2, &Font12, WHITE, WHITE);
  Paint_DrawString_EN(x3, 76, line3, &Font12, WHITE, WHITE);
  
  Serial.println("[NAV] portraitDrawDestinationPrompt");
}

// Navigation complete screen: HeadSense branding + "Arrived!" message
void portraitDrawCompleteScreen(UBYTE* image) {
  if (image == NULL) {
    Serial.println("[NAV] ERROR: image buffer is NULL!");
    return;
  }
  
  Paint_SelectImage(image);
  Paint_Clear(BLACK);
  
  // HeadSense ASCII art box - centered (Font8 for 8 chars)
  UWORD artX = portraitCenterX(HEADSENSE_ROW1_COMPACT, FONT8_WIDTH);
  Paint_DrawString_EN(artX, 30, HEADSENSE_ROW1_COMPACT, &Font8, WHITE, WHITE);
  Paint_DrawString_EN(artX, 42, HEADSENSE_ROW2_COMPACT, &Font8, WHITE, WHITE);
  Paint_DrawString_EN(artX, 54, HEADSENSE_ROW3_COMPACT, &Font8, WHITE, WHITE);
  Paint_DrawString_EN(artX, 66, HEADSENSE_ROW4_COMPACT, &Font8, WHITE, WHITE);
  
  // "Arrived!" text below
  const char* msg = "Arrived!";
  UWORD msgX = portraitCenterX(msg, FONT8_WIDTH);
  Paint_DrawString_EN(msgX, 90, msg, &Font8, WHITE, WHITE);
  
  Serial.println("[NAV] portraitDrawCompleteScreen");
}
