# HeadSense OLED Display Layout Design

Navigation HUD layout for the Waveshare 1.51" Transparent OLED (128×64 pixels).

## Display Specifications

| Property | Value |
|----------|-------|
| Resolution | 128×64 pixels (Rotate=270) |
| X range | 0–127 (left to right) |
| Y range | 0–63 (top to bottom) |
| Color | Monochrome cyan on transparent |

## Layout: 50/50 Vertical Split with Asymmetric Top

```
+------------ 128px total width ------------+
|                                           |
|  +-- 43px --+-------- 85px ----------+    |
|  |          |                        |    |
|  |  ASCII   |   DISTANCE · LABEL     |    | 32px
|  |  ARROW   |   (0.6km · TURN LEFT)  |    | (top half)
|  |  (icon)  |                        |    |
|  +----------+------------------------+    |
|                                           |
|  +-------- Full 128px width ---------+    |
|  |                                   |    |
|  |  INSTRUCTION LINE 1               |    | 32px
|  |  INSTRUCTION LINE 2               |    | (bottom half)
|  |                                   |    |
|  +-----------------------------------+    |
|                                           |
+-------------------------------------------+
```

## Quadrant Breakdown

### Top-Left: Navigation Arrow (43×32 px)

| Property | Value |
|----------|-------|
| X range | 0–42 |
| Y range | 0–31 |
| Content | ASCII arrow icon (2 rows) |
| Font | Font16 (11×16 px) |
| Row 1 Y | 0 |
| Row 2 Y | 16 |

**Arrow Graphics:** Uses `NAV_ROW1[]` and `NAV_ROW2[]` from `05_nav_graphics.ino`

### Top-Right: Distance + Label (85×32 px)

| Property | Value |
|----------|-------|
| X range | 43–127 |
| Y range | 0–31 |
| Content | Distance text + maneuver label |

**Sub-layout:**

| Element | X | Y | Font | Example |
|---------|---|---|------|---------|
| Distance | 45 | 4 | Font16 | "0.6km" |
| Label | 45 | 20 | Font12 | "TURN LEFT" |

Alternative single-line layout:

| Element | X | Y | Font | Example |
|---------|---|---|------|---------|
| Combined | 45 | 8 | Font16 | "0.6km" |
| Label | 45 | 24 | Font12 | "TURN LEFT" |

### Bottom: Full-Width Instructions (128×32 px)

| Property | Value |
|----------|-------|
| X range | 0–127 |
| Y range | 32–63 |
| Content | Cleaned HTML instructions (2 lines) |
| Font | Font12 (7×12 px) |
| Max chars/line | ~18 characters |
| Line 1 Y | 34 |
| Line 2 Y | 48 |

## Example Renders

### Step 1: turn-right, "Turn right", 0.6km

```
+----------+------------------------+
|   +-->   |   0.6km               |
|     |    |   TURN RIGHT          |
+--------------------------------------+
| Turn right                          |
|                                     |
+--------------------------------------+
```

### Step 2: turn-left, "Turn left toward 2nd Main Rd", 76m

```
+----------+------------------------+
| <--+     |   76m                  |
|   |      |   TURN LEFT            |
+--------------------------------------+
| Turn left toward                    |
| 2nd Main Rd                         |
+--------------------------------------+
```

### Step 10: keep-right, "Keep right to continue toward 100 Feet Ring Rd", 38m

```
+----------+------------------------+
|     \^   |   38m                  |
|    /|    |   KEEP RIGHT           |
+--------------------------------------+
| Keep right to continue              |
| toward 100 Feet Ring Rd             |
+--------------------------------------+
```

### Step 11: merge, "Merge onto 100 Feet Ring Rd/Outer Ring Rd", 0.3km

```
+----------+------------------------+
|   |\     |   0.3km                |
|    |     |   MERGE                |
+--------------------------------------+
| Merge onto 100 Feet                 |
| Ring Rd                             |
+--------------------------------------+
```

### Arrival: "Destination on the right"

```
+----------+------------------------+
|  [X]     |   --                   |
|  DONE    |   ARRIVED              |
+--------------------------------------+
| Destination will be                 |
| on the right                        |
+--------------------------------------+
```

## Data Mapping from Google Directions API

Source: `routes[0].legs[0].steps[i]`

| Display Element | JSON Path | Processing |
|-----------------|-----------|------------|
| ASCII Arrow | `maneuver` | `parseNavInstruction()` → NAV_ROW1/NAV_ROW2 |
| Distance | `distance.text` | Strip space: "0.6 km" → "0.6km" |
| Label | `maneuver` | `NAV_LABELS[nav]` ("TURN LEFT", "KEEP RIGHT", etc.) |
| Instruction L1 | `html_instructions` | `stripHtmlTags()` → word-wrap line 1 |
| Instruction L2 | `html_instructions` | Word-wrap overflow to line 2 |

### HTML Tag Examples to Strip

| Raw | Cleaned |
|-----|---------|
| `Turn \u003cb\u003eright\u003c/b\u003e` | `Turn right` |
| `Keep \u003cb\u003eleft\u003c/b\u003e to stay on...` | `Keep left to stay on...` |
| `Head \u003cb\u003esouthwest\u003c/b\u003e\u003cdiv...\u003eRestricted...\u003c/div\u003e` | `Head southwest` |

## Pixel Coordinates Reference

```cpp
// Layout constants
#define DISPLAY_WIDTH      128
#define DISPLAY_HEIGHT     64

// Top-left quadrant (arrow)
#define ARROW_X            0
#define ARROW_WIDTH        43
#define ARROW_ROW1_Y       0
#define ARROW_ROW2_Y       16

// Top-right quadrant (distance + label)
#define INFO_X             45
#define DISTANCE_Y         4
#define LABEL_Y            20

// Bottom section (instructions)
#define INSTR_X            2
#define INSTR_LINE1_Y      34
#define INSTR_LINE2_Y      48
#define INSTR_MAX_CHARS    18
```

## Font Reference

| Font | Width | Height | Use Case |
|------|-------|--------|----------|
| Font8 | 5px | 8px | Secondary info, compact text |
| Font12 | 7px | 12px | Instructions, labels |
| Font16 | 11px | 16px | Distance, arrows |
| Font24 | 17px | 24px | Large emphasis (limited use) |

## Implementation Functions

### Required Functions

```cpp
// Draw the full navigation layout
void drawNavLayout(
    NavInstruction nav,      // Arrow type
    const char* distance,    // "0.6km"
    const char* line1,       // Instruction line 1
    const char* line2,       // Instruction line 2 (can be empty)
    UBYTE* image
);

// Strip HTML tags from instructions
String stripHtmlTags(const char* html);

// Word-wrap text to fit display width
void wrapText(
    const char* text,        // Input text
    char* line1,             // Output line 1
    char* line2,             // Output line 2
    int maxChars             // Max chars per line (~18 for Font12)
);

// Center text in left 43px region
UWORD centerLeftX(const char* text, UWORD fontWidth);

// Format distance (remove space)
String formatDistance(const char* distText);  // "0.6 km" → "0.6km"
```

## Design Rationale

1. **Arrow prominence:** Left column dedicated to navigation icon for quick glance recognition while riding.

2. **Distance visibility:** Large Font16 for distance ensures readability at a glance through helmet visor.

3. **Full-width instructions:** Bottom half uses entire width for detailed turn instructions, accommodating longer road names.

4. **Asymmetric split (43/85):** Matches the visual balance shown in Waveshare's demo layouts; icon needs less width than text info.

5. **Two-line word wrap:** Handles longer instructions like "Keep right to continue toward Outer Ring Rd" without truncation.

## Future Enhancements

- [ ] Add ETA display (optional 5th element)
- [ ] Speed/distance remaining to destination
- [ ] Bitmap icons instead of ASCII for cleaner arrows
- [ ] Dynamic font sizing based on content length
- [ ] Night mode (inverted colors)
