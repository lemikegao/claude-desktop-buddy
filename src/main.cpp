// PLUS2 stub main.cpp — minimum needed to prove the screen + one ASCII buddy
// works on M5StickC PLUS2 + M5Unified. The upstream main.cpp has been parked;
// reintroduce its features (BLE, clock, menu, prompts, stats) one at a time
// after this boots cleanly.
#include <M5Unified.h>
#include <esp_mac.h>
#include <string.h>
#include "buddy.h"
#include "ble_bridge.h"

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

// Advertise as "Claude-XXXX" (last two BT MAC bytes) so multiple sticks in
// one room are distinguishable in the desktop picker. Matches upstream.
static char btName[16] = "Claude";
static void startBt() {
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_BT);
  snprintf(btName, sizeof(btName), "Claude-%02X%02X", mac[4], mac[5]);
  bleInit(btName);
}

// Full-screen takeover while the 6-digit passkey is on screen. Mirrors
// upstream drawPasskey() but uses fixed colors since the Palette/character
// system isn't wired in this stub.
static void drawPasskey(uint32_t pk) {
  spr.fillSprite(0x0000);
  spr.setTextDatum(MC_DATUM);
  spr.setTextSize(1);
  spr.setTextColor(0x8410, 0x0000);
  spr.drawString("BLUETOOTH PAIRING", W / 2, 56);
  spr.setTextSize(3);
  spr.setTextColor(0xFFFF, 0x0000);
  char b[8]; snprintf(b, sizeof(b), "%06lu", (unsigned long)pk);
  spr.drawString(b, W / 2, 120);
  spr.setTextSize(1);
  spr.setTextColor(0x8410, 0x0000);
  spr.drawString("enter on desktop", W / 2, 184);
  spr.setTextDatum(TL_DATUM);
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
  // M5Unified does NOT auto-init Serial (legacy M5StickCPlus did). Without
  // this, every Serial.print in ble_bridge silently no-ops.
  Serial.begin(115200);
  M5.Display.setRotation(0);
  M5.Display.setBrightness(180);

  spr.createSprite(W, H);
  buddyInit();
  buddySetSpeciesIdx(0);

  startBt();

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

  static bool wasShowingPasskey = false;
  uint32_t pk = blePasskey();
  if (pk) {
    // While pairing: passkey takes the whole screen. BtnA cycling is
    // suppressed so the user isn't fighting the UI mid-pair.
    drawPasskey(pk);
    spr.pushSprite(0, 0);
    wasShowingPasskey = true;
    delay(33);
    return;
  }
  if (wasShowingPasskey) {
    // First tick after passkey clears: wipe and force buddy to repaint
    // everything (buddyTick only clears its own region).
    spr.fillSprite(0x0000);
    drawSpeciesText();
    buddyInvalidate();
    wasShowingPasskey = false;
  }

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
