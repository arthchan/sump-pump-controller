/* Tunnel Sump Pump Controller (7609)*/
// Enable serial print for debugging
#define BLYNK_PRINT Serial

// Include Blynk device header file
#include "ArduinoBlynkDeviceInfo.h"

// Include libraries
#include <SPI.h>
#include <WiFiNINA.h>
#include <BlynkSimpleWiFiNINA.h>

// Include Wi-Fi header file
#include "ArduinoWiFiInfo.h"

// Provide Wi-Fi credentials
char ssid[] = SSID;
char pass[] = PASS;

// Declare global variables for DI, DO and flags
int P1, P2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12, D13, D14, D15, D16, D17, D18;
int D19 = 0;
bool high_level_flag = false;
bool middle_level_flag = false;
bool low_level_flag = false;
bool long_running_flag = false;
bool p1_fault_flag = false;
bool p2_fault_flag = false;
bool high_level_bu1_flag = false;
bool high_level_bu2_flag = false;
bool p1_running_flag = false;
bool p2_running_flag = false;

// Create Blynk timer object
BlynkTimer timer;

// Write DI value to datastream
void myTimer() 
{
  // Read DI values
  D3 = digitalRead(A0);  // Pump 1 Power
  Blynk.virtualWrite(V2, D3);

  D4 = digitalRead(A1);  // Pump 2 Power
  Blynk.virtualWrite(V3, D4);

  D5 = digitalRead(0);   // Low Level
  Blynk.virtualWrite(V4, D5);

  D6 = digitalRead(1);   // Pump 1 Running
  Blynk.virtualWrite(V5, D6);

  D7 = digitalRead(2);   // Pump 2 Running
  Blynk.virtualWrite(V6, D7);

  D8 = digitalRead(3);   // Middle Level
  Blynk.virtualWrite(V7, D8);

  D9 = digitalRead(5);   // Pump 1 Common Fault
  Blynk.virtualWrite(V8, D9);

  D10 = digitalRead(6);  // Pump 2 Common Fault
  Blynk.virtualWrite(V9, D10);

  D11 = digitalRead(9);  // High Level
  Blynk.virtualWrite(V10, D11);

  D12 = digitalRead(10); // High Level Backup 1
  Blynk.virtualWrite(V11, D12);

  D13 = digitalRead(11); // High Level Backup 2
  Blynk.virtualWrite(V12, D13);

  D14 = digitalRead(13); // Long Running
  Blynk.virtualWrite(V13, D14);

  D15 = 1 - D6; // Pump 1 Stopped (Opposite of Running)
  Blynk.virtualWrite(V14, D15);

  D16 = 1 - D7; // Pump 2 Stopped (Opposite of Running)
  Blynk.virtualWrite(V15, D16);

  D17 = 1 - D3; // Pump 1 Power Loss (Opposite of Pump 1 Power)
  Blynk.virtualWrite(V16, D17);

  D18 = 1 - D4; // Pump 2 Power Loss (Opposite of Pump 2 Power)
  Blynk.virtualWrite(V17, D18);

  // High level event
  if (D11 > 0 && high_level_flag == false) {
    Blynk.logEvent("high_level");
    Serial.println("HIGH LEVEL ALARM ON");
    high_level_flag = true;
  }
  else if (D11 == 0 && high_level_flag == true) {
    high_level_flag = false;
    Serial.println("HIGH LEVEL ALARM RESET");
  }

  // Middle level event
  if (D8 > 0 && middle_level_flag == false) {
    Blynk.logEvent("middle_level");
    Serial.println("MIDDLE LEVEL ALARM ON");
    middle_level_flag = true;
  }
  else if (D8 == 0 && middle_level_flag == true) {
    middle_level_flag = false;
    Serial.println("MIDDLE LEVEL ALARM RESET");
  }

  // Low level event
  if (D5 > 0 && low_level_flag == false) {
    Blynk.logEvent("low_level");
    Serial.println("LOW LEVEL ALARM ON");
    low_level_flag = true;
  }
  else if (D5 == 0 && low_level_flag == true) {
    low_level_flag = false;
    Serial.println("LOW LEVEL ALARM RESET");
  }

  // Long running event
  if (D14 > 0 && long_running_flag == false) {
    Blynk.logEvent("long_running");
    Serial.println("LONG RUNNING ALARM ON");
    long_running_flag = true;
  }
  else if (D14 == 0 && long_running_flag == true) {
    long_running_flag = false;
    Serial.println("LONG RUNNING ALARM RESET");
  }

  // P1 common fault event
  if (D9 > 0 && p1_fault_flag == false) {
    Blynk.logEvent("p1_fault");
    Serial.println("PUMP 1 COMMON FAULT ALARM ON");
    p1_fault_flag = true;
  }
  else if (D9 == 0 && p1_fault_flag == true) {
    p1_fault_flag = false;
    Serial.println("PUMP 1 COMMON FAULT ALARM RESET");
  }

  // P2 common fault event
  if (D10 > 0 && p2_fault_flag == false) {
    Blynk.logEvent("p2_fault");
    Serial.println("PUMP 2 COMMON FAULT ALARM ON");
    p2_fault_flag = true;
  }
  else if (D10 == 0 && p2_fault_flag == true) {
    p2_fault_flag = false;
    Serial.println("PUMP 2 COMMON FAULT ALARM RESET");
  }

  // High level backup 1 event
  if (D12 > 0 && high_level_bu1_flag == false) {
    Blynk.logEvent("high_level_backup_1");
    Serial.println("HIGH LEVEL BACKUP 1 ALARM ON");
    high_level_bu1_flag = true;
  }
  else if (D12 == 0 && high_level_bu1_flag == true) {
    high_level_bu1_flag = false;
    Serial.println("HIGH LEVEL BACKUP 1 ALARM RESET");
  }

  // High level backup 2 event
  if (D13 > 0 && high_level_bu2_flag == false) {
    Blynk.logEvent("high_level_backup_2");
    Serial.println("HIGH LEVEL BACKUP 2 ALARM ON");
    high_level_bu2_flag = true;
  }
  else if (D13 == 0 && high_level_bu2_flag == true) {
    high_level_bu2_flag = false;
    Serial.println("HIGH LEVEL BACKUP 2 ALARM RESET");
  }

  // Pump 1 running and stopped event
  if (D6 > 0 && p1_running_flag == false) {
    Blynk.logEvent("p1_running");
    Serial.println("PUMP 1 RUNNING");
    p1_running_flag = true;
  }
  else if (D6 == 0 && p1_running_flag == true) {
    p1_running_flag = false;
    Blynk.logEvent("p1_stopped");
    Serial.println("PUMP 1 STOPPED");
  }

  // Pump 2 running and stopped event
  if (D7 > 0 && p2_running_flag == false) {
    Blynk.logEvent("p2_running");
    Serial.println("PUMP 2 RUNNING");
    p2_running_flag = true;
  }
  else if (D7 == 0 && p2_running_flag == true) {
    p2_running_flag = false;
    Blynk.logEvent("p2_stopped");
    Serial.println("PUMP 2 STOPPED");
  }

  // Water level indication
  if (D13 == 1) {
    D19 = 7;
  }
  else if (D12 == 1) {
    D19 = 6;
  }
  else if (D11 == 1) {
    D19 = 5;
  }
  else if (D8 == 1) {
    D19 = 4;
  }
  else if (D5 == 1) {
    D19 = 2;
  }
  else {
    D19 = 3;
  }
  Blynk.virtualWrite(V18, D19);
}

// Function for checking connection status
void connectionstatus() {
  Serial.println("Checking connection status...");
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Reconnecting to WiFi...");
    //Blynk.connectWiFi(ssid, pass);
    WiFi.begin(ssid, pass);
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to WiFi.");
    }
  } 
  else {
    Serial.println("Connected to WiFi.");
  }
}

// Setup function
void setup() 
{
  // Set up debug console
  Serial.begin(9600);

  // Set up WiFi connection
  Blynk.connectWiFi(ssid, pass);
  //WiFi.begin(ssid, pass);

  // Set up Blynk server connection
  Blynk.config(BLYNK_AUTH_TOKEN);
  //Blynk.connect();

  // Set up Blynk timer object
  timer.setInterval(1L, myTimer); 
  timer.setInterval(0.5*60*1000, connectionstatus);
  
  // Set pin mode for DI pins
  pinMode(A0, INPUT); // Pump 1 Power
  pinMode(A1, INPUT); // Pump 2 Power
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

// DO for Pump 1
BLYNK_WRITE(V0)
{
  if (param.asInt() == 1) {
    digitalWrite(8, HIGH);
  }
  else {
    digitalWrite(8, LOW);
  }
  P1 = param.asInt();
}

// DO for Pump 2
BLYNK_WRITE(V1)
{
  if (param.asInt() == 1) {
    digitalWrite(12, HIGH);
  }
  else {
    digitalWrite(12, LOW);
  }
  P2 = param.asInt();
}

// Main function
void loop()
{ 
  Blynk.run(); // Run Blynk
  timer.run(); // Run Blynk timer
}
