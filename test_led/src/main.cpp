#include <Arduino.h>
#include "led_controller.h"

// ----------------------------------------------------------------
// LED cycle test — no WiFi, no API, no NTP.
// Each channel is tested in isolation to keep current draw minimal:
//   Phase 0 (5 s): alert strip only
//   Phase 1 (5 s): black LEDs only
//   Phase 2 (5 s): green LEDs only
// ----------------------------------------------------------------

static const uint32_t PHASE_MS = 5000UL;
static const LedMode  PHASES[] = {
    LedMode::ALERT_ONLY,
    LedMode::GENERAL_WASTE,
    LedMode::RECYCLING,
};
static const char* PHASE_NAMES[] = {"ALERT strip", "BLACK bin", "GREEN bin"};
static const uint8_t NUM_PHASES  = 3;

static uint8_t  phase      = 0;
static uint32_t phaseStart = 0;

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== Binfluencer LED cycle test ===");

    LedController::begin();
    LedController::setMode(PHASES[0]);
    Serial.printf("[TEST] Phase 0 → %s\n", PHASE_NAMES[0]);
    phaseStart = millis();
}

void loop() {
    uint32_t now = millis();

    if (now - phaseStart >= PHASE_MS) {
        phaseStart = now;
        phase      = (phase + 1) % NUM_PHASES;
        LedController::setMode(PHASES[phase]);
        Serial.printf("[TEST] Phase %u → %s\n", phase, PHASE_NAMES[phase]);
    }

    LedController::tick();
    delay(20);
}
