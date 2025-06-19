/* Tunnel Sump Pump Controller (7609) [WN] */
// Enable serial print for debugging
//#define BLYNK_PRINT Serial

// Include header files
#include "ArduinoBlynkDeviceInfo.h" // Blynk device information
#include <BlynkSimpleWiFiNINA.h>    // Blynk WiFi library
#include "ArduinoWiFiInfo.h"        // WiFi information

// Provide WiFi credentials
char ssid[] = SSID;
char pass[] = PASS;

// Declare global variables for DI, DO and flags
int PUMP1 = 0;
int PUMP2 = 0;
int SENA = 0;
int SENB = 0;
int SPEAM = 0;
int A1A2, A1A2X, P1, P2, L1R, L2R, L1RX, L2RX, F1, F2, PP1, PP2, LRA, LL, CP1, ML, HL, SA, SB;
int WLI = 0;
int WLI_old = 0;
int RSSI_count = 14;
int wifi_status = WL_IDLE_STATUS;
long RSSI = -100;
bool blynk_status = false;
bool high_level_flag = false;
bool middle_level_flag = false;
bool first_level_flag = false;
bool low_level_flag = false;
bool long_running_flag = false;
bool auto_mode_flag = false;
bool manual_mode_flag = false;
bool p1_fault_flag = false;
bool p2_fault_flag = false;
bool p1_running_flag = false;
bool p2_running_flag = false;
unsigned long p1_run_time;
unsigned long p2_run_time;
unsigned long initial_long_running_set_time = ILRA_SET_MINUTE * 60000;
bool p1_initial_long_running_flag = false;
bool p2_initial_long_running_flag = false;
bool standby_pump_early_assist_enable = false;

// Create Blynk timer object
BlynkTimer timer;

BLYNK_CONNECTED() {
  // Reset control switches and read RSSI
  RSSI = WiFi.RSSI();
  Blynk.virtualWrite(V0, PUMP1);   // Pump 1 Remote Run
  Blynk.virtualWrite(V1, PUMP2);   // Pump 2 Remote Run
  Blynk.virtualWrite(V19, RSSI);   // RSSI
  Blynk.syncVirtual(V0, V1, V21);  // Sync switches
  RSSI_count = 14;
}

// DO for Pump 1
BLYNK_WRITE(V0)
{
  PUMP1 = param.asInt();
  if (PUMP1 == 1) {
    digitalWrite(8, HIGH);
    BLYNK_LOG1("PUMP 1 RUNNING (REMOTE)");
  }
  else {
    digitalWrite(8, LOW);
    BLYNK_LOG1("PUMP 1 STOPPED (REMOTE)");
  }
}

// DO for Pump 2
BLYNK_WRITE(V1)
{
  PUMP2 = param.asInt();
  if (PUMP2 == 1) {
    digitalWrite(12, HIGH);
    BLYNK_LOG1("PUMP 2 RUNNING (REMOTE)");
  }
  else {
    digitalWrite(12, LOW);
    BLYNK_LOG1("PUMP 2 STOPPED (REMOTE)");
  }
}

// DO for Sensor A
BLYNK_WRITE(V22)
{
  SENA = param.asInt();
  if (SENA == 1) {
    digitalWrite(4, HIGH);
    delay(3000);
    digitalWrite(4, LOW);
    BLYNK_LOG1("SENSOR A ON/OFF TRIGGERED (REMOTE)");
  }
  else {
    digitalWrite(4, HIGH);
    delay(3000);
    digitalWrite(4, LOW);
    BLYNK_LOG1("SENSOR A ON/OFF TRIGGERED (REMOTE)");
  }
}

// DO for Sensor B
BLYNK_WRITE(V23)
{
  SENB = param.asInt();
  if (SENB == 1) {
    digitalWrite(7, HIGH);
    delay(3000);
    digitalWrite(7, LOW);
    BLYNK_LOG1("SENSOR B ON/OFF TRIGGERED (REMOTE)");
  }
  else {
    digitalWrite(7, HIGH);
    delay(3000);
    digitalWrite(7, LOW);
    BLYNK_LOG1("SENSOR B ON/OFF TRIGGERED (REMOTE)");
  }
}

// Standby Pump Early Assist Mode Switch
BLYNK_WRITE(V21)
{
  SPEAM = param.asInt();
  if (SPEAM == 1) {
    standby_pump_early_assist_enable = true;
    BLYNK_LOG1("STANDBY PUMP EARLY ASSIST MODE ENABLED (REMOTE)");
  }
  else {
    PUMP1 = 0;
    digitalWrite(8, LOW);
    Blynk.virtualWrite(V0, 0);
    PUMP2 = 0;
    digitalWrite(12, LOW);
    Blynk.virtualWrite(V1, 0);
    standby_pump_early_assist_enable = false;
    BLYNK_LOG1("STANDBY PUMP EARLY ASSIST MODE DISABLED (REMOTE)");
  }
}

// Read and write DI values (Part 1)
void processDI_1()
{
  // Read DI values
  A1A2 = digitalRead(A0); // Auto Mode (Pump 1 OR Pump 2)
  P1 = digitalRead(A1);   // Pump 1 Power
  P2 = digitalRead(A2);   // Pump 2 Power
  F1 = digitalRead(5);    // Pump 1 Common Fault
  F2 = digitalRead(6);    // Pump 2 Common Fault
  PP1 = digitalRead(10);  // High Level Backup 1
  PP2 = digitalRead(11);  // High Level Backup 2
  LRA = digitalRead(13);  // Long Running
  SA = digitalRead(A4);  // Sensor A
  SB = digitalRead(A5);  // Sensor B
  A1A2X = 1 - A1A2; // Manual Mode (Pump 1 AND Pump 2, opposite of Auto Mode)

  // Write DI values to datastreams
  Blynk.beginGroup();
  Blynk.virtualWrite(V16, A1A2); // Auto Mode (Pump 1 OR Pump 2)
  Blynk.virtualWrite(V2, P1);    // Pump 1 Power
  Blynk.virtualWrite(V3, P2);    // Pump 2 Power
  Blynk.virtualWrite(V8, F1);    // Pump 1 Common Fault
  Blynk.virtualWrite(V9, F2);    // Pump 2 Common Fault
  Blynk.virtualWrite(V11, PP1);  // High Level Backup 1
  Blynk.virtualWrite(V12, PP2);  // High Level Backup 2
  Blynk.virtualWrite(V13, LRA);  // Long Running
  Blynk.virtualWrite(V24, SA);  // Sensor A
  Blynk.virtualWrite(V25, SB);  // Sensor B
  Blynk.endGroup();

  // Read RSSI and write to datastream
  RSSI_count -= 1;
  wifi_status = WiFi.status();
  if (wifi_status == WL_CONNECTED && RSSI_count <= 0) {
    RSSI = WiFi.RSSI();
    Blynk.virtualWrite(V19, RSSI);   // RSSI
    RSSI_count = 14;
  }

  // Auto Mode Event
  if (A1A2 == 1 && auto_mode_flag == false) {
    Blynk.logEvent("auto_mode");
    BLYNK_LOG1("AUTO MODE SELECTED");
    auto_mode_flag = true;
  }
  else if (A1A2 == 0 && auto_mode_flag == true) {
    auto_mode_flag = false;
  }

  // Manual Mode Event
  if (A1A2X == 1 && manual_mode_flag == false) {
    Blynk.logEvent("manual_mode");
    BLYNK_LOG1("MANUAL MODE SELECTED");
    manual_mode_flag = true;
  }
  else if (A1A2X == 0 && manual_mode_flag == true) {
    manual_mode_flag = false;
  }

  // Pump 1 Common Fault Event
  if (F1 == 1 && p1_fault_flag == false) {
    Blynk.logEvent("p1_fault");
    BLYNK_LOG1("PUMP 1 COMMON FAULT ALARM ON");
    p1_fault_flag = true;
  }
  else if (F1 == 0 && p1_fault_flag == true) {
    p1_fault_flag = false;
    BLYNK_LOG1("PUMP 1 COMMON FAULT ALARM RESET");
  }

  // Pump 2 Common Fault Event
  if (F2 == 1 && p2_fault_flag == false) {
    Blynk.logEvent("p2_fault");
    BLYNK_LOG1("PUMP 2 COMMON FAULT ALARM ON");
    p2_fault_flag = true;
  }
  else if (F2 == 0 && p2_fault_flag == true) {
    p2_fault_flag = false;
    BLYNK_LOG1("PUMP 2 COMMON FAULT ALARM RESET");
  }

  // Long Running Event
  if (LRA == 1 && long_running_flag == false) {
    Blynk.logEvent("long_running");
    BLYNK_LOG1("LONG RUNNING ALARM ON");
    long_running_flag = true;
  }
  else if (LRA == 0 && long_running_flag == true) {
    long_running_flag = false;
    BLYNK_LOG1("LONG RUNNING ALARM RESET");
  }
}

// Read and write DI values (Part 2)
void processDI_2()
{
  // Read DI values
  LL = digitalRead(0);   // Low Level
  L1R = digitalRead(1);  // Pump 1 Running
  L2R = digitalRead(2);  // Pump 2 Running
  ML = digitalRead(3);   // Middle Level
  HL = digitalRead(9);   // High Level
  CP1 = 1 - digitalRead(A3); // First Level
  L1RX = 1 - L1R; // Pump 1 Stopped (Opposite of Running)
  L2RX = 1 - L2R; // Pump 2 Stopped (Opposite of Running)

 // Determine water level 
  if (HL == 1) {
    WLI = 6;
  }
  else if (ML == 1) {
    WLI = 5;
  }
  else if (CP1 == 1) {
    WLI = 4;
  }
  else if (LL == 1) {
    WLI = 2;
  }
  else {
    WLI = 3;
  }

  // Write DI values to datastreams
  Blynk.beginGroup();
  Blynk.virtualWrite(V4, LL);    // Low Level
  Blynk.virtualWrite(V17, CP1);  // First Level
  Blynk.virtualWrite(V7, ML);    // Middle Level
  Blynk.virtualWrite(V10, HL);   // High Level
  Blynk.virtualWrite(V5, L1R);   // Pump 1 Running
  Blynk.virtualWrite(V14, L1RX); // Pump 1 Stopped
  Blynk.virtualWrite(V6, L2R);   // Pump 2 Running
  Blynk.virtualWrite(V15, L2RX); // Pump 2 Stopped
  Blynk.virtualWrite(V20, WLI);  // Water Level Index
  Blynk.endGroup();

  // Change in Water Level Index
  if (WLI != WLI_old) {
    Blynk.virtualWrite(V18, WLI); // Water Level Index Change
    WLI_old = WLI;
  }

  // Low Level Event
  if (LL == 1 && low_level_flag == false) {
    Blynk.logEvent("low_level");
    BLYNK_LOG1("LOW LEVEL ALARM ON");
    low_level_flag = true;
  }
  else if (LL == 0 && low_level_flag == true) {
    low_level_flag = false;
    BLYNK_LOG1("LOW LEVEL ALARM RESET");
  }

  // First Level Event
  if (CP1 == 1 && first_level_flag == false) {
    Blynk.logEvent("first_level");
    BLYNK_LOG1("LIRST LEVEL ALARM ON");
    first_level_flag = true;
  }
  else if (CP1 == 0 && first_level_flag == true) {
    first_level_flag = false;
    BLYNK_LOG1("FIRST LEVEL ALARM RESET");
  }
  
  // Middle Level Event
  if (ML == 1 && middle_level_flag == false) {
    Blynk.logEvent("middle_level");
    BLYNK_LOG1("MIDDLE LEVEL ALARM ON");
    middle_level_flag = true;
  }
  else if (ML == 0 && middle_level_flag == true) {
    middle_level_flag = false;
    BLYNK_LOG1("MIDDLE LEVEL ALARM RESET");
  }

  // High Level Event
  if (HL == 1 && high_level_flag == false) {
    Blynk.logEvent("high_level");
    BLYNK_LOG1("HIGH LEVEL ALARM ON");
    high_level_flag = true;
  }
  else if (HL == 0 && high_level_flag == true) {
    high_level_flag = false;
    BLYNK_LOG1("HIGH LEVEL ALARM RESET");
  }

  // Pump 1 Running and Stopped Event
  if (L1R == 1 && p1_running_flag == false) {
    Blynk.logEvent("p1_running");
    BLYNK_LOG1("PUMP 1 RUNNING");
    p1_running_flag = true;
    p1_run_time = millis();
  }
  else if (L1R == 0 && p1_running_flag == true) {
    p1_running_flag = false;
    Blynk.logEvent("p1_stopped");
    BLYNK_LOG1("PUMP 1 STOPPED");
    if (p1_initial_long_running_flag == true) {
      p1_initial_long_running_flag = false;
      BLYNK_LOG1("PUMP 1 INITIAL LONG RUNNING ALARM RESET");
      if (standby_pump_early_assist_enable == true) {
        PUMP2 = 0;
        digitalWrite(12, LOW);
        Blynk.virtualWrite(V1, 0);
      }
    }
  }
  if (p1_running_flag == true && p1_initial_long_running_flag == false) {
    if (millis() - p1_run_time > initial_long_running_set_time) {
      p1_initial_long_running_flag = true;
      Blynk.logEvent("p1_initial_long_running");
      BLYNK_LOG1("PUMP 1 INITIAL LONG RUNNING ALARM ON");
    }
  }
  if (p1_initial_long_running_flag == true && standby_pump_early_assist_enable == true && p2_running_flag == false) {
    PUMP2 = 1;
    digitalWrite(12, HIGH);
    Blynk.virtualWrite(V1, 1);
  }

  // Pump 2 Running and Stopped Event
  if (L2R == 1 && p2_running_flag == false) {
    Blynk.logEvent("p2_running");
    BLYNK_LOG1("PUMP 2 RUNNING");
    p2_running_flag = true;
    p2_run_time = millis();
  }
  else if (L2R == 0 && p2_running_flag == true) {
    p2_running_flag = false;
    Blynk.logEvent("p2_stopped");
    BLYNK_LOG1("PUMP 2 STOPPED");
    if (p2_initial_long_running_flag == true) {
      p2_initial_long_running_flag = false;
      BLYNK_LOG1("PUMP 2 INITIAL LONG RUNNING ALARM RESET");
      if (standby_pump_early_assist_enable == true) {
        PUMP1 = 0;
        digitalWrite(8, LOW);
        Blynk.virtualWrite(V0, 0);
      }
    }
  }
  if (p2_running_flag == true && p2_initial_long_running_flag == false) {
    if (millis() - p2_run_time > initial_long_running_set_time) {
      p2_initial_long_running_flag = true;
      Blynk.logEvent("p2_initial_long_running");
      BLYNK_LOG1("PUMP 2 INITIAL LONG RUNNING ALARM ON");
    }
  }
  if (p2_initial_long_running_flag == true && standby_pump_early_assist_enable == true && p1_running_flag == false) {
    PUMP1 = 1;
    digitalWrite(8, HIGH);
    Blynk.virtualWrite(V0, 1);
  }
}

// Function for checking connection status
void checkConnectionStatus() {
  wifi_status = WiFi.status();
  if (wifi_status != WL_CONNECTED) {
    // Reset relays when disconnected from Blynk server
    PUMP1 = 0;
    digitalWrite(8, LOW);
    PUMP2 = 0;
    digitalWrite(12, LOW);
    BLYNK_LOG1("Reconnecting to WiFi...");
    WiFi.disconnect();
    Blynk.connectWiFi(ssid, pass);
    wifi_status = WiFi.status();
  }
  if (wifi_status == WL_CONNECTED) {
    BLYNK_LOG3("Connected to WiFi (RSSI: ", RSSI, " dBm).");
    blynk_status = Blynk.connected();
    if (blynk_status) {
      BLYNK_LOG1("Connected to Blynk server.");
    }
    else {
      BLYNK_LOG1("Reconnecting to Blynk server...");
      Blynk.config(BLYNK_AUTH_TOKEN);
      Blynk.connect();
    }
  }
}

// Setup function
void setup() 
{
  // Set up debug console
  //Serial.begin(9600);
  BLYNK_LOG1("Setting up device...");

  // Set up WiFi connection
  Blynk.connectWiFi(ssid, pass);

  // Set up Blynk server connection
  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();

  // Set up Blynk timer object
  timer.setInterval(3000L, processDI_1);
  delay(1500);
  timer.setInterval(3000L, processDI_2);
  delay(1000);
  timer.setInterval(60000L, checkConnectionStatus);
  
  // Set pin mode for DI pins
  pinMode(A0, INPUT);        // Auto Mode (PUMP1 OR PUMP2)
  pinMode(A1, INPUT);        // Pump 1 Power
  pinMode(A2, INPUT);        // Pump 2 Power
  pinMode(A3, INPUT_PULLUP); // First Level
  pinMode(A4, INPUT_PULLUP); // Sensor A
  pinMode(A5, INPUT_PULLUP); // Sensor B
  pinMode(0, INPUT);         // Low Level
  pinMode(1, INPUT);         // Pump 1 Running
  pinMode(2, INPUT);         // Pump 2 Running
  pinMode(3, INPUT);         // Middle Level
  pinMode(5, INPUT);         // Pump 1 Common Fault
  pinMode(6, INPUT);         // Pump 2 Common Fault
  pinMode(9, INPUT);         // High Level
  pinMode(10, INPUT);        // High Level Backup 1
  pinMode(11, INPUT);        // High Level Backup 2
  pinMode(13, INPUT);        // Long Running

  // Set pin mode for DO pins
  pinMode(4, OUTPUT);  // Relay 1 for Sensor A
  pinMode(7, OUTPUT);  // Relay 2 for Sensor B
  pinMode(8, OUTPUT);  // Relay 3 for Pump 1
  pinMode(12, OUTPUT); // Relay 4 for Pump 2
}

// Main function
void loop() 
{
  Blynk.run(); // Run Blynk
  timer.run(); // Run Blynk timer
}
