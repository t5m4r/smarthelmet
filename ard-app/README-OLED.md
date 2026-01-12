# OLED Display Programming Guide

Reference for the Waveshare 1.51inch Transparent OLED connected to Arduino UNO R4 WiFi.

**Product:** [Waveshare 1.51inch Transparent OLED](https://www.waveshare.com/wiki/1.51inch_Transparent_OLED)

## Display Dimensions

| Property | Value | Notes |
|----------|-------|-------|
| Raw dimensions | 64×128 | Controller's default orientation |
| Logical canvas | **128×64** | With `Rotate=270` (horizontal layout) |
| X range | 0–127 | Left to right |
| Y range | 0–63 | Top to bottom |

## Available Fonts

| Font | Width (px) | Height (px) | Max chars/line | Max lines |
|------|------------|-------------|----------------|-----------|
| Font8 | 5 | 8 | ~25 | 8 |
| Font12 | 7 | 12 | ~18 | 5 |
| Font16 | 11 | 16 | ~11 | 4 |
| Font20 | 14 | 20 | ~9 | 3 |
| Font24 | 17 | 24 | ~7 | 2 |

## Buffer Workflow

```cpp
// 1. Select the image buffer
Paint_SelectImage(BlackImage);

// 2. Clear to black (all pixels off)
Paint_Clear(BLACK);

// 3. Draw shapes and text
Paint_DrawString_EN(x, y, "text", &Font16, WHITE, WHITE);

// 4. Push buffer to display
OLED_1IN51_Display(BlackImage);
```

## Drawing Text

```cpp
void Paint_DrawString_EN(
    UWORD Xstart,           // X position (0-127)
    UWORD Ystart,           // Y position (0-63)
    const char *pString,    // Null-terminated string
    sFONT* Font,            // &Font8, &Font12, &Font16, &Font20, &Font24
    UWORD Color_Foreground, // WHITE for visible pixels
    UWORD Color_Background  // WHITE for transparent background
);
```

**Example:**
```cpp
Paint_DrawString_EN(10, 0, "Hello", &Font16, WHITE, WHITE);
```

**Text wrapping:** If text exceeds `Paint.Width`, it auto-wraps to next line.

## Drawing Shapes

### Points
```cpp
Paint_DrawPoint(UWORD x, UWORD y, UWORD Color, DOT_PIXEL size, DOT_STYLE style);

// Example: 3x3 pixel dot at (40, 10)
Paint_DrawPoint(40, 10, WHITE, DOT_PIXEL_3X3, DOT_STYLE_DFT);
```

### Lines
```cpp
Paint_DrawLine(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,
               UWORD Color, DOT_PIXEL width, LINE_STYLE style);

// Example: Solid line from (10,10) to (50,10)
Paint_DrawLine(10, 10, 50, 10, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

// Example: Dotted line
Paint_DrawLine(10, 20, 50, 20, WHITE, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
```

### Rectangles
```cpp
Paint_DrawRectangle(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,
                    UWORD Color, DOT_PIXEL width, DRAW_FILL fill);

// Example: Empty rectangle
Paint_DrawRectangle(10, 10, 50, 30, WHITE, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

// Example: Filled rectangle
Paint_DrawRectangle(60, 10, 100, 30, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
```

### Circles
```cpp
Paint_DrawCircle(UWORD X_Center, UWORD Y_Center, UWORD Radius,
                 UWORD Color, DOT_PIXEL width, DRAW_FILL fill);

// Example: Empty circle at (64, 32) with radius 15
Paint_DrawCircle(64, 32, 15, WHITE, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

// Example: Filled circle
Paint_DrawCircle(100, 32, 10, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
```

## Enums Reference

```cpp
// Pixel sizes for points and line widths
typedef enum {
    DOT_PIXEL_1X1 = 1,
    DOT_PIXEL_2X2,
    DOT_PIXEL_3X3,
    DOT_PIXEL_4X4,
    DOT_PIXEL_5X5,
    DOT_PIXEL_6X6,
    DOT_PIXEL_7X7,
    DOT_PIXEL_8X8,
} DOT_PIXEL;

// Line styles
typedef enum {
    LINE_STYLE_SOLID = 0,
    LINE_STYLE_DOTTED,
} LINE_STYLE;

// Fill modes for rectangles and circles
typedef enum {
    DRAW_FILL_EMPTY = 0,
    DRAW_FILL_FULL,
} DRAW_FILL;
```

## Initialization (One-time Setup)

```cpp
UBYTE *BlackImage;

void oled_setup() {
    System_Init();
    OLED_1IN51_Init();
    Driver_Delay_ms(500);
    OLED_1IN51_Clear();

    // Allocate image buffer
    UWORD Imagesize = ((OLED_1IN51_WIDTH % 8 == 0) ? 
                       (OLED_1IN51_WIDTH / 8) : 
                       (OLED_1IN51_WIDTH / 8 + 1)) * OLED_1IN51_HEIGHT;
    
    BlackImage = (UBYTE *)malloc(Imagesize);
    if (BlackImage == NULL) {
        Serial.println("Failed to allocate image buffer");
        return;
    }

    // Initialize Paint library with rotation
    Paint_NewImage(BlackImage, OLED_1IN51_WIDTH, OLED_1IN51_HEIGHT, 270, BLACK);
    Paint_SelectImage(BlackImage);
    Paint_Clear(BLACK);
}
```

## Centering Text

```cpp
// Center a string horizontally
void drawCenteredText(const char* text, UWORD y, sFONT* font) {
    UWORD text_width = font->Width * strlen(text);
    UWORD x = (128 - text_width) / 2;  // 128 = Paint.Width
    Paint_DrawString_EN(x, y, text, font, WHITE, WHITE);
}
```

## Navigation UI Layout Example

For a 128×64 display showing navigation arrows:

```
┌────────────────────────────────────┐  Y=0
│         (margin 4px)               │
│      ┌──────────────────┐          │  Y=4
│      │   ASCII Arrow    │ Font16   │  
│      │   (Row 1)        │ 16px     │  Y=20
│      │   ASCII Arrow    │ Font16   │
│      │   (Row 2)        │ 16px     │  Y=36
│      └──────────────────┘          │
│         (gap 4px)                  │  Y=40
│      ┌──────────────────┐          │  Y=44
│      │  Label (Font12)  │ 12px     │
│      └──────────────────┘          │  Y=56
│         (margin 8px)               │
└────────────────────────────────────┘  Y=64
         X=0                    X=128
```

## Tips

1. **Always clear before drawing:** `Paint_Clear(BLACK)` ensures no artifacts from previous frame.

2. **Colors:** On this monochrome OLED, `WHITE` = pixel on, `BLACK` = pixel off.

3. **Performance:** Minimize `OLED_1IN51_Display()` calls; batch all drawing first, then display once.

4. **Memory:** The image buffer uses ~1KB RAM (`64/8 * 128 = 1024 bytes`).

5. **Rotation:** With `Rotate=270`, the display is horizontal (128 wide × 64 tall).
