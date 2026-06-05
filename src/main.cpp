#include <Arduino.h>
#include "Relay.h"
#include "Sonar.h"

#define ALARM_RELAY_PIN 4
#define SONAR_TRIG_PIN 19
#define SONAR_ECHO_PIN 18
const char *ALARM_RELAY_NAME = "ALARM";

Relay Alarm(ALARM_RELAY_PIN, ALARM_RELAY_NAME, true);
Sonar FloodSensor(SONAR_TRIG_PIN, SONAR_ECHO_PIN);

// Macro for receiver phone number
#define RECEIVER_NUMBER "+639632146348"

// Water level thresholds in CM
#define ZERO_WATER_LEVEL_CM 50.0
#define FOOT_LEVEL_CM 30.0
#define KNEE_LEVEL_CM 45.0
#define LEG_LEVEL_CM 60.0
#define WAIST_LEVEL_CM 100.0
#define SHOULDER_LEVEL_CM 150.0

// Water Level Enum
enum WaterLevel
{
  NO_WATER,
  FOOT_LEVEL,
  KNEE_LEVEL,
  LEG_LEVEL,
  WAIST_LEVEL,
  SHOULDER_LEVEL,
  DANGER_LEVEL
};

// Structure to hold flood data
struct FloodData
{
  float measuredDistanceCm;
  float waterLevelCm;
  WaterLevel level;
  String location;
};

// Variables
float zeroWaterLevelDistanceCm = ZERO_WATER_LEVEL_CM;
float measuredDistanceCm = 0.0;
float waterLevelCm = 0.0;

// ========== BUFFER FUNCTIONS ==========
void clearBuffer()
{
  unsigned long start = millis();
  while (millis() - start < 500)
  {
    while (Serial2.available())
    {
      Serial2.read();
    }
  }
}

// ========== WATER LEVEL FUNCTIONS ==========
const char *getWaterLevelString(WaterLevel level)
{
  switch (level)
  {
  case NO_WATER:
    return "NO WATER";
  case FOOT_LEVEL:
    return "FOOT";
  case KNEE_LEVEL:
    return "KNEE";
  case LEG_LEVEL:
    return "LEG";
  case WAIST_LEVEL:
    return "WAIST";
  case SHOULDER_LEVEL:
    return "SHOULDER";
  case DANGER_LEVEL:
    return "DANGER";
  default:
    return "UNKNOWN";
  }
}

WaterLevel calculateWaterLevel(float distance)
{
  waterLevelCm = zeroWaterLevelDistanceCm - distance;
  if (waterLevelCm <= 0)
    return NO_WATER;
  if (waterLevelCm <= FOOT_LEVEL_CM)
    return FOOT_LEVEL;
  if (waterLevelCm <= KNEE_LEVEL_CM)
    return KNEE_LEVEL;
  if (waterLevelCm <= LEG_LEVEL_CM)
    return LEG_LEVEL;
  if (waterLevelCm <= WAIST_LEVEL_CM)
    return WAIST_LEVEL;
  if (waterLevelCm <= SHOULDER_LEVEL_CM)
    return SHOULDER_LEVEL;
  return DANGER_LEVEL;
}

// ========== SMS MESSAGE CREATION FUNCTIONS ==========
String createSMSMessage(WaterLevel level, float waterHeight)
{
  String message = "AGOS HERO: ";
  message += getWaterLevelString(level);
  message += " LEVEL (";
  message += String(waterHeight, 1);
  message += "cm) FLOOD IS DETECTED AT STATION";
  return message;
}

String createSMSMessageWithLocation(WaterLevel level, float waterHeight, String location)
{
  String message = "AGOS HERO: ";
  message += getWaterLevelString(level);
  message += " LEVEL (";
  message += String(waterHeight, 1);
  message += "cm) FLOOD IS DETECTED AT ";
  message += location;
  return message;
}

String createSMSMessageCustom(String prefix, WaterLevel level, float waterHeight, String suffix)
{
  String message = prefix;
  message += getWaterLevelString(level);
  message += " (";
  message += String(waterHeight, 1);
  message += "cm) ";
  message += suffix;
  return message;
}

// ========== SMS SENDING FUNCTIONS ==========
void initSMS()
{
  // Disable command echo
  Serial2.println("ATE0");
  delay(500);
  clearBuffer();

  // Set SMS text mode
  Serial2.println("AT+CMGF=1");
  delay(500);
  clearBuffer();
}

void sendSMS(String phoneNumber, String message)
{
  Serial.println("\n📱 Sending SMS...");
  Serial.print("To: ");
  Serial.println(phoneNumber);
  Serial.print("Msg: ");
  Serial.println(message);

  // Send command with phone number
  Serial2.print("AT+CMGS=\"");
  Serial2.print(phoneNumber);
  Serial2.println("\"");
  delay(1000);

  // Clear the '>' prompt and any echoes
  clearBuffer();

  // Send ONLY the message
  Serial2.print(message);
  delay(500);

  // Send Ctrl+Z (0x1A) to transmit
  Serial2.write(0x1A);
  delay(5000);

  Serial.println("✅ SMS sent!");
}

void saveAndSendSms(String phoneNumber, String message)
{
  Serial.println("\n💾 Saving and Sending SMS...");
  Serial.print("To: ");
  Serial.println(phoneNumber);
  Serial.print("Msg: ");
  Serial.println(message);

  // Step 1: Save message to SIM storage
  Serial2.print("AT+CMGW=\"");
  Serial2.print(phoneNumber);
  Serial2.println("\"");
  delay(1000);

  // Clear the '>' prompt and any echoes
  clearBuffer();

  // Write the message
  Serial2.print(message);
  delay(500);

  // Send Ctrl+Z to save
  Serial2.write(0x1A);
  delay(3000);

  // Read response to get message index
  String response = "";
  while (Serial2.available())
  {
    char c = Serial2.read();
    response += c;
    Serial.print(c);
  }

  // Extract index from +CMGW: <index>
  int msgIndex = -1;
  int startIdx = response.indexOf("+CMGW: ");
  if (startIdx != -1)
  {
    msgIndex = response.substring(startIdx + 7).toInt();
    Serial.print("\n📝 Message saved at index: ");
    Serial.println(msgIndex);
  }
  else
  {
    Serial.println("\n❌ Failed to save message");
    return;
  }

  // Step 2: Send the saved message
  delay(500);
  Serial2.print("AT+CMSS=");
  Serial2.println(msgIndex);
  delay(5000);

  // Read final response
  while (Serial2.available())
  {
    Serial.write(Serial2.read());
  }

  Serial.println("✅ SMS saved and sent!");
}

void sendSMSWithDeliveryReport(String phoneNumber, String message)
{
  Serial.println("\n📱 Sending SMS with delivery report...");
  Serial.print("To: ");
  Serial.println(phoneNumber);
  Serial.print("Msg: ");
  Serial.println(message);

  // Enable delivery report
  Serial2.println("AT+CNMI=2,1,0,1,0");
  delay(500);
  clearBuffer();

  // Send command with phone number
  Serial2.print("AT+CMGS=\"");
  Serial2.print(phoneNumber);
  Serial2.println("\"");
  delay(1000);

  clearBuffer();

  // Send message
  Serial2.print(message);
  delay(500);
  Serial2.write(0x1A);
  delay(5000);

  Serial.println("✅ SMS sent!");
}

// ========== FLOOD ALERT FUNCTIONS ==========
void sendFloodAlert(String phoneNumber, float measuredDistance, String location)
{
  // Calculate water level
  WaterLevel level = calculateWaterLevel(measuredDistance);
  float waterHeight = zeroWaterLevelDistanceCm - measuredDistance;

  // Create message
  String message = createSMSMessageWithLocation(level, waterHeight, location);

  // Send alert
  sendSMS(phoneNumber, message);

  // Print to serial
  Serial.println("🚨 FLOOD ALERT SENT!");
  Serial.print("   Level: ");
  Serial.println(getWaterLevelString(level));
  Serial.print("   Height: ");
  Serial.print(waterHeight);
  Serial.println(" cm");
}

void sendFloodAlertBatch(String phoneNumber, FloodData floodData[], int count)
{
  String combinedMessage = "AGOS HERO ALERTS: ";

  for (int i = 0; i < count; i++)
  {
    combinedMessage += getWaterLevelString(floodData[i].level);
    combinedMessage += "(";
    combinedMessage += String(floodData[i].waterLevelCm, 1);
    combinedMessage += "cm)@" + floodData[i].location;
    if (i < count - 1)
      combinedMessage += ", ";
  }

  // Truncate if too long (SMS max 160 chars)
  if (combinedMessage.length() > 160)
  {
    combinedMessage = combinedMessage.substring(0, 157) + "...";
  }

  sendSMS(phoneNumber, combinedMessage);
}

// ========== TEST FUNCTIONS ==========
void testFloodAlert(float distance, String levelName)
{
  Serial.println("\n--- Test: " + levelName + " level flood ---");
  measuredDistanceCm = distance;
  waterLevelCm = zeroWaterLevelDistanceCm - measuredDistanceCm;
  WaterLevel currentLevel = calculateWaterLevel(measuredDistanceCm);

  Serial.print("Water Level: ");
  Serial.print(waterLevelCm);
  Serial.println(" cm");
  Serial.print("Alert: ");
  Serial.println(getWaterLevelString(currentLevel));

  String smsMessage = createSMSMessage(currentLevel, waterLevelCm);
  sendSMS(RECEIVER_NUMBER, smsMessage);
}

void runAllTests()
{
  testFloodAlert(35.0, "FOOT");
  delay(5000);
  testFloodAlert(25.0, "LEG");
  delay(5000);
  testFloodAlert(10.0, "KNEE");
  delay(5000);
  testFloodAlert(-10.0, "WAIST");
  delay(5000);
  testFloodAlert(-30.0, "SHOULDER");
}

// ========== SETUP ==========
void setup()
{
  Serial.begin(115200);
  Serial2.begin(115200);
  delay(1000);
  Alarm.begin();
  // FloodSensor.begin();

  pinMode(SONAR_TRIG_PIN, OUTPUT);
  pinMode(SONAR_ECHO_PIN, INPUT);

  Serial.println("AGOS HERO - Flood Monitoring System");
  Serial.println("===================================");

  // Initialize SMS module
  initSMS();

  // Test 1: FOOT level
  // testFloodAlert(35.0, "FOOT");

  // delay(5000);

  // // Test 2: KNEE level
  // testFloodAlert(10.0, "KNEE");

  // // Example with location
  // Serial.println("\n--- Example with Location ---");
  // float sampleDistance = 35.0;
  // WaterLevel sampleLevel = calculateWaterLevel(sampleDistance);
  // float sampleHeight = zeroWaterLevelDistanceCm - sampleDistance;
  // String locationMessage = createSMSMessageWithLocation(sampleLevel, sampleHeight, "BARANGAY HALL");
  // sendSMS(RECEIVER_NUMBER, locationMessage);

  // // Example batch alert
  // Serial.println("\n--- Example Batch Alert ---");
  // FloodData alerts[2];
  // alerts[0] = {35.0, 15.0, FOOT_LEVEL, "SITE A"};
  // alerts[1] = {10.0, 40.0, KNEE_LEVEL, "SITE B"};
  // sendFloodAlertBatch(RECEIVER_NUMBER, alerts, 2);

  // Serial.println("\n===================================");
  // Serial.println("Ready for commands (echo disabled)");

  saveAndSendSms(RECEIVER_NUMBER, "hello");
}

// ========== LOOP ==========
void loop()
{
  return;
  // USB Serial → Serial2 (send to SIM module)
  if (Serial.available() > 0)
  {
    char c = Serial.read();
    Serial2.print(c);
    Serial.print(c);
  }

  // Serial2 → USB Serial (receive from SIM module)
  if (Serial2.available() > 0)
  {
    char c = Serial2.read();
    Serial.print(c);
  }

  digitalWrite(SONAR_TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(SONAR_TRIG_PIN, HIGH); // turn on the Trigger to generate pulse
  delayMicroseconds(20);              // keep the trigger "ON" for 10 ms to generate pulse
  digitalWrite(SONAR_TRIG_PIN, LOW);  // Turn off the pulse trigger to stop pulse

  // If pulse reached the receiver SONAR_ECHO_PIN
  // become high Then pulseIn() returns the
  // time taken by the pulse to reach the receiver
  long duration = pulseIn(SONAR_ECHO_PIN, HIGH);
  int distance = (duration / 2) * 0.0342;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  delay(1000);

  // Serial.println("RELAY ON");
  // Alarm.on();
  // delay(3000);
  // Serial.println("RELAY OFF");
  // Alarm.off();
  // delay(3000);
}