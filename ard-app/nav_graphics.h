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

// =============================================================================
// BACKWARD COMPATIBILITY WRAPPERS
// =============================================================================

void drawNavInstruction(NavInstruction nav, UBYTE* image);
void drawNavArrowOnly(NavInstruction nav, UBYTE* image);
void drawNavWithDistance(NavInstruction nav, const char* distance, UBYTE* image);
void drawNavFromApi(
    const char* maneuver,
    const char* distanceText,
    const char* htmlInstructions,
    UBYTE* image
);

#endif // NAV_GRAPHICS_H
