#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "config.h"
#include "schedule.h"

namespace BinsApi {

static const char* BASE_URL = "https://bins.felixyeung.com";

// ----------------------------------------------------------------
// HTTP helper — performs a GET and returns the response body.
// Returns empty string on failure.
// ----------------------------------------------------------------
static String httpGet(const String& url) {
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(10000);   // 10 s timeout

    if (!http.begin(url)) {
        Serial.printf("[API] http.begin() failed for: %s\n", url.c_str());
        return "";
    }

    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("[API] HTTP %d for: %s\n", code, url.c_str());
        http.end();
        return "";
    }

    String body = http.getString();
    http.end();
    return body;
}

// ----------------------------------------------------------------
// Step 1: Fetch premises ID for BINS_POSTCODE / BINS_HOUSE.
// Tries NVS cache first; falls back to API call.
// Returns empty string on failure.
// ----------------------------------------------------------------
String getPremisesId() {
    // --- Try NVS cache ---
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, /*readOnly=*/false);
    String cached = prefs.getString(NVS_KEY_PREM, "");
    if (!cached.isEmpty()) {
        Serial.printf("[API] Premises ID loaded from NVS: %s\n", cached.c_str());
        prefs.end();
        return cached;
    }

    // --- Fetch from API ---
    String postcode = String(BINS_POSTCODE);
    postcode.replace(" ", "%20");
    String url = String(BASE_URL) + "/api/premises?postcode=" + postcode;
    Serial.printf("[API] Fetching premises: %s\n", url.c_str());

    String body = httpGet(url);
    if (body.isEmpty()) {
        prefs.end();
        return "";
    }

    // Parse JSON array of premises objects
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        Serial.printf("[API] JSON parse error (premises): %s\n", err.c_str());
        prefs.end();
        return "";
    }

    // Find the entry whose addressNumber matches BINS_HOUSE
    String house = String(BINS_HOUSE);
    JsonArray arr = doc["data"].as<JsonArray>();
    for (JsonObject obj : arr) {
        String num = obj["addressNumber"].as<String>();
        if (num == house) {
            String id = String(obj["id"].as<long>());
            if (!id.isEmpty()) {
                String street = obj["addressStreet"].as<String>();
                Serial.printf("[API] Found premises: \"%s %s\" → id=%s\n",
                              num.c_str(), street.c_str(), id.c_str());
                prefs.putString(NVS_KEY_PREM, id);
                prefs.end();
                return id;
            }
        }
    }

    Serial.printf("[API] No premises found matching house \"%s\".\n", BINS_HOUSE);
    prefs.end();
    return "";
}

// ----------------------------------------------------------------
// Step 2: Fetch collection jobs for a given premises ID.
// Populates `sched` with the next upcoming date for each bin type.
// Returns true on success.
// ----------------------------------------------------------------
bool getCollectionDates(const String& premisesId, BinSchedule& sched) {
    sched.recyclingDate    = "";
    sched.generalWasteDate = "";

    String url = String(BASE_URL) + "/api/jobs?premises=" + premisesId;
    Serial.printf("[API] Fetching jobs: %s\n", url.c_str());

    String body = httpGet(url);
    if (body.isEmpty()) return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        Serial.printf("[API] JSON parse error (jobs): %s\n", err.c_str());
        return false;
    }

    // Get current date string for comparison (YYYY-MM-DD)
    struct tm now;
    if (!getLocalTime(&now)) {
        Serial.println("[API] Cannot get local time for date comparison.");
        return false;
    }
    char todayBuf[11];
    snprintf(todayBuf, sizeof(todayBuf), "%04d-%02d-%02d",
             now.tm_year + 1900, now.tm_mon + 1, now.tm_mday);
    String today(todayBuf);

    JsonArray jobs = doc["data"]["jobs"].as<JsonArray>();
    if (jobs.isNull()) {
        // Some API responses wrap the array differently; try top-level array
        jobs = doc.as<JsonArray>();
    }

    for (JsonObject job : jobs) {
        String bin  = job["bin"].as<String>();
        String date = job["date"].as<String>();

        if (date.isEmpty() || date < today) continue;   // skip past dates

        if (bin == "GREEN") {
            if (sched.recyclingDate.isEmpty() || date < sched.recyclingDate) {
                sched.recyclingDate = date;
            }
        } else if (bin == "BLACK") {
            if (sched.generalWasteDate.isEmpty() || date < sched.generalWasteDate) {
                sched.generalWasteDate = date;
            }
        }
    }

    return true;
}

// ----------------------------------------------------------------
// Convenience: clear the cached premises ID from NVS.
// (Useful if the premises list changes.)
// ----------------------------------------------------------------
void clearCachedPremisesId() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.remove(NVS_KEY_PREM);
    prefs.end();
    Serial.println("[API] Cleared cached premises ID from NVS.");
}

} // namespace BinsApi
