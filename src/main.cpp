/*
 * Poker Timer Slave Display with ESP-NOW
 * For LILYGO T-Display ESP32 (135x240 ST7789)
 * 
 * ESP-NOW Slave device - display only, no controls
 * Syncs with Poker_Timer_Lite_CYD master via ESP-NOW
 * Landscape mode (240x135)
 */

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <esp_now.h>
#include <WiFi.h>

// Initialize display
TFT_eSPI tft = TFT_eSPI();

// ESP-NOW Message Types (matching master)
#define MSG_TIMER_STATE   1
#define MSG_ROUND_CONFIG  2
#define MSG_HEARTBEAT     3

// Message structures (matching master exactly)
struct TimerStateMessage {
    uint8_t messageType;      // MSG_TIMER_STATE
    uint8_t currentRound;
    uint16_t remainingSeconds;
    bool timerRunning;
    uint32_t timestamp;       // For sync verification
};

struct RoundConfigMessage {
    uint8_t messageType;      // MSG_ROUND_CONFIG
    uint8_t roundIndex;       // Which round this configures
    uint16_t duration;
    uint16_t smallBlind;
    uint16_t bigBlind;
    uint16_t ante;
    bool isBreak;
    uint8_t totalRounds;      // Total number of rounds
};

struct HeartbeatMessage {
    uint8_t messageType;      // MSG_HEARTBEAT
    uint8_t slaveId;          // For future use
};

// Round structure
#define MAX_ROUNDS 25
struct Round {
    uint16_t duration;
    uint16_t smallBlind;
    uint16_t bigBlind;
    uint16_t ante;
    bool isBreak;
};

// Global state
Round rounds[MAX_ROUNDS];
uint8_t currentRound = 0;
uint16_t remainingSeconds = 900;
bool timerRunning = false;
uint8_t totalRounds = 25;
unsigned long lastMasterBroadcast = 0;
unsigned long lastHeartbeat = 0;
const unsigned long MASTER_TIMEOUT = 5000;
const unsigned long SLEEP_TIMEOUT = 15000;  // Sleep after 15 seconds no master
const unsigned long WAKE_INTERVAL = 60000000;  // Wake every 60 seconds (microseconds)
const unsigned long LISTEN_DURATION = 5000;  // Listen for 5 seconds when awake
bool connectedToMaster = false;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// No battery monitoring - WiFi blocks ADC2 on ESP32
// Battery charging is automatic via hardware (TP4054 chip)

// Display colors
#define COLOR_BG        TFT_BLACK
#define COLOR_TEXT      TFT_WHITE
#define COLOR_BLINDS    TFT_YELLOW
#define COLOR_ANTE      TFT_CYAN
#define COLOR_TIMER     TFT_GREEN
#define COLOR_RUNNING   TFT_GREEN
#define COLOR_STOPPED   TFT_RED
#define COLOR_HEADER    TFT_ORANGE
#define COLOR_NEXT      TFT_LIGHTGREY

// Screen dimensions (landscape)
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 135

// Function declarations
void setupDisplay();
void setupESPNow();
void sendHeartbeat();
void handleTimerStateMessage(const TimerStateMessage *msg);
void handleRoundConfigMessage(const RoundConfigMessage *msg);
void onESPNowDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);
void drawMainScreen();
void drawHeader();
void drawBlindsAnte();
void drawTimer();
void drawNextRound();
void checkMasterConnection();
void goToSleep();
String formatTime(int seconds);
String formatNumber(int num);

void setup() {
    Serial.begin(115200);
    
    // Check if waking from deep sleep
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
        Serial.println("\n\n🔄 Woke from sleep - checking for master...");
    } else {
        Serial.println("\n\n⚡ Power on - starting up...");
    }
    
    Serial.println("Poker Timer Slave Display - ESP-NOW + Deep Sleep");
    Serial.println("T-Display ESP32 (135x240)");
    Serial.println("Note: Battery charges automatically when USB connected");
    
    // Initialize rounds to empty (will be filled by master configs)
    for (int i = 0; i < MAX_ROUNDS; i++) {
        rounds[i].duration = 900;
        rounds[i].smallBlind = 0;  // 0 indicates not configured yet
        rounds[i].bigBlind = 0;
        rounds[i].ante = 0;
        rounds[i].isBreak = false;
    }
    
    setupDisplay();
    setupESPNow();
    
    // If waking from sleep, listen for master for LISTEN_DURATION
    if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
        Serial.println("📡 Listening for master broadcasts...");
        unsigned long listenStart = millis();
        lastMasterBroadcast = 0;  // Reset so we can detect new broadcasts
        
        // Listen for LISTEN_DURATION (5 seconds)
        while (millis() - listenStart < LISTEN_DURATION) {
            delay(100);
            if (connectedToMaster) {
                Serial.println("✓ Master found! Staying awake");
                break;
            }
        }
        
        // If no master found, go back to sleep
        if (!connectedToMaster) {
            Serial.println("✗ No master found - going back to sleep");
            goToSleep();
        }
    }
    
    drawMainScreen();
    
    Serial.println("Setup complete!");
    Serial.println("Waiting for master broadcasts...");
}

void loop() {
    unsigned long currentTime = millis();
    
    // Send heartbeat every 5 seconds
    if (currentTime - lastHeartbeat >= 5000) {
        lastHeartbeat = currentTime;
        sendHeartbeat();
    }
    
    // Check master connection status (will sleep if disconnected too long)
    checkMasterConnection();
    
    delay(100);
}

void setupDisplay() {
    tft.init();
    tft.setRotation(1);  // Landscape mode (240x135)
    tft.fillScreen(COLOR_BG);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    
    // Turn on backlight
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    
    Serial.println("Display initialized - 240x135 landscape");
}

void setupESPNow() {
    // Set WiFi mode to STA
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());
    
    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("✗ ESP-NOW init failed!");
        return;
    }
    
    Serial.println("✓ ESP-NOW initialized");
    
    // Register receive callback
    esp_now_register_recv_cb(onESPNowDataRecv);
    
    // Add broadcast peer for sending heartbeats
    esp_now_peer_info_t peerInfo = {};
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("✗ Failed to add broadcast peer");
    } else {
        Serial.println("✓ Broadcast peer added");
    }
    
    Serial.println("Device mode: SLAVE");
}

void sendHeartbeat() {
    HeartbeatMessage msg;
    msg.messageType = MSG_HEARTBEAT;
    msg.slaveId = 0;  // For future use
    
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&msg, sizeof(msg));
    
    Serial.print("Slave: Heartbeat sent (");
    Serial.print(sizeof(msg));
    Serial.print(" bytes) - ");
    Serial.println(result == ESP_OK ? "OK" : "FAILED");
}

void onESPNowDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
    if (data_len < 1) return;
    
    uint8_t messageType = data[0];
    
    if (messageType == MSG_TIMER_STATE) {
        if (data_len == sizeof(TimerStateMessage)) {
            handleTimerStateMessage((const TimerStateMessage*)data);
        } else {
            Serial.print("Timer state message size mismatch. Expected ");
            Serial.print(sizeof(TimerStateMessage));
            Serial.print(", got ");
            Serial.println(data_len);
        }
    } else if (messageType == MSG_ROUND_CONFIG) {
        if (data_len == sizeof(RoundConfigMessage)) {
            handleRoundConfigMessage((const RoundConfigMessage*)data);
        }
    }
}

void handleTimerStateMessage(const TimerStateMessage *msg) {
    // Update last sync time
    lastMasterBroadcast = millis();
    
    // Debug output
    static uint8_t lastRound = 255;
    static uint16_t lastSeconds = 0;
    static bool lastRunning = false;
    
    bool stateChanged = (msg->currentRound != lastRound) || 
                        (msg->remainingSeconds != lastSeconds) ||
                        (msg->timerRunning != lastRunning);
    
    if (stateChanged) {
        Serial.print("Slave RX: Round ");
        Serial.print(msg->currentRound + 1);  // Display rounds are 1-based
        Serial.print(", Time ");
        Serial.print(msg->remainingSeconds);
        Serial.print("s, ");
        Serial.println(msg->timerRunning ? "RUNNING" : "PAUSED");
        
        lastRound = msg->currentRound;
        lastSeconds = msg->remainingSeconds;
        lastRunning = msg->timerRunning;
    }
    
    // Mark as connected
    if (!connectedToMaster) {
        connectedToMaster = true;
        Serial.println("✓ Connected to master");
    }
    
    // Update local state
    currentRound = msg->currentRound;
    remainingSeconds = msg->remainingSeconds;
    timerRunning = msg->timerRunning;
    
    // Redraw display if state changed
    if (stateChanged) {
        drawMainScreen();
    }
}

void handleRoundConfigMessage(const RoundConfigMessage *msg) {
    // Slave receives round configuration from master
    if (msg->roundIndex >= MAX_ROUNDS) return;
    
    // Update total rounds if changed
    if (msg->totalRounds != totalRounds) {
        totalRounds = msg->totalRounds;
    }
    
    // Update round configuration
    rounds[msg->roundIndex].duration = msg->duration;
    rounds[msg->roundIndex].smallBlind = msg->smallBlind;
    rounds[msg->roundIndex].bigBlind = msg->bigBlind;
    rounds[msg->roundIndex].ante = msg->ante;
    rounds[msg->roundIndex].isBreak = msg->isBreak;
    
    Serial.print("Slave received config for round ");
    Serial.print(msg->roundIndex + 1);
    if (msg->isBreak) {
        Serial.println(" - BREAK");
    } else {
        Serial.print(" - Blinds: ");
        Serial.print(msg->smallBlind);
        Serial.print("/");
        Serial.println(msg->bigBlind);
    }
    
    // Redraw if this is the current or next round
    if (msg->roundIndex == currentRound || msg->roundIndex == currentRound + 1) {
        drawMainScreen();
    }
}

void drawMainScreen() {
    tft.fillScreen(COLOR_BG);
    drawHeader();
    drawNextRound();
    drawBlindsAnte();
    drawTimer();
}

void drawHeader() {
    // Line 1: "Round x" on left
    tft.setTextColor(COLOR_HEADER, COLOR_BG);
    tft.setTextDatum(TL_DATUM);
    tft.setFreeFont(&FreeSansBold9pt7b);
    String roundText = "Round " + String(currentRound + 1);
    tft.drawString(roundText, 5, 2);
}

void drawBlindsAnte() {
    if (currentRound >= MAX_ROUNDS) return;
    
    // Line 3: "Blinds:" label centered - full width
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setFreeFont(&FreeSans9pt7b);
    tft.drawString("Blinds:", 120, 35);
    
    // Line 4: Blinds value or "BREAK" centered - full width
    if (rounds[currentRound].isBreak) {
        tft.setTextColor(COLOR_BLINDS, COLOR_BG);
        tft.setFreeFont(&FreeSansBold12pt7b);
        tft.drawString("BREAK", 120, 52);
    } else {
        tft.setTextColor(COLOR_BLINDS, COLOR_BG);
        tft.setFreeFont(&FreeSansBold12pt7b);  // Larger font for better readability
        String blindsText;
        if (rounds[currentRound].smallBlind == 0 && rounds[currentRound].bigBlind == 0) {
            blindsText = "...";
        } else {
            blindsText = formatNumber(rounds[currentRound].smallBlind) + "/" + 
                         formatNumber(rounds[currentRound].bigBlind);
        }
        tft.drawString(blindsText, 120, 52);
    }
    
    // Line 5: Ante (only if there is one) - full width
    if (rounds[currentRound].ante > 0) {
        tft.setTextColor(COLOR_ANTE, COLOR_BG);
        tft.setFreeFont(&FreeSansBold9pt7b);
        String anteText = "Ante: " + formatNumber(rounds[currentRound].ante);
        tft.drawString(anteText, 120, 75);
    }
}

void drawTimer() {
    // Bottom: Large timer - green when running, red when paused
    tft.setTextDatum(TC_DATUM);
    if (timerRunning) {
        tft.setTextColor(COLOR_RUNNING, COLOR_BG);  // Green when running
    } else {
        tft.setTextColor(COLOR_STOPPED, COLOR_BG);  // Red when paused
    }
    tft.setTextFont(6);  // Use built-in font 6 (gives room for ante)
    tft.setTextSize(1);
    
    String timeText = formatTime(remainingSeconds);
    tft.drawString(timeText, 120, 90);
}

void drawNextRound() {
    uint8_t nextRound = currentRound + 1;
    if (nextRound >= totalRounds) {
        return;
    }
    
    // Line 1: "Next:" on right
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(COLOR_NEXT, COLOR_BG);
    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.drawString("Next:", SCREEN_WIDTH - 3, 2);
    
    // Line 2: Next round value (right justified) - smaller font for better fit
    tft.setFreeFont(&FreeSans9pt7b);  // Use non-bold font to save space
    String nextValue;
    if (rounds[nextRound].isBreak) {
        nextValue = "BREAK";
    } else if (rounds[nextRound].smallBlind == 0 && rounds[nextRound].bigBlind == 0) {
        nextValue = "...";
    } else {
        nextValue = formatNumber(rounds[nextRound].smallBlind) + "/" + 
                    formatNumber(rounds[nextRound].bigBlind);
    }
    tft.drawString(nextValue, SCREEN_WIDTH - 3, 17);
}

void checkMasterConnection() {
    unsigned long currentTime = millis();
    
    // Check if we've received a broadcast recently
    if (currentTime - lastMasterBroadcast > MASTER_TIMEOUT) {
        if (connectedToMaster) {
            connectedToMaster = false;
            Serial.println("✗ Lost connection to master");
        }
        
        // If no master for SLEEP_TIMEOUT, go to deep sleep
        if (currentTime - lastMasterBroadcast > SLEEP_TIMEOUT) {
            Serial.println("💤 No master found - going to sleep");
            goToSleep();
        }
    }
}

// Battery monitoring disabled - WiFi blocks ADC2 on ESP32
// These functions are not used in this slave-only display
/*
void readBatteryVoltage() {
    // Read battery voltage from ADC
    // T-Display has voltage divider: VBAT -> 100K -> ADC (GPIO 14) -> 100K -> GND
    // So ADC reads VBAT/2
    int rawValue = analogRead(BATTERY_ADC);
    
    // Convert: 12-bit ADC (0-4095) reading 0-3.3V, multiply by 2 for divider
    batteryVoltage = (rawValue / 4095.0) * 3.3 * 2.0;
    
    // Debug output
    Serial.print("Battery ADC: ");
    Serial.print(rawValue);
    Serial.print(" = ");
    Serial.print(batteryVoltage);
    Serial.println("V");
}

bool isUSBConnected() {
    // Read voltage multiple times and average to reduce noise
    float avgVoltage = 0;
    for (int i = 0; i < 5; i++) {
        int raw = analogRead(BATTERY_ADC);
        avgVoltage += (raw / 4095.0) * 3.3 * 2.0;
        delay(10);
    }
    batteryVoltage = avgVoltage / 5.0;
    
    Serial.print("USB Check - Battery: ");
    Serial.print(batteryVoltage);
    Serial.print("V - USB: ");
    
    // When USB connected and charging, voltage will be higher (4.2V+)
    // When on battery only, voltage will be 3.0V-4.2V
    bool usbConnected = (batteryVoltage > USB_DETECT_VOLTAGE);
    Serial.println(usbConnected ? "YES" : "NO");
    
    return usbConnected;
}

void drawChargingScreen() {
    tft.fillScreen(COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_RUNNING, COLOR_BG);
    tft.setFreeFont(&FreeSansBold12pt7b);
    tft.drawString("CHARGING", 120, 50);
    
    // Show battery voltage
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    char voltStr[20];
    sprintf(voltStr, "%.2fV", batteryVoltage);
    tft.drawString(voltStr, 120, 75);
    
    // Show percentage estimate (3.0V = 0%, 4.2V = 100%)
    int percent = (int)((batteryVoltage - 3.0) / 1.2 * 100.0);
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    sprintf(voltStr, "%d%%", percent);
    tft.drawString(voltStr, 120, 95);
}
*/

void goToSleep() {
    Serial.println("💤 Entering deep sleep...");
    Serial.println("💤 Will wake in 60 seconds to check for master");
    
    // Turn off display backlight
    digitalWrite(TFT_BL, LOW);
    
    // Clear screen
    tft.fillScreen(TFT_BLACK);
    
    // Configure wake up timer (60 seconds)
    esp_sleep_enable_timer_wakeup(WAKE_INTERVAL);
    
    // Enter deep sleep
    esp_deep_sleep_start();
}

String formatTime(int seconds) {
    int mins = seconds / 60;
    int secs = seconds % 60;
    
    char buffer[10];
    sprintf(buffer, "%02d:%02d", mins, secs);
    return String(buffer);
}

String formatNumber(int num) {
    if (num >= 1000000) {
        return String(num / 1000000) + "M";
    } else if (num >= 1000) {
        return String(num / 1000) + "K";
    } else {
        return String(num);
    }
}
