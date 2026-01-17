// navigation.h - Navigation types and enums

#ifndef NAVIGATION_H
#define NAVIGATION_H

// Navigation instruction enum
enum NavInstruction {
  NAV_NONE = 0,
  NAV_CONTINUE,
  NAV_TURN_LEFT,
  NAV_TURN_RIGHT,
  NAV_SLIGHT_LEFT,
  NAV_SLIGHT_RIGHT,
  NAV_SHARP_LEFT,
  NAV_SHARP_RIGHT,
  NAV_UTURN_LEFT,
  NAV_UTURN_RIGHT,
  NAV_MERGE_LEFT,
  NAV_MERGE_RIGHT,
  NAV_EXIT_LEFT,
  NAV_EXIT_RIGHT,
  NAV_ROUNDABOUT,
  NAV_KEEP_LEFT,
  NAV_KEEP_RIGHT,
  NAV_ARRIVE,
  NAV_COUNT
};

enum HeadSenseState {
  HEADSENSE_UNINITIALIZED,
  HEADSENSE_WELCOME,
  HEADSENSE_TASKED,
  HEADSENSE_TASKED_RECALCULATING,
  HEADSENSE_TASKED_COMPLETE
};

static void prettyPrintHeadSenseState(const HeadSenseState state) {
  if (state == HEADSENSE_UNINITIALIZED) {
    Serial.println("HEADSENSE UNINITIALIZED");
  } else if (state == HEADSENSE_WELCOME) {
    Serial.println("HEADSENSE WELCOME");
  } else if (state == HEADSENSE_TASKED) {
    Serial.println("HEADSENSE TASKED");
  } else if (state == HEADSENSE_TASKED_RECALCULATING) {
    Serial.println("HEADSENSE TASKED RECALCULATING");
  } else if (state == HEADSENSE_TASKED_COMPLETE) {
    Serial.println("HEADSENSE TASKED COMPLETE");
  }
}

#endif  // NAVIGATION_H
