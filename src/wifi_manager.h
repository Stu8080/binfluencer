#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

namespace WifiManager {

// Attempts to connect to WiFi with exponential backoff.
// Blocks until connected or maxAttempts exhausted (returns false on failure).
bool connect(uint8_t maxAttempts = 10) {
    Serial.printf("[WiFi] Connecting to \"%s\"\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t delayMs = 500;
    for (uint8_t attempt = 1; attempt <= maxAttempts; ++attempt) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("[WiFi] Connected. IP: %s\n",
                          WiFi.localIP().toString().c_str());
            return true;
        }
        Serial.printf("[WiFi] Attempt %u/%u — waiting %u ms...\n",
                      attempt, maxAttempts, delayMs);
        delay(delayMs);
        if (delayMs < 8000) delayMs *= 2;   // exponential backoff, cap at 8 s
    }

    Serial.println("[WiFi] Failed to connect.");
    return false;
}

// Returns true if currently connected.
bool isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

// Re-connects if disconnected. Returns true if connection is established.
bool ensureConnected() {
    if (isConnected()) return true;
    Serial.println("[WiFi] Connection lost — reconnecting...");
    return connect();
}

} // namespace WifiManager
