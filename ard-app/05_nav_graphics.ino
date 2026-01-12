// 05_nav_graphics.ino - ASCII navigation icons for OLED display
// Integrates with Waveshare OLED_1in51 library (128x64 logical with Rotate=270)

#include "OLED_1in51/GUI_paint.h"
#include "navigation.h"

// Display dimensions (logical, after Rotate=270)
#define DISPLAY_WIDTH  128
#define DISPLAY_HEIGHT 64

// Font dimensions for centering calculations
#define FONT16_WIDTH  11
#define FONT16_HEIGHT 16
#define FONT12_WIDTH  7
#define FONT12_HEIGHT 12

// Layout constants
#define ARROW_ROW1_Y  4
#define ARROW_ROW2_Y  (ARROW_ROW1_Y + FONT16_HEIGHT + 2)  // 22
#define LABEL_Y       (ARROW_ROW2_Y + FONT16_HEIGHT + 6)  // 44

// ASCII graphics - Row 1 (direction arrow) - 10 chars each
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

// Calculate X position to center text horizontally
UWORD centerTextX(const char* text, UWORD fontWidth) {
  UWORD textWidth = fontWidth * strlen(text);
  if (textWidth >= DISPLAY_WIDTH) return 0;
  return (DISPLAY_WIDTH - textWidth) / 2;
}

// Display navigation instruction on Waveshare OLED (centered)
// Call OLED_1IN51_Display(BlackImage) after to refresh screen
void drawNavInstruction(NavInstruction nav, UBYTE* image) {
  if (nav >= NAV_COUNT) nav = NAV_NONE;
  
  Paint_SelectImage(image);
  Paint_Clear(BLACK);
  
  // Calculate centered X positions
  UWORD arrowX = centerTextX(NAV_ROW1[nav], FONT16_WIDTH);
  UWORD labelX = centerTextX(NAV_LABELS[nav], FONT12_WIDTH);
  
  // Row 1: ASCII arrow graphic (centered)
  Paint_DrawString_EN(arrowX, ARROW_ROW1_Y, NAV_ROW1[nav], &Font16, WHITE, WHITE);
  
  // Row 2: ASCII road/path graphic (centered)
  Paint_DrawString_EN(arrowX, ARROW_ROW2_Y, NAV_ROW2[nav], &Font16, WHITE, WHITE);
  
  // Row 3: Human-readable label (centered, smaller font)
  Paint_DrawString_EN(labelX, LABEL_Y, NAV_LABELS[nav], &Font12, WHITE, WHITE);
}

// Compact version: just the arrow, no label (larger, centered)
void drawNavArrowOnly(NavInstruction nav, UBYTE* image) {
  if (nav >= NAV_COUNT) nav = NAV_NONE;
  
  Paint_SelectImage(image);
  Paint_Clear(BLACK);
  
  // Center vertically: (64 - 2*16 - 4) / 2 = 14
  UWORD arrowX = centerTextX(NAV_ROW1[nav], FONT16_WIDTH);
  UWORD row1Y = 14;
  UWORD row2Y = row1Y + FONT16_HEIGHT + 4;  // 34
  
  Paint_DrawString_EN(arrowX, row1Y, NAV_ROW1[nav], &Font16, WHITE, WHITE);
  Paint_DrawString_EN(arrowX, row2Y, NAV_ROW2[nav], &Font16, WHITE, WHITE);
}

// Draw navigation with distance info (e.g., "500m")
void drawNavWithDistance(NavInstruction nav, const char* distance, UBYTE* image) {
  if (nav >= NAV_COUNT) nav = NAV_NONE;
  
  Paint_SelectImage(image);
  Paint_Clear(BLACK);
  
  // Arrow takes top portion
  UWORD arrowX = centerTextX(NAV_ROW1[nav], FONT16_WIDTH);
  Paint_DrawString_EN(arrowX, 2, NAV_ROW1[nav], &Font16, WHITE, WHITE);
  Paint_DrawString_EN(arrowX, 20, NAV_ROW2[nav], &Font16, WHITE, WHITE);
  
  // Distance at bottom (large, centered)
  UWORD distX = centerTextX(distance, FONT16_WIDTH);
  Paint_DrawString_EN(distX, 44, distance, &Font16, WHITE, WHITE);
}
