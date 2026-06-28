# Quick Start Guide
## Poker Timer Slave Display - T-Display ESP32

## Upload to Device

### 1. Connect Hardware
- Connect your T-Display ESP32 via USB
- Device should appear as COM port (Windows) or /dev/ttyUSB (Linux)

### 2. Build and Upload
```bash
# From project directory
~/.platformio/penv/Scripts/platformio run --target upload

# Or in VSCode with PlatformIO extension
# Click the Upload arrow icon in the bottom toolbar
```

### 3. Monitor Serial Output
```bash
~/.platformio/penv/Scripts/platformio device monitor

# Or in VSCode
# Click the Serial Monitor plug icon in the bottom toolbar
```

Expected output:
```
Poker Timer Slave Display
T-Display ESP32 (135x240)
Display initialized - 240x135 landscape
Setup complete!
```

## What You'll See

The display shows in landscape orientation (240x135):

**Top:** Round number (e.g., "Round 1")

**Middle Section:**
- "Blinds" label
- Current blinds (25/50, formatted with K/M for large numbers)
- "Ante" label (if ante > 0)
- Ante amount

**Center:** Large countdown timer (15:00)

**Bottom:**
- Status: RUNNING (green) or STOPPED (red)
- Next round preview (e.g., "Next: 50/100")

## Testing the Display

Currently, the device runs in demo mode with:
- Round 1
- Blinds: 25/50
- Ante: 0
- Timer: 15:00 (900 seconds)
- Status: STOPPED (change `state.isRunning = true` in code to auto-countdown)

## Customizing Initial State

Edit `src/main.cpp`, find the `TimerState state = {...}` section (around line 38):

```cpp
TimerState state = {
    1,          // currentRound
    25,         // smallBlind
    50,         // bigBlind
    0,          // ante
    900,        // timeRemaining (seconds)
    true,       // isRunning (set to true for auto-countdown)
    50,         // nextSmallBlind
    100,        // nextBigBlind
    0,          // nextAnte
    false       // nextIsBreak
};
```

After changes:
```bash
~/.platformio/penv/Scripts/platformio run --target upload
```

## Common Issues

### Display Blank
- Check backlight: GPIO 4 should be HIGH
- Try different USB cable/port
- Check serial monitor for errors

### Colors Wrong
- Display may need rotation adjustment
- Try changing `tft.setRotation(1)` to 0, 2, or 3 in `setupDisplay()`

### Upload Fails
- Hold BOOT button (GPIO 0) during upload
- Reset device after upload completes
- Try different baud rate: add `upload_speed = 115200` to platformio.ini

### IntelliSense Errors in VSCode
These are normal before first build. They'll resolve after:
```bash
~/.platformio/penv/Scripts/platformio run
```
Then reload VSCode window (Ctrl+Shift+P > "Reload Window")

## Next Steps

To make this a functional slave device, you'll need to add:

1. **Communication Protocol**
   - ESP-NOW for direct ESP32-to-ESP32 communication
   - WiFi + MQTT for network-based updates
   - Serial communication if connected to master

2. **State Updates**
   - Receive timer updates from master device
   - Update `TimerState` structure with new data
   - Redraw relevant screen sections

3. **Example ESP-NOW Receiver** (pseudo-code):
```cpp
void onDataReceived(const uint8_t *mac, const uint8_t *data, int len) {
    // Parse incoming data
    memcpy(&state, data, sizeof(TimerState));
    
    // Redraw entire screen
    drawMainScreen();
}
```

## Hardware Specs

- **MCU:** ESP32-D0WD (dual-core 240MHz)
- **Display:** ST7789 135x240 TFT
- **RAM:** 520KB SRAM
- **Flash:** 4MB
- **Available GPIO:** 0, 35 (buttons on T-Display board)

## File Structure

```
Poker_Timer_MiniSlave_TDisplay/
├── src/
│   └── main.cpp              # Main application
├── platformio.ini            # PlatformIO config
├── README.md                 # Full documentation
├── QUICK_START.md           # This file
└── .gitignore               # Git ignore rules
```

## Resources

- [PlatformIO Docs](https://docs.platformio.org/)
- [TFT_eSPI Library](https://github.com/Bodmer/TFT_eSPI)
- [LILYGO T-Display](https://github.com/Xinyuan-LilyGO/TTGO-T-Display)
- [Original CYD Project](https://github.com/RFLTools/Poker_Timer_CYD)

## Support

For issues or questions:
1. Check serial monitor output (115200 baud)
2. Review README.md for detailed info
3. Check display pin definitions in platformio.ini
4. Verify hardware connections

---

**Ready to display your poker tournament!** 🎰
