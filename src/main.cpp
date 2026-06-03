#include <Arduino.h>

// Macro for receiver phone number
#define RECEIVER_NUMBER "+639632146348"

// Water level thresholds in CM
#define ZERO_WATER_LEVEL_CM 50.0
#define FOOT_LEVEL_CM 30.0
#define KNEE_LEVEL_CM 45.0
#define LEG_LEVEL_CM 60.0
#define WAIST_LEVEL_CM 100.0
#define SHOULDER_LEVEL_CM 150.0

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

// Variables
float zeroWaterLevelDistanceCm = ZERO_WATER_LEVEL_CM;
float measuredDistanceCm = 0.0;
float waterLevelCm = 0.0;

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

void sendSMS(String phoneNumber, String message)
{
  Serial.println("\n📱 Sending SMS...");
  Serial.print("To: ");
  Serial.println(phoneNumber);
  Serial.print("Msg: ");
  Serial.println(message);

  // Disable command echo
  Serial2.println("ATE0");
  delay(500);
  clearBuffer();

  // Set SMS text mode
  Serial2.println("AT+CMGF=1");
  delay(500);
  clearBuffer();

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

  // Re-enable echo for terminal use (optional)
  Serial2.println("ATE1");
  delay(500);
  clearBuffer();

  Serial.println("✅ SMS sent!");
}

void setup()
{
  Serial.begin(115200);
  Serial2.begin(115200);
  delay(1000);

  Serial.println("AGOS HERO - Flood Monitoring System");
  Serial.println("===================================");

  // Disable echo globally
  Serial2.println("ATE0");
  delay(500);
  clearBuffer();

  // Test 1: FOOT level
  Serial.println("\n--- Test 1: FOOT level flood ---");
  measuredDistanceCm = 35.0;
  waterLevelCm = zeroWaterLevelDistanceCm - measuredDistanceCm;
  WaterLevel currentLevel = calculateWaterLevel(measuredDistanceCm);

  Serial.print("Water Level: ");
  Serial.print(waterLevelCm);
  Serial.println(" cm");
  Serial.print("Alert: ");
  Serial.println(getWaterLevelString(currentLevel));

  String smsMessage = "AGOS HERO: ";
  smsMessage += getWaterLevelString(currentLevel);
  smsMessage += " LEVEL (";
  smsMessage += String(waterLevelCm, 1);
  smsMessage += "cm) FLOOD IS DETECTED AT STATION";

  delay(2000);
  sendSMS(RECEIVER_NUMBER, smsMessage);

  delay(5000);

  // Test 2: KNEE level
  Serial.println("\n--- Test 2: KNEE level flood ---");
  measuredDistanceCm = 10.0;
  waterLevelCm = zeroWaterLevelDistanceCm - measuredDistanceCm;
  currentLevel = calculateWaterLevel(measuredDistanceCm);

  Serial.print("Water Level: ");
  Serial.print(waterLevelCm);
  Serial.println(" cm");
  Serial.print("Alert: ");
  Serial.println(getWaterLevelString(currentLevel));

  smsMessage = "AGOS HERO: ";
  smsMessage += getWaterLevelString(currentLevel);
  smsMessage += " LEVEL (";
  smsMessage += String(waterLevelCm, 1);
  smsMessage += "cm) FLOOD IS DETECTED AT STATION";

  delay(2000);
  sendSMS(RECEIVER_NUMBER, smsMessage);

  Serial.println("\n===================================");
  Serial.println("Ready for commands (echo disabled)");
}

void loop()
{
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
}