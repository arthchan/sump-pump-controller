/* Tunnel Sump Pump Controller (7609) [WN_PLRA] */
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
int P1 = 0;
int P2 = 0;
int D3, D4, D5, D6, D7, D8, D9, D10, D11, D12, D13, D14, D15, D16, D17, D18;
int D19 = 0;
int RSSI_count = 14;
int wifi_status = WL_IDLE_STATUS;
long RSSI = -100;
bool blynk_status = false;
bool high_level_flag = false;
bool middle_level_flag = false;
bool low_level_flag = false;
bool long_running_flag = false;
bool auto_mode_flag = false;
bool manual_mode_flag = false;
bool p1_fault_flag = false;
bool p2_fault_flag = false;
bool high_level_bu1_flag = false;
bool high_level_bu2_flag = false;
bool p1_running_flag = false;
bool p2_running_flag = false;
unsigned long p1_run_time;
unsigned long p2_run_time;
int pre_long_running_period = PLRA_PERIOD * 1000;
bool p1_pre_long_running_flag = false;
bool p2_pre_long_running_flag = false;

// Create Blynk timer object
BlynkTimer timer;

BLYNK_CONNECTED() {
  // Reset control switches and read RSSI
  RSSI = WiFi.RSSI();
  Blynk.virtualWrite(V0, P1);   // Pump 1 Remote Run
  Blynk.virtualWrite(V1, P2);   // Pump 2 Remote Run
  Blynk.virtualWrite(V19, RSSI);   // RSSI
  RSSI_count = 14;
}

// DO for Pump 1
BLYNK_WRITE(V0)
{
  P1 = param.asInt();
  if (P1 == 1) {
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
  P2 = param.asInt();
  if (P2 == 1) {
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
  D3 = digitalRead(A0);  // Auto Mode (P1 OR P2)
  D4 = digitalRead(A1);  // Pump 1 Power
  D5 = digitalRead(A2);  // Pump 2 Power
  D10 = digitalRead(5);  // Pump 1 Common Fault
  D11 = digitalRead(6);  // Pump 2 Common Fault
  D15 = digitalRead(13); // Long Running
  D18 = 1 - D3; // Manual Mode (P1 AND P2, opposite of Auto Mode)

  // Write DI values to datastreams
  Blynk.beginGroup();
  Blynk.virtualWrite(V16, D3);  // Auto Mode (P1 OR P2)
  Blynk.virtualWrite(V2, D4);   // Pump 1 Power
  Blynk.virtualWrite(V3, D5);   // Pump 2 Power
  Blynk.virtualWrite(V8, D10);  // Pump 1 Common Fault
  Blynk.virtualWrite(V9, D11);  // Pump 2 Common Fault
  Blynk.virtualWrite(V13, D15); // Long Running
  Blynk.endGroup();

  // Read RSSI and write to datastream
  RSSI_count -= 1;
  wifi_status = WiFi.status();
  if (wifi_status == WL_CONNECTED && RSSI_count <= 0) {
    RSSI = WiFi.RSSI();
    BLYNK_LOG3("Connected to WiFi (RSSI: ", RSSI, " dBm).");
    Blynk.virtualWrite(V19, RSSI);   // RSSI
    RSSI_count = 14;
  }

  // Auto mode event
  if (D3 > 0 && auto_mode_flag == false) {
    Blynk.logEvent("auto_mode");
    BLYNK_LOG1("AUTO MODE SELECTED");
    auto_mode_flag = true;
  }
  else if (D3 == 0 && auto_mode_flag == true) {
    auto_mode_flag = false;
  }

  // Manual mode event
  if (D18 > 0 && manual_mode_flag == false) {
    Blynk.logEvent("manual_mode");
    BLYNK_LOG1("MANUAL MODE SELECTED");
    manual_mode_flag = true;
  }
  else if (D18 == 0 && manual_mode_flag == true) {
    manual_mode_flag = false;
  }

  // P1 common fault event
  if (D10 > 0 && p1_fault_flag == false) {
    Blynk.logEvent("p1_fault");
    BLYNK_LOG1("PUMP 1 COMMON FAULT ALARM ON");
    p1_fault_flag = true;
  }
  else if (D10 == 0 && p1_fault_flag == true) {
    p1_fault_flag = false;
    BLYNK_LOG1("PUMP 1 COMMON FAULT ALARM RESET");
  }

  // P2 common fault event
  if (D11 > 0 && p2_fault_flag == false) {
    Blynk.logEvent("p2_fault");
    BLYNK_LOG1("PUMP 2 COMMON FAULT ALARM ON");
    p2_fault_flag = true;
  }
  else if (D11 == 0 && p2_fault_flag == true) {
    p2_fault_flag = false;
    BLYNK_LOG1("PUMP 2 COMMON FAULT ALARM RESET");
  }

  // Long running event
  if (D15 > 0 && long_running_flag == false) {
    Blynk.logEvent("long_running");
    BLYNK_LOG1("LONG RUNNING ALARM ON");
    long_running_flag = true;
  }
  else if (D15 == 0 && long_running_flag == true) {
    long_running_flag = false;
    BLYNK_LOG1("LONG RUNNING ALARM RESET");
  }
}

// Read and write DI values (Part 2)
void processDI_2()
{
  // Read DI values
  D6 = digitalRead(0);   // Low Level
  D7 = digitalRead(1);   // Pump 1 Running
  D8 = digitalRead(2);   // Pump 2 Running
  D9 = digitalRead(3);   // Middle Level
  D12 = digitalRead(9);  // High Level
  D13 = digitalRead(10); // High Level Backup 1
  D14 = digitalRead(11); // High Level Backup 2
  D16 = 1 - D7; // Pump 1 Stopped (Opposite of Running)
  D17 = 1 - D8; // Pump 2 Stopped (Opposite of Running)

 // Determine water level index
  if (D14 == 1) {
    D19 = 7;
  }
  else if (D13 == 1) {
    D19 = 6;
  }
  else if (D12 == 1) {
    D19 = 5;
  }
  else if (D9 == 1) {
    D19 = 4;
  }
  else if (D6 == 1) {
    D19 = 2;
  }
  else {
    D19 = 3;
  }

  // Write DI values to datastreams
  Blynk.beginGroup();
  Blynk.virtualWrite(V4, D6);   // Low Level
  Blynk.virtualWrite(V5, D7);   // Pump 1 Running
  Blynk.virtualWrite(V14, D16); // Pump 1 Stopped
  Blynk.virtualWrite(V6, D8);   // Pump 2 Running
  Blynk.virtualWrite(V15, D17); // Pump 2 Stopped
  Blynk.virtualWrite(V7, D9);   // Middle Level
  Blynk.virtualWrite(V10, D12); // High Level
  Blynk.virtualWrite(V11, D13); // High Level Backup 1
  Blynk.virtualWrite(V12, D14); // High Level Backup 2
  Blynk.virtualWrite(V18, D19); // Water Level Index
  Blynk.endGroup();

  // Low level event
  if (D6 > 0 && low_level_flag == false) {
    Blynk.logEvent("low_level");
    BLYNK_LOG1("LOW LEVEL ALARM ON");
    low_level_flag = true;
  }
  else if (D6 == 0 && low_level_flag == true) {
    low_level_flag = false;
    BLYNK_LOG1("LOW LEVEL ALARM RESET");
  }

  // Pump 1 running and stopped event
  if (D7 > 0 && p1_running_flag == false) {
    Blynk.logEvent("p1_running");
    BLYNK_LOG1("PUMP 1 RUNNING");
    p1_running_flag = true;
    p1_run_time = millis();
  }
  else if (D7 == 0 && p1_running_flag == true) {
    p1_running_flag = false;
    Blynk.logEvent("p1_stopped");
    BLYNK_LOG1("PUMP 1 STOPPED");
    if (p1_pre_long_running_flag == true) {
      p1_pre_long_running_flag = false;
      BLYNK_LOG1("PUMP 1 PRE-LONG RUNNING ALARM RESET");
      P2 = 0;
      digitalWrite(12, LOW);
      BLYNK_LOG1("PUMP 2 STOPPED (PLRA)");
    }
  }
  if (p1_running_flag == true && p1_pre_long_running_flag == false) {
    if (millis() - p1_run_time > pre_long_running_period) {
      Blynk.logEvent("p1_pre_long_running");
      BLYNK_LOG1("PUMP 1 PRE-LONG RUNNING ALARM ON");
      P2 = 1;
      digitalWrite(12, HIGH);
      BLYNK_LOG1("PUMP 2 RUNNING (PLRA)");
      p1_pre_long_running_flag = true;
    }
  }

  // Pump 2 running and stopped event
  if (D8 > 0 && p2_running_flag == false) {
    Blynk.logEvent("p2_running");
    BLYNK_LOG1("PUMP 2 RUNNING");
    p2_running_flag = true;
    p2_run_time = millis();
  }
  else if (D8 == 0 && p2_running_flag == true) {
    p2_running_flag = false;
    Blynk.logEvent("p2_stopped");
    BLYNK_LOG1("PUMP 2 STOPPED");
    if (p2_pre_long_running_flag == true) {
      p2_pre_long_running_flag = false;
      BLYNK_LOG1("PUMP 2 PRE-LONG RUNNING ALARM RESET");
      P1 = 0;
      digitalWrite(8, LOW);
      BLYNK_LOG1("PUMP 1 STOPPED (PLRA)");
    }
  }
  if (p2_running_flag == true && p2_pre_long_running_flag == false) {
    if (millis() - p2_run_time > pre_long_running_period) {
      Blynk.logEvent("p2_pre_long_running");
      BLYNK_LOG1("PUMP 2 PRE-LONG RUNNING ALARM ON");
      P1 = 1;
      digitalWrite(8, HIGH);
      BLYNK_LOG1("PUMP 1 RUNNING (PLRA)");
      p2_pre_long_running_flag = true;
    }
  }

  // Middle level event
  if (D9 > 0 && middle_level_flag == false) {
    Blynk.logEvent("middle_level");
    BLYNK_LOG1("MIDDLE LEVEL ALARM ON");
    middle_level_flag = true;
  }
  else if (D9 == 0 && middle_level_flag == true) {
    middle_level_flag = false;
    BLYNK_LOG1("MIDDLE LEVEL ALARM RESET");
  }

  // High level event
  if (D12 > 0 && high_level_flag == false) {
    Blynk.logEvent("high_level");
    BLYNK_LOG1("HIGH LEVEL ALARM ON");
    high_level_flag = true;
  }
  else if (D12 == 0 && high_level_flag == true) {
    high_level_flag = false;
    BLYNK_LOG1("HIGH LEVEL ALARM RESET");
  }

  // High level backup 1 event
  if (D13 > 0 && high_level_bu1_flag == false) {
    Blynk.logEvent("high_level_backup_1");
    BLYNK_LOG1("HIGH LEVEL BACKUP 1 ALARM ON");
    high_level_bu1_flag = true;
  }
  else if (D13 == 0 && high_level_bu1_flag == true) {
    high_level_bu1_flag = false;
    BLYNK_LOG1("HIGH LEVEL BACKUP 1 ALARM RESET");
  }

  // High level backup 2 event
  if (D14 > 0 && high_level_bu2_flag == false) {
    Blynk.logEvent("high_level_backup_2");
    BLYNK_LOG1("HIGH LEVEL BACKUP 2 ALARM ON");
    high_level_bu2_flag = true;
  }
  else if (D14 == 0 && high_level_bu2_flag == true) {
    high_level_bu2_flag = false;
    BLYNK_LOG1("HIGH LEVEL BACKUP 2 ALARM RESET");
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
  timer.setInterval(2000L, processDI_1);
  delay(1000);
  timer.setInterval(2000L, processDI_2);
  
  // Set pin mode for DI pins
  pinMode(A0, INPUT); // Auto Mode (P1 OR P2)
  pinMode(A1, INPUT); // Pump 1 Power
  pinMode(A2, INPUT); // Pump 2 Power
  pinMode(0, INPUT);  // Low Level
  pinMode(1, INPUT);  // Pump 1 Running
  pinMode(2, INPUT);  // Pump 2 Running
  pinMode(3, INPUT);  // Middle Level
  pinMode(5, INPUT);  // Pump 1 Common Fault
  pinMode(6, INPUT);  // Pump 2 Common Fault
  pinMode(9, INPUT);  // High Level
  pinMode(10, INPUT); // High Level Backup 1
  pinMode(11, INPUT); // High Level Backup 2
  pinMode(13, INPUT); // Long Running

  // Set pin mode for DO pins
  pinMode(8, OUTPUT);  // Relay 3 for Pump 1
  pinMode(12, OUTPUT); // Relay 4 for pump 2
}

// Main function
void loop() 
{
  wifi_status = WiFi.status();
  blynk_status = Blynk.connected();
  if (!blynk_status)
  {
    // Reset relays when disconnected from Blynk server
    P1 = 0;
    digitalWrite(8, P1);
    P2 = 0;
    digitalWrite(12, P2);

    if (wifi_status != WL_CONNECTED)
    {
      // Disconnect from WiFi
      Blynk.disconnect();
      
      // Set up WiFi connection
      Blynk.connectWiFi(ssid, pass);
    }
  
    // Set up Blynk server connection
    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk.connect();
  }
  Blynk.run(); // Run Blynk
  timer.run(); // Run Blynk timer
}
