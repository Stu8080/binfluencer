#include <Arduino.h>
#include "config.h"
#include "wifi_manager.h"
#include "bins_api.h"
#include "led_controller.h"
#include "schedule.h"

// ----------------------------------------------------------------
// Globals
// ----------------------------------------------------------------
static BinSchedule g_schedule;
static uint32_t    g_lastApiRefresh    = 0;
static uint32_t    g_lastScheduleCheck = 0;
static bool        g_apiReady          = false;

// ----------------------------------------------------------------
// Forward declarations
// ----------------------------------------------------------------
bool refreshApi();
void evaluateAndSetLeds();

// ----------------------------------------------------------------
// setup()
// ----------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(500);   // wait for serial monitor to attach
    Serial.println("\n=== Binfluencer booting ===");

    // 1. Initialise LEDs (all off)
    LedController::begin();

    // 2. Connect to WiFi
    if (!WifiManager::connect()) {
        Serial.println("[BOOT] WiFi failed — retrying on next loop.");
    }

    // 3. Sync time via NTP (blocks until time is set, up to ~5 s)
    if (WifiManager::isConnected()) {
        Serial.println("[BOOT] Syncing NTP time...");
        configTzTime(TZ_INFO, NTP_SERVER);

        // Wait for NTP sync (up to 10 s)
        struct tm timeInfo;
        uint8_t attempts = 0;
        while (!getLocalTime(&timeInfo) && attempts < 20) {
            delay(500);
            ++attempts;
        }
        if (getLocalTime(&timeInfo)) {
            char buf[64];
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z", &timeInfo);
            Serial.printf("[BOOT] Local time: %s\n", buf);
        } else {
            Serial.println("[BOOT] NTP sync timed out — time may be wrong.");
        }
    }

    // 4. Fetch premises ID (NVS cache or API) and collection dates
    g_apiReady = refreshApi();

    // 5. Initial LED evaluation
    evaluateAndSetLeds();

    Serial.println("[BOOT] Setup complete.\n");
}

// ----------------------------------------------------------------
// loop()
// ----------------------------------------------------------------
void loop() {
    uint32_t now = millis();

    // --- Re-fetch API once per day ---
    if (g_apiReady && (now - g_lastApiRefresh >= API_REFRESH_MS)) {
        Serial.println("[LOOP] Daily API refresh.");
        refreshApi();
        evaluateAndSetLeds();
    }

    // --- Retry if initial API fetch failed (check every 60 s) ---
    if (!g_apiReady && (now - g_lastApiRefresh >= SCHEDULE_CHECK_MS)) {
        if (WifiManager::ensureConnected()) {
            g_apiReady = refreshApi();
            if (g_apiReady) evaluateAndSetLeds();
        }
    }

    // --- Periodic schedule check ---
    if (g_apiReady && (now - g_lastScheduleCheck >= SCHEDULE_CHECK_MS)) {
        g_lastScheduleCheck = now;
        evaluateAndSetLeds();
    }

    // --- Animate LEDs ---
    LedController::tick();

    delay(20);   // ~50 fps animation tick
}

// ----------------------------------------------------------------
// refreshApi(): fetch premises ID then collection dates.
// Updates g_schedule and g_lastApiRefresh.
// Returns true on success.
// ----------------------------------------------------------------
bool refreshApi() {
    if (!WifiManager::ensureConnected()) {
        Serial.println("[API] No WiFi — skipping refresh.");
        return false;
    }

    String premId = BinsApi::getPremisesId();
    if (premId.isEmpty()) {
        Serial.println("[API] Could not get premises ID.");
        return false;
    }

    if (!BinsApi::getCollectionDates(premId, g_schedule)) {
        Serial.println("[API] Could not get collection dates.");
        return false;
    }

    Schedule::printSchedule(g_schedule);
    g_lastApiRefresh = millis();
    return true;
}

// ----------------------------------------------------------------
// evaluateAndSetLeds(): decide which LED mode to apply right now.
// ----------------------------------------------------------------
void evaluateAndSetLeds() {
    LedController::heartbeat();

    // If today IS a collection day → clear LEDs (bin already out)
    bool recyclingToday    = Schedule::isToday(g_schedule.recyclingDate);
    bool generalWasteToday = Schedule::isToday(g_schedule.generalWasteDate);

    if (recyclingToday || generalWasteToday) {
        Serial.println("[SCHED] Collection day — LEDs off.");
        LedController::setMode(LedMode::OFF);
        return;
    }

    // Evaluate whether this evening is the night before a collection
    bool alertR = false, alertG = false;
    Schedule::evaluate(g_schedule, alertR, alertG);

    if (alertR && alertG) {
        LedController::setMode(LedMode::BOTH);
    } else if (alertR) {
        LedController::setMode(LedMode::RECYCLING);
    } else if (alertG) {
        LedController::setMode(LedMode::GENERAL_WASTE);
    } else {
        LedController::setMode(LedMode::OFF);
    }
}
