#pragma once

#include <Arduino.h>
#include <time.h>
#include "config.h"

// Holds the next upcoming collection date for each bin type.
// Date stored as "YYYY-MM-DD" (ISO 8601). Empty string = no upcoming date.
struct BinSchedule {
    String recyclingDate;      // next Recycling collection
    String generalWasteDate;   // next General Waste collection
};

namespace Schedule {

// Parse "YYYY-MM-DD" into a struct tm (time fields only; time of day zeroed).
// Returns false if the string is malformed.
static bool parseDate(const String& dateStr, struct tm& out) {
    memset(&out, 0, sizeof(out));
    if (dateStr.length() < 10) return false;
    int y = dateStr.substring(0, 4).toInt();
    int m = dateStr.substring(5, 7).toInt();
    int d = dateStr.substring(8, 10).toInt();
    if (y < 2000 || m < 1 || m > 12 || d < 1 || d > 31) return false;
    out.tm_year = y - 1900;
    out.tm_mon  = m - 1;
    out.tm_mday = d;
    return true;
}

// Returns the current local date as a struct tm (time fields preserved).
static bool getLocalNow(struct tm& out) {
    return getLocalTime(&out);
}

// Compare two dates (year, month, mday only). Returns:
//  -1  if a < b
//   0  if a == b
//   1  if a > b
static int compareDates(const struct tm& a, const struct tm& b) {
    if (a.tm_year != b.tm_year) return a.tm_year < b.tm_year ? -1 : 1;
    if (a.tm_mon  != b.tm_mon)  return a.tm_mon  < b.tm_mon  ? -1 : 1;
    if (a.tm_mday != b.tm_mday) return a.tm_mday < b.tm_mday ? -1 : 1;
    return 0;
}

// Returns true if 'candidate' is exactly one calendar day after 'base'.
static bool isOneDayAfter(const struct tm& base, const struct tm& candidate) {
    // Convert both to time_t (midnight) and check 24-hour difference.
    struct tm a = base;
    struct tm b = candidate;
    a.tm_hour = a.tm_min = a.tm_sec = 0;
    b.tm_hour = b.tm_min = b.tm_sec = 0;
    time_t ta = mktime(&a);
    time_t tb = mktime(&b);
    if (ta == (time_t)-1 || tb == (time_t)-1) return false;
    return (tb - ta) == 86400;
}

// Evaluate which bins need to be alerted based on the current local time
// and the stored schedule.
//
// Alert criteria: today is the day BEFORE the collection date AND
//                 the current local hour >= ALERT_HOUR.
//
// Returns booleans by reference.
void evaluate(const BinSchedule& sched,
              bool& alertRecycling,
              bool& alertGeneralWaste) {
    alertRecycling   = false;
    alertGeneralWaste = false;

    struct tm now;
    if (!getLocalNow(now)) {
        Serial.println("[Schedule] Failed to get local time.");
        return;
    }

    // Only alert in the evening (hour >= ALERT_HOUR) or
    // if collection is today (morning-clear logic handled in loop).
    if (now.tm_hour < ALERT_HOUR) return;

    auto check = [&](const String& dateStr, bool& flag) {
        if (dateStr.isEmpty()) return;
        struct tm collDay;
        if (!parseDate(dateStr, collDay)) return;
        if (isOneDayAfter(now, collDay)) {
            flag = true;
        }
    };

    check(sched.recyclingDate,    alertRecycling);
    check(sched.generalWasteDate, alertGeneralWaste);
}

// Returns true if 'dateStr' is today — used to switch LEDs off on collection day.
bool isToday(const String& dateStr) {
    if (dateStr.isEmpty()) return false;
    struct tm now;
    if (!getLocalNow(now)) return false;
    struct tm coll;
    if (!parseDate(dateStr, coll)) return false;
    return compareDates(now, coll) == 0;
}

// Log helper.
void printSchedule(const BinSchedule& sched) {
    Serial.printf("[Schedule] Next Recycling:    %s\n",
                  sched.recyclingDate.isEmpty()    ? "(none)" : sched.recyclingDate.c_str());
    Serial.printf("[Schedule] Next General Waste: %s\n",
                  sched.generalWasteDate.isEmpty() ? "(none)" : sched.generalWasteDate.c_str());
}

} // namespace Schedule
