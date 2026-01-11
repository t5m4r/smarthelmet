// 05_nav_graphics.ino - ASCII navigation icons for OLED display
// Integrates with Waveshare OLED_1in51 library

#include "OLED_1in51/GUI_paint.h"

// ASCII graphics - Row 1 (direction arrow)
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

// ASCII graphics - Row 2 (road/path)
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

// String names for parsing BLE commands
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

// Parse string command to NavInstruction enum
NavInstruction parseNavInstruction(const char* cmd) {
  for (int i = 0; i < NAV_COUNT; i++) {
    if (strcmp(cmd, NAV_NAMES[i]) == 0) {
      return (NavInstruction)i;
    }
  }
  return NAV_NONE;
}

// Display navigation instruction on Waveshare OLED
// Uses Paint_DrawString_EN from GUI_paint.h
// Call OLED_1IN51_Display(BlackImage) after to refresh screen
void drawNavInstruction(NavInstruction nav, UBYTE* image) {
  if (nav >= NAV_COUNT) nav = NAV_NONE;
  
  Paint_SelectImage(image);
  Paint_Clear(BLACK);
  
  // Row 1: ASCII arrow graphic (large font, top)
  Paint_DrawString_EN(0, 0, NAV_ROW1[nav], &Font16, WHITE, WHITE);
  
  // Row 2: ASCII road/path graphic
  Paint_DrawString_EN(0, 20, NAV_ROW2[nav], &Font16, WHITE, WHITE);
  
  // Row 3: Human-readable label (smaller font, bottom)
  Paint_DrawString_EN(0, 44, NAV_LABELS[nav], &Font12, WHITE, WHITE);
}

// Compact version: just the arrow, no label
void drawNavArrowOnly(NavInstruction nav, UBYTE* image) {
  if (nav >= NAV_COUNT) nav = NAV_NONE;
  
  Paint_SelectImage(image);
  Paint_Clear(BLACK);
  
  // Large ASCII graphics centered vertically
  Paint_DrawString_EN(0, 8, NAV_ROW1[nav], &Font16, WHITE, WHITE);
  Paint_DrawString_EN(0, 28, NAV_ROW2[nav], &Font16, WHITE, WHITE);
}
