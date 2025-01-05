/* Tunnel Sump Pump Controller (611)*/
// Enable serial print for debugging
//#define BLYNK_PRINT Serial

// Include Blynk device header file
#include "ArduinoBlynkDeviceInfo.h"

// Include libraries
#include <WiFiNINA.h>
#include <BlynkSimpleWiFiNINA.h>

// Include Wi-Fi header file
#include "ArduinoWiFiInfo.h"

// Provide Wi-Fi credentials
char ssid[] = SSID;
char pass[] = PASS;

// Declare global variables for DI, DO and flags
int P1 = 0;
int P2 = 0;
int D3, D4, D5, D6, D7, D8, D9, D10, D11, D12, D13, D14, D15, D16, D17, D18;
int D19 = 0;
long RSSI = -100;
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

// Create Blynk timer object
BlynkTimer timer;

BLYNK_CONNECTED() {
  // Reset control switches and read RSSI
  Blynk.virtualWrite(V0, P1);   // Pump 1 Remote Run
  Blynk.virtualWrite(V1, P2);   // Pump 2 Remote Run
  Blynk.virtualWrite(V19, WiFi.RSSI());   // RSSI
}

// DO for Pump 1
BLYNK_WRITE(V0)
{
  P1 = param.asInt();
  if (P1 == 1) {
    digitalWrite(8, HIGH);
    Serial.println("PUMP 1 RUNNING (REMOTE)");
  }
  else {
    digitalWrite(8, LOW);
    Serial.println("PUMP 1 STOPPED (REMOTE)");
  }
}

// DO for Pump 2
BLYNK_WRITE(V1)
{
  P2 = param.asInt();
  if (P2 == 1) {
    digitalWrite(12, HIGH);
    Serial.println("PUMP 2 RUNNING (REMOTE)");
  }
  else {
    digitalWrite(12, LOW);
    Serial.println("PUMP 2 STOPPED (REMOTE)");
  }
}

// Read and write DI values (Part 1)
void processDI_1()
{
  // Read DI values
  D3 = digitalRead(A0);  // Auto Mode
  D4 = digitalRead(A1);  // Pump 1 Power
  D5 = digitalRead(A2);  // Pump 2 Power
  D7 = digitalRead(1);   // Pump 1 Running
  D8 = digitalRead(2);   // Pump 2 Running
  D10 = digitalRead(5);  // Pump 1 Common Fault
  D11 = digitalRead(6);  // Pump 2 Common Fault
  D16 = 1 - D7; // Pump 1 Stopped (Opposite of Running)
  D17 = 1 - D8; // Pump 2 Stopped (Opposite of Running)
  D18 = 1 - D3; // Manual Mode (Opposite of Auto Mode)

  // Write DI values to datastreams
  Blynk.beginGroup();
  Blynk.virtualWrite(V16, D3);  // Auto Mode
  Blynk.virtualWrite(V2, D4);   // Pump 1 Power
  Blynk.virtualWrite(V3, D5);   // Pump 2 Power
  Blynk.virtualWrite(V5, D7);   // Pump 1 Running
  Blynk.virtualWrite(V14, D16); // Pump 1 Stopped
  Blynk.virtualWrite(V6, D8);   // Pump 2 Running
  Blynk.virtualWrite(V15, D17); // Pump 2 Stopped
  Blynk.virtualWrite(V8, D10);  // Pump 1 Common Fault
  Blynk.virtualWrite(V9, D11);  // Pump 2 Common Fault
  Blynk.endGroup();

  // Auto mode event
  if (D3 > 0 && auto_mode_flag == false) {
    Blynk.logEvent("auto_mode");
    Serial.println("AUTO MODE SELECTED");
    auto_mode_flag = true;
  }
  else if (D3 == 0 && auto_mode_flag == true) {
    auto_mode_flag = false;
  }

  // Manual mode event
  if (D18 > 0 && manual_mode_flag == false) {
    Blynk.logEvent("manual_mode");
    Serial.println("MANUAL MODE SELECTED");
    manual_mode_flag = true;
  }
  else if (D18 == 0 && manual_mode_flag == true) {
    manual_mode_flag = false;
  }

  // Pump 1 running and stopped event
  if (D7 > 0 && p1_running_flag == false) {
    Blynk.logEvent("p1_running");
    Serial.println("PUMP 1 RUNNING");
    p1_running_flag = true;
  }
  else if (D7 == 0 && p1_running_flag == true) {
    p1_running_flag = false;
    Blynk.logEvent("p1_stopped");
    Serial.println("PUMP 1 STOPPED");
  }

  // Pump 2 running and stopped event
  if (D8 > 0 && p2_running_flag == false) {
    Blynk.logEvent("p2_running");
    Serial.println("PUMP 2 RUNNING");
    p2_running_flag = true;
  }
  else if (D8 == 0 && p2_running_flag == true) {
    p2_running_flag = false;
    Blynk.logEvent("p2_stopped");
    Serial.println("PUMP 2 STOPPED");
  }

  // P1 common fault event
  if (D10 > 0 && p1_fault_flag == false) {
    Blynk.logEvent("p1_fault");
    Serial.println("PUMP 1 COMMON FAULT ALARM ON");
    p1_fault_flag = true;
  }
  else if (D10 == 0 && p1_fault_flag == true) {
    p1_fault_flag = false;
    Serial.println("PUMP 1 COMMON FAULT ALARM RESET");
  }

  // P2 common fault event
  if (D11 > 0 && p2_fault_flag == false) {
    Blynk.logEvent("p2_fault");
    Serial.println("PUMP 2 COMMON FAULT ALARM ON");
    p2_fault_flag = true;
  }
  else if (D11 == 0 && p2_fault_flag == true) {
    p2_fault_flag = false;
    Serial.println("PUMP 2 COMMON FAULT ALARM RESET");
  }
}

// Read and write DI values (Part 2)
void processDI_2()
{
  // Read DI values
  D6 = digitalRead(0);   // Low Level
  D9 = digitalRead(3);   // First Preset Level
  D12 = digitalRead(9);  // Second Preset Level
  D13 = digitalRead(10); // Pump 1 Heat Cut
  D14 = digitalRead(11); // Pump 2 Heat Cut
  D15 = digitalRead(13); // High Level

 // Determine water level index
  if (D15 == 1) {
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
  Blynk.virtualWrite(V7, D9);   // First Preset Level
  Blynk.virtualWrite(V10, D15); // High Level
  Blynk.virtualWrite(V11, D13); // Pump 1 Heat Cut
  Blynk.virtualWrite(V12, D14); // Pump 2 Heat Cut
  Blynk.virtualWrite(V13, D12); // Second Preset Level
  Blynk.virtualWrite(V18, D19); // Water Level Index
  Blynk.endGroup();

  // Low level event
  if (D6 > 0 && low_level_flag == false) {
    Blynk.logEvent("low_level");
    Serial.println("LOW LEVEL ALARM ON");
    low_level_flag = true;
  }
  else if (D6 == 0 && low_level_flag == true) {
    low_level_flag = false;
    Serial.println("LOW LEVEL ALARM RESET");
  }

  // First preset level event
  if (D9 > 0 && first_preset_level_flag == false) {
    Blynk.logEvent("first_preset_level");
    Serial.println("FIRST PRESET LEVEL ALARM ON");
    first_preset_level_flag = true;
  }
  else if (D9 == 0 && first_preset_level_flag == true) {
    first_preset_level_flag = false;
    Serial.println("FIRST PRESET LEVEL ALARM RESET");
  }

  // High level event
  if (D15 > 0 && high_level_flag == false) {
    Blynk.logEvent("high_level");
    Serial.println("HIGH LEVEL ALARM ON");
    high_level_flag = true;
  }
  else if (D15 == 0 && high_level_flag == true) {
    high_level_flag = false;
    Serial.println("HIGH LEVEL ALARM RESET");
  }

  // P1 heat cut event
  if (D13 > 0 && p1_heat_cut_flag == false) {
    Blynk.logEvent("p1_heat_cut");
    Serial.println("PUMP 1 HEAT CUT ALARM ON");
    p1_heat_cut_flag = true;
  }
  else if (D13 == 0 && p1_heat_cut_flag == true) {
    p1_heat_cut_flag = false;
    Serial.println("PUMP 1 HEAT CUT ALARM RESET");
  }

  // P2 heat cut event
  if (D14 > 0 && p2_heat_cut_flag == false) {
    Blynk.logEvent("p2_heat_cut");
    Serial.println("PUMP 2 HEAT CUT ALARM ON");
    p2_heat_cut_flag = true;
  }
  else if (D14 == 0 && p2_heat_cut_flag == true) {
    p2_heat_cut_flag = false;
    Serial.println("PUMP 2 HEAT CUT ALARM RESET");
  }

  // Second preset level event
  if (D12 > 0 && second_preset_level_flag == false) {
    Blynk.logEvent("second_preset_level");
    Serial.println("SECOND PRESET LEVEL ALARM ON");
    second_preset_level_flag = true;
  }
  else if (D12 == 0 && second_preset_level_flag == true) {
    second_preset_level_flag = false;
    Serial.println("SECOND PRESET LEVEL ALARM RESET");
  }
}

// Function for checking connection status
void checkConnectionStatus() {
  if (WiFi.status() != WL_CONNECTED) {
    // Reset relays when disconnected from WiFi
    P1 = 0;
    digitalWrite(8, P1);
    P2 = 0;
    digitalWrite(12, P2);
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.begin(ssid, pass);
    //Blynk.connectWiFi(ssid, pass);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected to WiFi. ");
    RSSI = WiFi.RSSI();
    Serial.print("(RSSI: ");
    Serial.print(RSSI);
    Serial.println(" dBm)");
    Blynk.virtualWrite(V19, RSSI);   // RSSI
    if (Blynk.connected()) {
      Serial.println("Connected to Blynk server.");
    }
    else {
      Serial.println("Reconnecting to Blynk server...");
    }
  }
}

// Setup function
void setup() 
{
  // Set up debug console
  Serial.begin(9600);
  Serial.println("Setting up device...");

  // Set up WiFi connection
  //WiFi.begin(ssid, pass);
  Blynk.connectWiFi(ssid, pass);

  // Set up Blynk server connection
  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();

  // Set up Blynk timer object
  timer.setInterval(2000L, processDI_1);
  delay(1000);
  timer.setInterval(2000L, processDI_2);
  delay(500);
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
  pinMode(12, OUTPUT); // Relay 4 for pump 2
}

// Main function
void loop()
{
  Blynk.run(); // Run Blynk
  timer.run(); // Run Blynk timer
}
