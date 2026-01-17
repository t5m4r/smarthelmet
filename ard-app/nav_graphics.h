// nav_graphics.h - Function declarations for navigation graphics
// Include this in files that need to call nav graphics functions

#ifndef NAV_GRAPHICS_H
#define NAV_GRAPHICS_H

#include "navigation.h"
#include "OLED_1in51/GUI_Paint.h"

// =============================================================================
// TEXT PROCESSING UTILITIES (04_smarthelmet_utils.ino)
// =============================================================================

String stripHtmlTags(const char* html);
String formatDistance(const char* distText);
void wrapText(const char* text, char* line1, char* line2, int maxChars);
String abbreviateRoadName(const char* text);

// =============================================================================
// SHARED NAV DATA (05_nav_data.ino)
// =============================================================================

extern const char* NAV_ROW1[];
extern const char* NAV_ROW2[];
extern const char* NAV_ROW1_COMPACT[];
extern const char* NAV_ROW2_COMPACT[];
extern const char* NAV_LABELS[];
extern const char* NAV_NAMES[];

NavInstruction parseNavInstruction(const char* cmd);

// =============================================================================
// LANDSCAPE MODE FUNCTIONS (06_nav_graphics_landscape.ino)
// =============================================================================

// Simple centered layout
void landscapeDrawNavInstruction(NavInstruction nav, UBYTE* image);
void landscapeDrawNavArrowOnly(NavInstruction nav, UBYTE* image);
void landscapeDrawNavWithDistance(NavInstruction nav, const char* distance, UBYTE* image);

// Quadrant layout (50/50 vertical, 1/3-2/3 top)
void landscapeDrawNavQuadrant(
    NavInstruction nav,
    const char* distance,
    const char* instrLine1,
    const char* instrLine2,
    UBYTE* image
);

// High-level: processes raw API data and renders quadrant layout
void landscapeDrawNavFromApi(
    const char* maneuver,
    const char* distanceText,
    const char* htmlInstructions,
    UBYTE* image
);

// Status screens
void landscapeDrawWelcomeScreen(UBYTE* image);
void landscapeDrawDestinationPrompt(UBYTE* image);
void landscapeDrawCompleteScreen(UBYTE* image);

// =============================================================================
// PORTRAIT MODE FUNCTIONS (08_nav_graphics_portrait.ino)
// =============================================================================

// Simple centered layout
void portraitDrawNavInstruction(NavInstruction nav, UBYTE* image);
void portraitDrawNavArrowOnly(NavInstruction nav, UBYTE* image);
void portraitDrawNavWithDistance(NavInstruction nav, const char* distance, UBYTE* image);

// Full vertical stacking layout
void portraitDrawNavQuadrant(
    NavInstruction nav,
    const char* distance,
    const char* instrLine1,
    const char* instrLine2,
    UBYTE* image
);

// Status screens
void portraitDrawWelcomeScreen(UBYTE* image);
void portraitDrawDestinationPrompt(UBYTE* image);
void portraitDrawCompleteScreen(UBYTE* image);

// High-level: processes raw API data and renders portrait layout
void portraitDrawNavFromApi(
    const char* maneuver,
    const char* distanceText,
    const char* htmlInstructions,
    UBYTE* image
);

// =============================================================================
// ORIENTATION-AWARE WRAPPER FUNCTIONS
// =============================================================================
// Pass isLandscape=true for 128x64, false for 64x128

void drawNavInstruction(NavInstruction nav, UBYTE* image, bool isLandscape);
void drawNavArrowOnly(NavInstruction nav, UBYTE* image, bool isLandscape);
void drawNavWithDistance(NavInstruction nav, const char* distance, UBYTE* image, bool isLandscape);
void drawNavFromApi(
    const char* maneuver,
    const char* distanceText,
    const char* htmlInstructions,
    UBYTE* image,
    bool isLandscape
);

// Status screen wrappers
void drawWelcomeScreen(UBYTE* image, bool isLandscape);
void drawDestinationPrompt(UBYTE* image, bool isLandscape);
void drawCompleteScreen(UBYTE* image, bool isLandscape);

#endif // NAV_GRAPHICS_H
