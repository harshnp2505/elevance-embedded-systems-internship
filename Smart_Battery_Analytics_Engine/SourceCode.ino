#define BLYNK_TEMPLATE_ID "TMPTMPL3KD_IAK4Y"
#define BLYNK_TEMPLATE_NAME "Smart Battery Analytics Engine"
#define BLYNK_AUTH_TOKEN "aQiTjkNG8DmowL4IsGFxSvb5h_P90QLp"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

char ssid[] = "Wokwi-GUEST";
char pass[] = "";

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pins
const int cellPins[4] = {34, 35, 32, 33};
const int relayPin = 25;
const int buzzerPin = 26;

// Runtime Modes
enum RuntimeMode {
  NORMAL,
  DEGRADED,
  FAILSAFE,
  SHUTDOWN
};

RuntimeMode currentMode = NORMAL;
RuntimeMode lastLoggedMode = NORMAL;

// Globals
float cellVoltages[4];
unsigned long previousMillis = 0;
const long monitoringInterval = 1000;

String faultHistory = "BOOT";
String fault2 = "";
String fault3 = "";
String previousFault = "";

unsigned long relayLastChanged = 0;
const unsigned long relayCooldown = 2000;
bool relayState = true;

// ===============================
// SENSOR FUNCTIONS
// ===============================

void readVoltages() {
  for (int i = 0; i < 4; i++) {
    int adc = analogRead(cellPins[i]);
    cellVoltages[i] = (adc * 3.3) / 4095.0;
  }
}

bool checkSensorDisconnect() {
  for (int i = 0; i < 4; i++) {
    if (cellVoltages[i] < 0.2) return true;
  }
  return false;
}

// ===============================
// ANALYTICS ENGINE
// ===============================

float calculateAverageVoltage() {
  float sum = 0;
  for (int i = 0; i < 4; i++) sum += cellVoltages[i];
  return sum / 4.0;
}

float calculatePackVoltage() {
  float sum = 0;
  for (int i = 0; i < 4; i++) sum += cellVoltages[i];
  return sum;
}

float calculateImbalance() {
  float minV = cellVoltages[0];
  float maxV = cellVoltages[0];

  for (int i = 1; i < 4; i++) {
    if (cellVoltages[i] < minV) minV = cellVoltages[i];
    if (cellVoltages[i] > maxV) maxV = cellVoltages[i];
  }

  return maxV - minV;
}

int calculateBatteryHealth() {
  int score = 100;
  float avgVoltage = calculateAverageVoltage();
  float imbalance = calculateImbalance();

  if (avgVoltage < 2.0) score -= 30;
  else if (avgVoltage < 2.7) score -= 15;

  if (imbalance > 0.5) score -= 20;
  else if (imbalance > 0.25) score -= 10;

  switch (currentMode) {
    case DEGRADED: score -= 10; break;
    case FAILSAFE: score -= 30; break;
    case SHUTDOWN: score -= 50; break;
    default: break;
  }

  if (score < 0) score = 0;
  return score;
}

int calculateFailureProbability() {
  return 100 - calculateBatteryHealth();
}

// ===============================
// AI ENGINE
// ===============================

String modeToString(RuntimeMode mode) {
  switch (mode) {
    case NORMAL: return "NORMAL";
    case DEGRADED: return "DEGRADED";
    case FAILSAFE: return "FAILSAFE";
    case SHUTDOWN: return "SHUTDOWN";
  }
  return "UNKNOWN";
}

String getRiskLevel() {
  switch (currentMode) {
    case NORMAL: return "LOW";
    case DEGRADED: return "MODERATE";
    case FAILSAFE: return "HIGH";
    case SHUTDOWN: return "CRITICAL";
  }
  return "UNKNOWN";
}

String getAIRecommendation() {
  switch (currentMode) {
    case NORMAL: return "System healthy";
    case DEGRADED: return "Inspect weak cell";
    case FAILSAFE: return "Reduce load now";
    case SHUTDOWN: return "Manual intervention";
  }
  return "N/A";
}

String getSensorStatus() {
  if (checkSensorDisconnect()) return "FAULT";
  return "HEALTHY";
}

String getRelayStatus() {
  if (currentMode == FAILSAFE || currentMode == SHUTDOWN)
    return "OFF";
  return "ACTIVE";
}

String getCloudStatus() {
  if (Blynk.connected()) return "ONLINE";
  return "OFFLINE";
}

int getWiFiRSSI() {
  return WiFi.RSSI();
}

String getFaultHistory() {
  return faultHistory + " | " + fault2 + " | " + fault3;
}

void updateFaultHistory() {
  String currentFault = modeToString(currentMode);

  if (currentFault != previousFault) {
    fault3 = fault2;
    fault2 = faultHistory;
    faultHistory = currentFault;
    previousFault = currentFault;
  }
}

// ===============================
// OUTPUT CONTROL
// ===============================

void handleOutputs() {
  bool desiredRelayState = true;

  if (currentMode == FAILSAFE || currentMode == SHUTDOWN)
    desiredRelayState = false;

  if (desiredRelayState != relayState &&
      millis() - relayLastChanged > relayCooldown) {
    relayState = desiredRelayState;
    digitalWrite(relayPin, relayState);
    relayLastChanged = millis();
  }

  switch (currentMode) {
    case NORMAL:
      noTone(buzzerPin);
      break;
    case DEGRADED:
      tone(buzzerPin, 1000, 200);
      break;
    case FAILSAFE:
      tone(buzzerPin, 1800);
      break;
    case SHUTDOWN:
      tone(buzzerPin, 2500);
      break;
  }
}

void updateLCD() {
  static String lastLine1 = "";
  static String lastLine2 = "";

  String line1 = modeToString(currentMode);
  String line2 = getRiskLevel();

  if (line1 != lastLine1 || line2 != lastLine2) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);

    lcd.setCursor(0, 1);
    lcd.print(line2);

    lastLine1 = line1;
    lastLine2 = line2;
  }
}
// ===============================
// BLYNK TELEMETRY
// ===============================

void sendToBlynk() {
  if (!Blynk.connected()) return;

  Blynk.virtualWrite(V0, cellVoltages[0]);
  Blynk.virtualWrite(V1, cellVoltages[1]);
  Blynk.virtualWrite(V2, cellVoltages[2]);
  Blynk.virtualWrite(V3, cellVoltages[3]);

  Blynk.virtualWrite(V4, calculateAverageVoltage());
  Blynk.virtualWrite(V5, calculatePackVoltage());
  Blynk.virtualWrite(V6, calculateBatteryHealth());
  Blynk.virtualWrite(V7, getRiskLevel());

  Blynk.virtualWrite(V8, getSensorStatus());
  Blynk.virtualWrite(V9, getRelayStatus());
  Blynk.virtualWrite(V10, getCloudStatus());

  Blynk.virtualWrite(V11, getAIRecommendation());
  Blynk.virtualWrite(V12, getFaultHistory());
  Blynk.virtualWrite(V13, calculateFailureProbability());
  Blynk.virtualWrite(V14, getWiFiRSSI());
}

// ===============================
// SETUP
// ===============================

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
  relayState = true;

  pinMode(buzzerPin, OUTPUT);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("BATTERY AI");
  lcd.setCursor(0, 1);
  lcd.print("BOOTING...");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  delay(1500);
  lcd.clear();
}

// ===============================
// LOOP
// ===============================

void loop() {
  Blynk.run();

  if (!Blynk.connected()) {
    Blynk.connect(1000);
  }

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= monitoringInterval) {
    previousMillis = currentMillis;

    readVoltages();

    bool sensorDisconnected = checkSensorDisconnect();

    float maxV = cellVoltages[0];
    for (int i = 1; i < 4; i++) {
      if (cellVoltages[i] > maxV)
        maxV = cellVoltages[i];
    }

    bool overVoltage = maxV > 3.10;
    bool imbalanceFault = calculateImbalance() > 0.5;

    // =========================
    // Runtime State Machine
    // =========================
    if (sensorDisconnected && overVoltage) {
      currentMode = SHUTDOWN;
    }
    else if (overVoltage) {
      currentMode = FAILSAFE;
    }
    else if (sensorDisconnected || imbalanceFault) {
      currentMode = DEGRADED;
    }
    else {
      currentMode = NORMAL;
    }

    updateFaultHistory();

    if (currentMode != lastLoggedMode) {
      Serial.print("MODE CHANGED TO: ");
      Serial.println(modeToString(currentMode));
      lastLoggedMode = currentMode;
    }

    handleOutputs();
    updateLCD();
    sendToBlynk();

    // =========================
    // Serial Monitor
    // =========================
    Serial.println("===== TASK 6 LIVE =====");

    for (int i = 0; i < 4; i++) {
      Serial.print("Cell ");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(cellVoltages[i], 2);
      Serial.println(" V");
    }

    Serial.print("Avg Voltage: ");
    Serial.println(calculateAverageVoltage());

    Serial.print("Pack Voltage: ");
    Serial.println(calculatePackVoltage());

    Serial.print("Health: ");
    Serial.print(calculateBatteryHealth());
    Serial.println("%");

    Serial.print("Risk: ");
    Serial.println(getRiskLevel());

    Serial.print("WiFi RSSI: ");
    Serial.println(getWiFiRSSI());

    Serial.print("AI: ");
    Serial.println(getAIRecommendation());

    Serial.println("=======================");
  }
}
