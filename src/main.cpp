// PLUS2 stub main.cpp — minimum needed to prove the screen + one ASCII buddy
// works on M5StickC PLUS2 + M5Unified. The upstream main.cpp has been parked;
// reintroduce its features (BLE, clock, menu, prompts, stats) one at a time
// after this boots cleanly.
#include <M5Unified.h>
#include <string.h>
#include "buddy.h"

const int W = 135, H = 240;
M5Canvas spr(&M5.Display);

// Region below the buddy where we draw the species name + sound. Buddy only
// clears y=0..82 at 1× scale, so we own everything from y=110 down. Wipe
// this band whenever the species changes so old text doesn't ghost.
const int TEXT_TOP = 110;

static const char* speciesSound(const char* name) {
  if (!strcmp(name, "capybara")) return "squee!";
  if (!strcmp(name, "duck"))     return "quack!";
  if (!strcmp(name, "goose"))    return "HONK!";
  if (!strcmp(name, "blob"))     return "blorp...";
  if (!strcmp(name, "cat"))      return "meow!";
  if (!strcmp(name, "dragon"))   return "ROAR!";
  if (!strcmp(name, "octopus"))  return "blub blub";
  if (!strcmp(name, "owl"))      return "hoot hoot";
  if (!strcmp(name, "penguin"))  return "noot noot!";
  if (!strcmp(name, "turtle"))   return "...slurp";
  if (!strcmp(name, "snail"))    return "...";
  if (!strcmp(name, "ghost"))    return "BOO!";
  if (!strcmp(name, "axolotl"))  return "glub glub";
  if (!strcmp(name, "cactus"))   return "ouch!";
  if (!strcmp(name, "robot"))    return "BEEP BOOP";
  if (!strcmp(name, "rabbit"))   return "boing!";
  if (!strcmp(name, "mushroom")) return "spore!";
  if (!strcmp(name, "chonk"))    return "om nom";
  return "...";
}

static void drawSpeciesText() {
  spr.fillRect(0, TEXT_TOP, W, H - TEXT_TOP, 0x0000);
  spr.setTextDatum(MC_DATUM);
  spr.setTextSize(2);
  spr.setTextColor(0x07FF, 0x0000);              // cyan
  spr.drawString(speciesSound(buddySpeciesName()), W / 2, 160);
  spr.setTextDatum(TL_DATUM);
}

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
  drawSpeciesText();
}

void loop() {
  M5.update();

  if (M5.BtnA.wasPressed()) {
    buddyNextSpecies();     // wraps around at the end of the species table
    drawSpeciesText();
  }

  // buddyTick is internally throttled to 5fps and clears its own region; do
  // NOT fillSprite() here or the screen flickers black between buddy frames.
  buddyTick(1);  // 1 = P_IDLE in upstream PersonaState ordering
  spr.pushSprite(0, 0);
  delay(33);
}
