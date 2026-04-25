// PLUS2 stub main.cpp — minimum needed to prove the screen + one ASCII buddy
// works on M5StickC PLUS2 + M5Unified. The upstream main.cpp has been parked;
// reintroduce its features (BLE, clock, menu, prompts, stats) one at a time
// after this boots cleanly.
#include <M5Unified.h>
#include "buddy.h"

const int W = 135, H = 240;
M5Canvas spr(&M5.Display);

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(0);
  M5.Display.setBrightness(180);

  spr.createSprite(W, H);
  buddyInit();
  buddySetSpeciesIdx(0);

  spr.fillSprite(0x0000);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(0xFFFF, 0x0000);
  spr.setTextSize(2);
  spr.drawString("Hello!", W / 2, H / 2 - 12);
  spr.setTextSize(1);
  spr.setTextColor(0x8410, 0x0000);
  spr.drawString("a buddy appears", W / 2, H / 2 + 12);
  spr.setTextDatum(TL_DATUM);
  spr.pushSprite(0, 0);
  delay(1500);
  spr.fillSprite(0x0000);   // splash → blank, then let buddyTick paint in
}

void loop() {
  M5.update();
  // buddyTick is internally throttled to 5fps and clears its own region; do
  // NOT fillSprite() here or the screen flickers black between buddy frames.
  buddyTick(1);  // 1 = P_IDLE in upstream PersonaState ordering
  spr.pushSprite(0, 0);
  delay(33);
}
