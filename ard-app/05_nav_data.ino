// 05_nav_data.ino - Shared navigation data arrays
// Used by both landscape and portrait layout implementations
// Must compile BEFORE 06_nav_graphics_*.ino files

#include <string.h>
#include "navigation.h"

// =============================================================================
// ASCII GRAPHICS DATA (shared between landscape/portrait)
// =============================================================================

// ASCII graphics - Row 1 (direction arrow) - 10 chars each, for full-width display
const char* NAV_ROW1[] = {
  "          ",  // NAV_NONE
  "    ^     ",  // NAV_CONTINUE
  "  <--+    ",  // NAV_TURN_LEFT
  "    +-->  ",  // NAV_TURN_RIGHT
  "   /      ",  // NAV_SLIGHT_LEFT
  "      \\   ",  // NAV_SLIGHT_RIGHT
  " <-+      ",  // NAV_SHARP_LEFT
  "      +-> ",  // NAV_SHARP_RIGHT
  "  <--+    ",  // NAV_UTURN_LEFT
  "    +-->  ",  // NAV_UTURN_RIGHT
  "    /|    ",  // NAV_MERGE_LEFT
  "   |\\     ",  // NAV_MERGE_RIGHT
  "  \\--|    ",  // NAV_EXIT_LEFT
  "   |--/   ",  // NAV_EXIT_RIGHT
  "   (O)    ",  // NAV_ROUNDABOUT
  "  ^/      ",  // NAV_KEEP_LEFT
  "      \\^  ",  // NAV_KEEP_RIGHT
  "   [X]    "   // NAV_ARRIVE
};

// ASCII graphics - Row 2 (road/path) - 10 chars each
const char* NAV_ROW2[] = {
  "          ",  // NAV_NONE
  "    |     ",  // NAV_CONTINUE
  "    |     ",  // NAV_TURN_LEFT
  "    |     ",  // NAV_TURN_RIGHT
  "    |     ",  // NAV_SLIGHT_LEFT
  "    |     ",  // NAV_SLIGHT_RIGHT
  "      |   ",  // NAV_SHARP_LEFT
  "  |       ",  // NAV_SHARP_RIGHT
  "  -->+    ",  // NAV_UTURN_LEFT
  "    +<--  ",  // NAV_UTURN_RIGHT
  "    |     ",  // NAV_MERGE_LEFT
  "    |     ",  // NAV_MERGE_RIGHT
  "    |     ",  // NAV_EXIT_LEFT
  "    |     ",  // NAV_EXIT_RIGHT
  "    |     ",  // NAV_ROUNDABOUT
  "   |\\     ",  // NAV_KEEP_LEFT
  "     /|   ",  // NAV_KEEP_RIGHT
  "  DONE    "   // NAV_ARRIVE
};

// Compact 5-char arrows for quadrant layout (fits in narrow columns)
const char* NAV_ROW1_COMPACT[] = {
  "     ",  // NAV_NONE
  "  ^  ",  // NAV_CONTINUE
  " <-+ ",  // NAV_TURN_LEFT
  " +-> ",  // NAV_TURN_RIGHT
  " /   ",  // NAV_SLIGHT_LEFT
  "   \\ ",  // NAV_SLIGHT_RIGHT
  "<-+  ",  // NAV_SHARP_LEFT
  "  +->",  // NAV_SHARP_RIGHT
  " <-+ ",  // NAV_UTURN_LEFT
  " +-> ",  // NAV_UTURN_RIGHT
  "  /| ",  // NAV_MERGE_LEFT
  " |\\  ",  // NAV_MERGE_RIGHT
  " \\-| ",  // NAV_EXIT_LEFT
  " |-/ ",  // NAV_EXIT_RIGHT
  " (O) ",  // NAV_ROUNDABOUT
  " ^/  ",  // NAV_KEEP_LEFT
  "  \\^ ",  // NAV_KEEP_RIGHT
  " [X] "   // NAV_ARRIVE
};

const char* NAV_ROW2_COMPACT[] = {
  "     ",  // NAV_NONE
  "  |  ",  // NAV_CONTINUE
  "  |  ",  // NAV_TURN_LEFT
  "  |  ",  // NAV_TURN_RIGHT
  "  |  ",  // NAV_SLIGHT_LEFT
  "  |  ",  // NAV_SLIGHT_RIGHT
  "   | ",  // NAV_SHARP_LEFT
  " |   ",  // NAV_SHARP_RIGHT
  " ->+ ",  // NAV_UTURN_LEFT
  " +<- ",  // NAV_UTURN_RIGHT
  "  |  ",  // NAV_MERGE_LEFT
  "  |  ",  // NAV_MERGE_RIGHT
  "  |  ",  // NAV_EXIT_LEFT
  "  |  ",  // NAV_EXIT_RIGHT
  "  |  ",  // NAV_ROUNDABOUT
  " |\\  ",  // NAV_KEEP_LEFT
  "  /| ",  // NAV_KEEP_RIGHT
  "DONE "   // NAV_ARRIVE
};

// Human-readable labels for each instruction
const char* NAV_LABELS[] = {
  "",
  "GO STRAIGHT",
  "TURN LEFT",
  "TURN RIGHT",
  "SLIGHT LEFT",
  "SLIGHT RIGHT",
  "SHARP LEFT",
  "SHARP RIGHT",
  "U-TURN LEFT",
  "U-TURN RIGHT",
  "MERGE LEFT",
  "MERGE RIGHT",
  "EXIT LEFT",
  "EXIT RIGHT",
  "ROUNDABOUT",
  "KEEP LEFT",
  "KEEP RIGHT",
  "ARRIVED"
};

// String names for parsing BLE/API commands (must match Google Directions API)
const char* NAV_NAMES[] = {
  "none",
  "continue",
  "turn-left",
  "turn-right",
  "slight-left",
  "slight-right",
  "sharp-left",
  "sharp-right",
  "uturn-left",
  "uturn-right",
  "merge-left",
  "merge-right",
  "exit-left",
  "exit-right",
  "roundabout",
  "keep-left",
  "keep-right",
  "arrive"
};

// =============================================================================
// SHARED PARSING FUNCTION
// =============================================================================

// Parse string command to NavInstruction enum
NavInstruction parseNavInstruction(const char* cmd) {
  if (cmd == NULL) return NAV_NONE;
  
  for (int i = 0; i < NAV_COUNT; i++) {
    if (strcmp(cmd, NAV_NAMES[i]) == 0) {
      Serial.print("[NAV] Parsed '");
      Serial.print(cmd);
      Serial.print("' -> ");
      Serial.println(i);
      return (NavInstruction)i;
    }
  }
  
  // Handle "merge" without direction (default to right for India left-hand driving)
  if (strcmp(cmd, "merge") == 0) {
    Serial.println("[NAV] Parsed 'merge' -> MERGE_RIGHT (default)");
    return NAV_MERGE_RIGHT;
  }
  
  Serial.print("[NAV] Unknown maneuver: '");
  Serial.print(cmd);
  Serial.println("' -> NAV_NONE");
  return NAV_NONE;
}
