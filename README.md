# Binfluencer

A Leeds bin collection reminder built on the Seeed XIAO ESP32-S3. It queries the live Leeds City Council bin API each day and pulses WS2812B LEDs the evening before your bins are due — so you never forget to put them out.

![Binfluencer Exploded View](Binfluencer%20Exploded.png)

---

## How it works

- On boot it connects to WiFi, syncs time via NTP, and fetches your next collection dates from [bins.felixyeung.com](https://bins.felixyeung.com)
- Every **60 seconds** it re-evaluates the schedule (a brief white pulse on the alert strip confirms it's alive)
- Every **24 hours** it re-fetches from the API
- From **7 PM the evening before** a collection, the relevant LEDs start breathing
- On the **collection day itself** the LEDs switch off (bin is already out)

### LED behaviour

| Situation | LEDs |
|-----------|------|
| Recycling tomorrow evening | Green lettering breathes |
| General waste tomorrow evening | BLACK lettering breathes white |
| Both due tomorrow | Sequences: alert strip → black → green (5 s each) |
| Collection day | All off |
| Schedule check heartbeat | Middle 3 alert strip LEDs pulse once at 50% |

Only one LED channel is ever active at a time to minimise peak current draw.

---

## The enclosure

The housing is a 3D printed miniature wheelie bin. All parts are included in this repo. You will need some 2mm brass rod for the lid hinge and wheels.

### Parts

| File | Material | Notes |
|------|----------|-------|
| `Wheelie Bin - Bin.stl` | PLA — any colour | Main bin body. Printed in Sunlu Midnight Black |
| `Wheelie Bin - Lid.stl` | PLA — any colour | Snap-fit lid |
| `Wheelie Bin - BLACK.stl` | Translucent PETG | "BLACK" lettering panel, snaps into front of bin |
| `Wheelie Bin - GREEN.stl` | Translucent PETG | "GREEN" lettering panel, snaps into front of bin |
| `Wheelie Bin - Top Light.stl` | Translucent PETG | Alert strip diffuser, seals between lid and bin body |
| `Wheelie Bin - Part 7.stl` | PLA | F-shaped LED mount and light baffle — isolates the two 3-LED channels |
| `Wheelie Bin - Part 8.stl` | PLA | Electronics backing plate |
| `Wheelie Bin - Wheel 1.stl` | PLA | Decorative wheel |
| `Wheelie Bin - Wheel 2.stl` | PLA | Decorative wheel |

### Assembly

1. Mount the XIAO ESP32-S3 onto the yellow base plate. You can use a little hot glue or double sided tape. Make sure you've wired +Ve and -Ve to 5v and the three data pins for the addressable LED's
2. Attach the 3-LED strips to the front side of the F-shaped baffle — one strip behind the GREEN lettering, one behind BLACK
3. Snap the translucent GREEN and BLACK lettering panels into the front of the bin
4. Seat the F-shaped assembly inside the bin body, theres a small retainer on the walls to snap it into place
5. Mount the 10-LED alert strip horizontally at the top of the bin light panel, facing outwards — it seals against the lid when closed and glows around the lid edge
6. Close the lid

---

## Hardware

| Component | Details |
|-----------|---------|
| Microcontroller | Seeed XIAO ESP32-S3 |
| LEDs | WS2812B — 3× green channel, 3× black channel, 10× alert strip |
| Power | 5V pin on XIAO → LED +ve; GND → LED −ve |
| Data lines | Separate per channel (not daisy-chained) |

### Wiring

> **Note:** On the XIAO ESP32-S3 the physical D-pin labels do not match GPIO numbers from D6 onwards.

| Physical pin | GPIO | Connect to |
|---|---|---|
| D7 | GPIO 44 | BLACK bin LEDs (3×) |
| D8 | GPIO 7 | GREEN bin LEDs (3×) |
| D9 | GPIO 8 | Alert strip (10×) |
| 5V | — | LED +ve (all strips) |
| GND | — | LED −ve (all strips) |

---

## Software

Built with [PlatformIO](https://platformio.org/) for Arduino framework.

### Dependencies

| Library | Version |
|---------|---------|
| FastLED | `~3.9.0` (pinned — 3.10.x has a build issue on ESP-IDF 5.3) |
| ArduinoJson | `^7.0.0` |

### Project structure

```
binfluencer/
├── platformio.ini          ← main firmware
├── src/
│   ├── main.cpp            ← setup / loop / LED evaluation
│   ├── config.example.h    ← copy to config.h and fill in your details
│   ├── bins_api.h          ← Leeds bin collection API client
│   ├── led_controller.h    ← FastLED state machine
│   ├── schedule.h          ← date parsing and alert logic
│   └── wifi_manager.h      ← WiFi connection with backoff
└── test_led/               ← standalone LED test (no WiFi/API needed)
    ├── platformio.ini
    └── src/
        ├── main.cpp        ← cycles alert → black → green, 5 s each
        ├── config.h
        └── led_controller.h
```

---

## Setup

### 1. Configure

```bash
cp src/config.example.h src/config.h
```

Edit `src/config.h`:

```cpp
#define WIFI_SSID     "your-ssid"
#define WIFI_PASSWORD "your-password"

#define BINS_POSTCODE "LS1 1AA"   // your postcode
#define BINS_HOUSE    "1"         // your house number
```

Find your premises on [bins.felixyeung.com](https://bins.felixyeung.com) to confirm your postcode and house number are recognised. The premises ID is cached to flash (NVS) after the first successful lookup.

### 2. Flash

```bash
# Main firmware
cd binfluencer
pio run --target upload

# LED test only (no WiFi required)
cd binfluencer/test_led
pio run --target upload
```

### 3. Monitor

```bash
pio device monitor
```

On a successful boot you should see:

```
=== Binfluencer booting ===
[WiFi] Connected. IP: 192.168.x.x
[BOOT] Local time: 2026-03-01 19:00:00 GMT
[API] Found premises: "73 DEVONSHIRE AVENUE" → id=983201
[Schedule] Next Recycling:    2026-03-06
[Schedule] Next General Waste: 2026-03-13
[BOOT] Setup complete.
```

---

## API

Collection data is sourced from [bins.felixyeung.com](https://bins.felixyeung.com), an unofficial Leeds City Council bin collection API.

```
GET /api/premises?postcode=LS8%201AU
GET /api/jobs?premises={id}
```

The premises ID is stored in NVS after the first lookup so the device only hits the premises endpoint once.

---

## Licence

MIT
