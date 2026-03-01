#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include "config.h"

// ----------------------------------------------------------------
// LED state machine
// Only one channel is ever active at a time to minimise peak
// current draw and avoid voltage-drop flicker.
// ----------------------------------------------------------------
enum class LedMode {
    OFF,
    ALERT_ONLY,     // alert strip breathes; bin LEDs off
    RECYCLING,      // green LEDs breathe; alert + black off
    GENERAL_WASTE,  // black LEDs breathe; alert + green off
    BOTH            // sequences: alert(5s) → black(5s) → green(5s)
};

namespace LedController {

static CRGB ledsGreen[LED_COUNT_GREEN];
static CRGB ledsBlack[LED_COUNT_BLACK];
static CRGB ledsAlert[LED_COUNT_ALERT];

static LedMode  currentMode  = LedMode::OFF;
static uint8_t  seqPhase     = 0;
static uint32_t seqPhaseStart = 0;

static const uint32_t SEQ_PHASE_MS = 5000UL;   // 5 s per phase in BOTH mode

// Call once in setup().
void begin() {
    FastLED.addLeds<WS2812B, PIN_GREEN_BIN,   GRB>(ledsGreen, LED_COUNT_GREEN);
    FastLED.addLeds<WS2812B, PIN_BLACK_BIN,   GRB>(ledsBlack, LED_COUNT_BLACK);
    FastLED.addLeds<WS2812B, PIN_ALERT_STRIP, GRB>(ledsAlert, LED_COUNT_ALERT);
    FastLED.setBrightness(200);
    FastLED.clear(true);
}

// Set the current mode. Animation is applied in tick().
void setMode(LedMode mode) {
    if (mode == currentMode) return;
    currentMode   = mode;
    seqPhase      = 0;
    seqPhaseStart = millis();
    const char* names[] = {"OFF", "ALERT_ONLY", "RECYCLING", "GENERAL_WASTE", "BOTH"};
    Serial.printf("[LED] Mode → %s\n", names[static_cast<int>(mode)]);
    FastLED.clear(true);   // blank all strips immediately on every transition
}

LedMode getMode() { return currentMode; }

// Sine-wave breathing value (0-255).
static uint8_t breathe(uint32_t now_ms, uint32_t phase_ms = 0) {
    uint32_t t     = (now_ms + phase_ms) % BREATHE_PERIOD_MS;
    uint8_t  angle = map(t, 0, BREATHE_PERIOD_MS - 1, 0, 255);
    uint8_t  sinVal = sin8(angle);
    return map(sinVal, 0, 255, BREATHE_MIN_VAL, BREATHE_MAX_VAL);
}

// Must be called frequently from loop() to animate LEDs.
void tick() {
    if (currentMode == LedMode::OFF) return;

    uint32_t now = millis();

    // Advance BOTH-mode phase (alert → black → green, 5 s each).
    if (currentMode == LedMode::BOTH &&
        now - seqPhaseStart >= SEQ_PHASE_MS) {
        seqPhase      = (seqPhase + 1) % 3;
        seqPhaseStart = now;
        FastLED.clear(true);   // blank between phases to avoid ghosting
        const char* phaseNames[] = {"alert", "black", "green"};
        Serial.printf("[LED] BOTH phase → %s\n", phaseNames[seqPhase]);
    }

    uint8_t bri = breathe(now);

    // Determine which single channel to light.
    bool doAlert = false, doBlack = false, doGreen = false;

    switch (currentMode) {
        case LedMode::ALERT_ONLY:    doAlert = true; break;
        case LedMode::RECYCLING:     doGreen = true; break;
        case LedMode::GENERAL_WASTE: doBlack = true; break;
        case LedMode::BOTH:
            if      (seqPhase == 0) doAlert = true;
            else if (seqPhase == 1) doBlack = true;
            else                    doGreen = true;
            break;
        default: break;
    }

    fill_solid(ledsAlert, LED_COUNT_ALERT, doAlert ? CRGB(bri, bri, bri) : CRGB::Black);
    fill_solid(ledsBlack, LED_COUNT_BLACK, doBlack ? CRGB(bri, bri, bri) : CRGB::Black);
    fill_solid(ledsGreen, LED_COUNT_GREEN, doGreen ? CRGB(0, bri, 0)     : CRGB::Black);

    FastLED.show();
}

} // namespace LedController
