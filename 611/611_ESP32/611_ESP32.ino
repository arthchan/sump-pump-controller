/* Tunnel Sump Pump Controller (611) [ESP32] */
// Enable serial print for debugging
//#define BLYNK_PRINT Serial

// Include header files
#include "ArduinoBlynkDeviceInfo.h" // Blynk device information
#include <WiFiS3.h>                 // Blynk WiFi library
#include <BlynkSimpleWifi.h>        // Blynk WiFi library
#include "ArduinoWiFiInfo.h"        // WiFi information

// Provide WiFi credentials
char ssid[] = SSID;
char pass[] = PASS;

// Declare global variables for DI, DO and flags
int PUMP1 = 0;
int PUMP2 = 0;
int R1, R2, R3, R4, R3X, R4X, PF1, PF2, OL1, OL2, UR1, UR2, LL, ML, HL, HHL;
int WLI = 0;
int WLI_old = 0;
int RSSI_count = 14;
int wifi_status = WL_IDLE_STATUS;
long RSSI = -100;
bool blynk_status = false;
bool high_level_flag = false;
bool second_preset_level_flag = false;
bool first_preset_level_flag = false;
bool low_level_flag = false;
bool auto_mode_flag = false;
bool manual_mode_flag = false;
bool p1_fault_flag = false;
bool p2_fault_flag = false;
bool p1_heat_cut_flag = false;
bool p2_heat_cut_flag = false;
bool p1_running_flag = false;
bool p2_running_flag = false;
unsigned long p1_run_time;
unsigned long p2_run_time;
unsigned long initial_long_running_set_time = ILRA_SET_MINUTE * 60000;
bool p1_initial_long_running_flag = false;
bool p2_initial_long_running_flag = false;

// Create Blynk timer object
BlynkTimer timer;

BLYNK_CONNECTED() {
  // Reset control switches and read RSSI
  RSSI = WiFi.RSSI();
  Blynk.virtualWrite(V0, PUMP1);   // Pump 1 Remote Run
  Blynk.virtualWrite(V1, PUMP2);   // Pump 2 Remote Run
  Blynk.virtualWrite(V19, RSSI);   // RSSI
  Blynk.syncVirtual(V0, V1);       // Sync switches
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

// Read and write DI values (Part 1)
void processDI_1()
{
  // Read DI values
  R2 = digitalRead(A0);  // Auto Mode
  PF1 = digitalRead(A1); // Pump 1 Power
  PF2 = digitalRead(A2); // Pump 2 Power
  OL1 = digitalRead(5);  // Pump 1 Common Fault
  OL2 = digitalRead(6);  // Pump 2 Common Fault
  UR1 = digitalRead(10); // Pump 1 Heat Cut
  UR2 = digitalRead(11); // Pump 2 Heat Cut
  R1 = 1 - R2; // Manual Mode (Opposite of Auto Mode)

  // Write DI values to datastreams
  Blynk.beginGroup();
  Blynk.virtualWrite(V16, R2);  // Auto Mode
  Blynk.virtualWrite(V2, PF1);  // Pump 1 Power
  Blynk.virtualWrite(V3, PF2);  // Pump 2 Power
  Blynk.virtualWrite(V8, OL1);  // Pump 1 Common Fault
  Blynk.virtualWrite(V9, OL2);  // Pump 2 Common Fault
  Blynk.virtualWrite(V11, UR1); // Pump 1 Heat Cut
  Blynk.virtualWrite(V12, UR2); // Pump 2 Heat Cut
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
  if (R2 == 1 && auto_mode_flag == false) {
    Blynk.logEvent("auto_mode");
    BLYNK_LOG1("AUTO MODE SELECTED");
    auto_mode_flag = true;
  }
  else if (R2 == 0 && auto_mode_flag == true) {
    auto_mode_flag = false;
  }

  // Manual Mode Event
  if (R1 == 1 && manual_mode_flag == false) {
    Blynk.logEvent("manual_mode");
    BLYNK_LOG1("MANUAL MODE SELECTED");
    manual_mode_flag = true;
  }
  else if (R1 == 0 && manual_mode_flag == true) {
    manual_mode_flag = false;
  }

  // Pump 1 Common Fault Event
  if (OL1 == 1 && p1_fault_flag == false) {
    Blynk.logEvent("p1_fault");
    BLYNK_LOG1("PUMP 1 COMMON FAULT ALARM ON");
    p1_fault_flag = true;
  }
  else if (OL1 == 0 && p1_fault_flag == true) {
    p1_fault_flag = false;
    BLYNK_LOG1("PUMP 1 COMMON FAULT ALARM RESET");
  }

  // Pump 2 Common Fault Event
  if (OL2 == 1 && p2_fault_flag == false) {
    Blynk.logEvent("p2_fault");
    BLYNK_LOG1("PUMP 2 COMMON FAULT ALARM ON");
    p2_fault_flag = true;
  }
  else if (OL2 == 0 && p2_fault_flag == true) {
    p2_fault_flag = false;
    BLYNK_LOG1("PUMP 2 COMMON FAULT ALARM RESET");
  }

  // Pump 1 Heat Cut Event
  if (UR1 == 1 && p1_heat_cut_flag == false) {
    Blynk.logEvent("p1_heat_cut");
    BLYNK_LOG1("PUMP 1 HEAT CUT ALARM ON");
    p1_heat_cut_flag = true;
  }
  else if (UR1 == 0 && p1_heat_cut_flag == true) {
    p1_heat_cut_flag = false;
    BLYNK_LOG1("PUMP 1 HEAT CUT ALARM RESET");
  }

  // Pump 2 Heat Cut Event
  if (UR2 == 1 && p2_heat_cut_flag == false) {
    Blynk.logEvent("p2_heat_cut");
    BLYNK_LOG1("PUMP 2 HEAT CUT ALARM ON");
    p2_heat_cut_flag = true;
  }
  else if (UR2 == 0 && p2_heat_cut_flag == true) {
    p2_heat_cut_flag = false;
    BLYNK_LOG1("PUMP 2 HEAT CUT ALARM RESET");
  }
}

// Read and write DI values (Part 2)
void processDI_2()
{
  // Read DI values
  LL = digitalRead(0);   // Low Level
  R3 = digitalRead(1);   // Pump 1 Running
  R4 = digitalRead(2);   // Pump 2 Running
  ML = digitalRead(3);   // First Preset Level
  HL = digitalRead(9);   // Second Preset Level
  HHL = digitalRead(13); // High Level
  R3X = 1 - R3; // Pump 1 Stopped (Opposite of Running)
  R4X = 1 - R4; // Pump 2 Stopped (Opposite of Running)

  // Determine water level
  if (HHL == 1) {
    WLI = 6;
  }
  else if (HL == 1) {
    WLI = 5;
  }
  else if (ML == 1) {
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
  Blynk.virtualWrite(V4, LL);   // Low Level
  Blynk.virtualWrite(V7, ML);   // First Preset Level
  Blynk.virtualWrite(V13, HL);  // Second Preset Level
  Blynk.virtualWrite(V10, HHL); // High Level
  Blynk.virtualWrite(V5, R3);   // Pump 1 Running
  Blynk.virtualWrite(V14, R3X); // Pump 1 Stopped
  Blynk.virtualWrite(V6, R4);   // Pump 2 Running
  Blynk.virtualWrite(V15, R4X); // Pump 2 Stopped
  Blynk.virtualWrite(V17, WLI); // Water Level Index
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

  // First Preset Level Event
  if (ML == 1 && first_preset_level_flag == false) {
    Blynk.logEvent("first_preset_level");
    BLYNK_LOG1("FIRST PRESET LEVEL ALARM ON");
    first_preset_level_flag = true;
  }
  else if (ML == 0 && first_preset_level_flag == true) {
    first_preset_level_flag = false;
    BLYNK_LOG1("FIRST PRESET LEVEL ALARM RESET");
  }

  // Second Preset Level Event
  if (HL == 1 && second_preset_level_flag == false) {
    Blynk.logEvent("second_preset_level");
    BLYNK_LOG1("SECOND PRESET LEVEL ALARM ON");
    second_preset_level_flag = true;
  }
  else if (HL == 0 && second_preset_level_flag == true) {
    second_preset_level_flag = false;
    BLYNK_LOG1("SECOND PRESET LEVEL ALARM RESET");
  }

  // High Level Event
  if (HHL == 1 && high_level_flag == false) {
    Blynk.logEvent("high_level");
    BLYNK_LOG1("HIGH LEVEL ALARM ON");
    high_level_flag = true;
  }
  else if (HHL == 0 && high_level_flag == true) {
    high_level_flag = false;
    BLYNK_LOG1("HIGH LEVEL ALARM RESET");
  }

  // Pump 1 Running and Stopped Event
  if (R3 == 1 && p1_running_flag == false) {
    Blynk.logEvent("p1_running");
    BLYNK_LOG1("PUMP 1 RUNNING");
    p1_running_flag = true;
    p1_run_time = millis();
  }
  else if (R3 == 0 && p1_running_flag == true) {
    p1_running_flag = false;
    Blynk.logEvent("p1_stopped");
    BLYNK_LOG1("PUMP 1 STOPPED");
    if (p1_initial_long_running_flag == true) {
      p1_initial_long_running_flag = false;
      BLYNK_LOG1("PUMP 1 INITIAL LONG RUNNING ALARM RESET");
    }
  }
  if (p1_running_flag == true && p1_initial_long_running_flag == false) {
    if (millis() - p1_run_time > initial_long_running_set_time) {
      p1_initial_long_running_flag = true;
      Blynk.logEvent("p1_initial_long_running");
      BLYNK_LOG1("PUMP 1 INITIAL LONG RUNNING ALARM ON");
    }
  }

  // Pump 2 Running and Stopped Event
  if (R4 == 1 && p2_running_flag == false) {
    Blynk.logEvent("p2_running");
    BLYNK_LOG1("PUMP 2 RUNNING");
    p2_running_flag = true;
    p2_run_time = millis();
  }
  else if (R4 == 0 && p2_running_flag == true) {
    p2_running_flag = false;
    Blynk.logEvent("p2_stopped");
    BLYNK_LOG1("PUMP 2 STOPPED");
    if (p2_initial_long_running_flag == true) {
      p2_initial_long_running_flag = false;
      BLYNK_LOG1("PUMP 2 INITIAL LONG RUNNING ALARM RESET");
    }
  }
  if (p2_running_flag == true && p2_initial_long_running_flag == false) {
    if (millis() - p2_run_time > initial_long_running_set_time) {
      p2_initial_long_running_flag = true;
      Blynk.logEvent("p2_initial_long_running");
      BLYNK_LOG1("PUMP 2 INITIAL LONG RUNNING ALARM ON");
    }
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
  pinMode(A0, INPUT); // Auto Mode
  pinMode(A1, INPUT); // Pump 1 Power
  pinMode(A2, INPUT); // Pump 2 Power
  pinMode(0, INPUT);  // Low Level
  pinMode(1, INPUT);  // Pump 1 Running
  pinMode(2, INPUT);  // Pump 2 Running
  pinMode(3, INPUT);  // First Preset Level
  pinMode(5, INPUT);  // Pump 1 Common Fault
  pinMode(6, INPUT);  // Pump 2 Common Fault
  pinMode(9, INPUT);  // Second Preset Level
  pinMode(10, INPUT); // Pump 1 Heat Cut
  pinMode(11, INPUT); // Pump 2 Heat Cut
  pinMode(13, INPUT); // High Level

  // Set pin mode for DO pins
  pinMode(8, OUTPUT);  // Relay 3 for Pump 1
  pinMode(12, OUTPUT); // Relay 4 for Pump 2
}

// Main function
void loop() 
{
  Blynk.run(); // Run Blynk
  timer.run(); // Run Blynk timer
}
