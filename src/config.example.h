#pragma once

// ============================================================
// WiFi credentials — fill in before flashing
// ============================================================
#define WIFI_SSID     "your-ssid"
#define WIFI_PASSWORD "your-password"

// ============================================================
// Bin collection address
// Filter: premises whose addressNumber matches BINS_HOUSE
// Find your postcode/house via https://bins.felixyeung.com
// ============================================================
#define BINS_POSTCODE "LS1 1AA"
#define BINS_HOUSE    "1"

// ============================================================
// LED pins (XIAO-ESP32-S3: D7=GPIO44, D8=GPIO7, D9=GPIO8)
// Adjust to match your wiring.
// ============================================================
#define PIN_GREEN_BIN    7  // D8 — 3 LEDs, green recycling bin
#define PIN_BLACK_BIN   44  // D7 — 3 LEDs, black general waste bin
#define PIN_ALERT_STRIP  8  // D9 — 10 LEDs, general alert strip

#define LED_COUNT_GREEN 3
#define LED_COUNT_BLACK 3
#define LED_COUNT_ALERT 10

// ============================================================
// Alert window — LEDs activate on the EVENING BEFORE collection
// ============================================================
#define ALERT_HOUR 19   // 7:00 PM — adjust to taste (24-hour clock)
#define ALERT_MIN   0

// ============================================================
// API re-poll interval — once per day
// ============================================================
#define API_REFRESH_MS (24UL * 60UL * 60UL * 1000UL)

// ============================================================
// Schedule check interval — how often loop() evaluates LED state
// ============================================================
#define SCHEDULE_CHECK_MS (60UL * 1000UL)   // every 60 seconds

// ============================================================
// NTP / timezone
// ============================================================
#define NTP_SERVER "pool.ntp.org"
#define TZ_INFO    "GMT0BST,M3.5.0/1,M10.5.0"   // UK (handles BST/GMT automatically)

// ============================================================
// NVS (Preferences) storage
// ============================================================
#define NVS_NAMESPACE  "binfluencer"
#define NVS_KEY_PREM   "prem_id"

// ============================================================
// LED animation
// ============================================================
#define BREATHE_PERIOD_MS 3000   // breathing cycle length in ms
#define BREATHE_MIN_VAL     20   // minimum brightness (0-255)
#define BREATHE_MAX_VAL    220   // maximum brightness (0-255)
