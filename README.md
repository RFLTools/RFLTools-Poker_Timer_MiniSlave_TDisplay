# Poker Timer Slave Display
## LILYGO T-Display ESP32 (135x240)

A slave-only poker tournament timer display for the TENSTAR T-Display ESP32-D0WD with ST7789 135x240 LCD in landscape mode.

## Features

- **Display Only** - No controls, pure display device
- **Main Screen Shows:**
  - Current round number
  - Small/Big blinds (formatted with K/M suffixes)
  - Ante amount (if applicable)
  - Large countdown timer (MM:SS format)
  - Running/Stopped status
  - Next round blinds preview

## Hardware

- **Board:** TENSTAR T-Display ESP32-D0WD
- **Display:** ST7789 135x240 TFT
- **Orientation:** Landscape (240x135)

### Pin Configuration

The T-Display uses these pins for the ST7789 display:
- MOSI: GPIO 19
- SCLK: GPIO 18
- CS: GPIO 5
- DC: GPIO 16
- RST: GPIO 23
- BL (Backlight): GPIO 4

## Display Layout (Landscape 240x135)

```
┌────────────────────────────────────┐
│ Round 1                            │
│                                    │
│           Blinds                   │
│           25/50                    │
│                                    │
│          15:00                     │
│                                    │
│         RUNNING                    │
│                                    │
│ Next: 50/100                       │
└────────────────────────────────────┘
```

## Building & Uploading

### Using PlatformIO

```bash
# Build
pio run

# Upload
pio run --target upload

# Monitor
pio run --target monitor
```

### Using PlatformIO IDE

1. Open this folder in VSCode with PlatformIO extension
2. Click "Build" (checkmark icon)
3. Click "Upload" (arrow icon)
4. Click "Serial Monitor" to view debug output

## Configuration

All display settings are configured in `platformio.ini`:

- Display driver: ST7789
- Resolution: 135x240
- Rotation: 1 (Landscape mode)
- SPI Frequency: 40MHz

## Current Implementation

This is a **demonstration/template** version with:
- Hardcoded timer state
- Auto-countdown when running
- No network/communication (yet)

### Timer State Structure

```cpp
struct TimerState {
    int currentRound;      // Current round number
    int smallBlind;        // Small blind amount
    int bigBlind;          // Big blind amount
    int ante;              // Ante amount (0 if none)
    int timeRemaining;     // Seconds remaining
    bool isRunning;        // Timer running state
    int nextSmallBlind;    // Next round small blind
    int nextBigBlind;      // Next round big blind
    int nextAnte;          // Next round ante
    bool nextIsBreak;      // Next round is break
};
```

## Future Enhancements

To make this a fully functional slave device, you could add:

1. **WiFi Communication**
   - ESP-NOW for low-latency updates from master
   - MQTT for cloud-based synchronization
   - WebSocket connection to master device

2. **Settings Persistence**
   - Save last known state to SPIFFS/Preferences
   - Auto-reconnect to master on boot

3. **Physical Controls** (optional)
   - Two buttons on T-Display for basic control
   - GPIO 0 and GPIO 35 available

4. **Audio/Visual Alerts**
   - Buzzer on GPIO (if added)
   - Screen flash on round change
   - Color changes for warnings

## Display Colors

Defined in `main.cpp`:
- Background: Black
- Headers: Orange
- Blinds: Yellow
- Ante: Cyan
- Timer: Green
- Status Running: Green
- Status Stopped: Red
- Next Round: Light Grey

## Dependencies

- TFT_eSPI (v2.5.43+) - Display driver
- ArduinoJson (v6.21.3+) - JSON parsing for future network features

## Serial Monitor

Connect at **115200 baud** to see:
- Initialization messages
- Display setup confirmation
- Timer updates
- Round changes

## Troubleshooting

### Display not working
- Check USB connection
- Verify correct board selected in platformio.ini
- Check backlight pin (GPIO 4)

### Colors wrong
- Try adjusting `tft.setRotation()`
- Check ST7789_DRIVER is defined

### Text garbled
- Ensure TFT_eSPI fonts are loading
- Check SPI frequency settings

## Integration with Poker_Timer_Lite_CYD

This slave display is designed to work with the [Poker_Timer_Lite_CYD](https://github.com/RFLTools/Poker_Timer_Lite_CYD) master device, which supports:

- **ESP-NOW Multi-Device Sync** - Built-in support for unlimited slave displays
- **Bidirectional Control** - Any device can control the timer
- **~200m Range** - Outdoors, ~50m through walls
- **<10ms Latency** - Near-instant synchronization
- **Auto-Reconnect** - Automatic recovery from connection loss

### Next Steps for Full Integration

To connect this T-Display as a slave to the CYD master:

1. Add ESP-NOW library to `platformio.ini`
2. Implement ESP-NOW receiver to get `TimerState` updates from master
3. Optionally add buttons (GPIO 0, 35) for bidirectional control
4. See [ESPNOW_SYNC.md](https://github.com/RFLTools/Poker_Timer_Lite_CYD/blob/main/ESPNOW_SYNC.md) in the main project for protocol details

## License

Based on the [Poker_Timer_Lite_CYD](https://github.com/RFLTools/Poker_Timer_Lite_CYD) project by RFLTools.

**NON-COMMERCIAL USE ONLY**

This software is licensed for personal, non-commercial use only. Commercial use is strictly prohibited without explicit permission.

## Acknowledgments

- Based on [Poker_Timer_Lite_CYD](https://github.com/RFLTools/Poker_Timer_Lite_CYD) by RFLTools
- Built with PlatformIO
- TFT_eSPI library by Bodmer
- LILYGO T-Display hardware
