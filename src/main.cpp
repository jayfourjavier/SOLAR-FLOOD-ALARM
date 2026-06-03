#include <Arduino.h>

// Macro for receiver phone number
#define RECEIVER_NUMBER "+639632146348" // Replace with your target number

// Water level thresholds in CM
#define ZERO_WATER_LEVEL_CM 50.0 // Distance when water starts (calibrate this)
#define FOOT_LEVEL_CM 30.0       // Up to 30cm = Foot level
#define KNEE_LEVEL_CM 45.0       // 31-45cm = Knee level
#define LEG_LEVEL_CM 60.0        // 46-60cm = Leg level
#define WAIST_LEVEL_CM 100.0     // 61-100cm = Waist level
#define SHOULDER_LEVEL_CM 150.0  // 101-150cm = Shoulder level

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

// Variables for water level detection
float zeroWaterLevelDistanceCm = ZERO_WATER_LEVEL_CM; // Calibration point
float measuredDistanceCm = 0.0;                       // Distance from sensor to water
float waterLevelCm = 0.0;                             // Calculated water height

// Function to get water level string from enum
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

// Function to determine water level based on measured distance
WaterLevel calculateWaterLevel(float distance)
{
  waterLevelCm = zeroWaterLevelDistanceCm - distance;

  if (waterLevelCm <= 0)
  {
    return NO_WATER;
  }
  else if (waterLevelCm <= FOOT_LEVEL_CM)
  {
    return FOOT_LEVEL;
  }
  else if (waterLevelCm <= KNEE_LEVEL_CM)
  {
    return KNEE_LEVEL;
  }
  else if (waterLevelCm <= LEG_LEVEL_CM)
  {
    return LEG_LEVEL;
  }
  else if (waterLevelCm <= WAIST_LEVEL_CM)
  {
    return WAIST_LEVEL;
  }
  else if (waterLevelCm <= SHOULDER_LEVEL_CM)
  {
    return SHOULDER_LEVEL;
  }
  else
  {
    return DANGER_LEVEL;
  }
}

// Function to create SMS message with actual values
String createSMSMessage(float measuredDistance, float waterHeight, WaterLevel level)
{
  String message = "AGOS HERO: ";
  message += String(getWaterLevelString(level));
  message += " LEVEL (";
  message += String(waterHeight, 1);
  message += "cm) FLOOD IS DETECTED AT STATION";
  return message;
}

void sendSMS(String phoneNumber, String message)
{
  Serial.println("\n📱 Sending SMS...");
  Serial.print("To: ");
  Serial.println(phoneNumber);
  Serial.print("Msg: ");
  Serial.println(message);

  // Set SMS text mode
  Serial2.println("AT+CMGF=1");
  delay(1000);
  // Clear any response
  while (Serial2.available())
    Serial2.read();

  // Send command with phone number
  Serial2.print("AT+CMGS=\"");
  Serial2.print(phoneNumber);
  Serial2.println("\"");
  delay(1000);

  // Clear the '>' prompt from buffer
  while (Serial2.available())
    Serial2.read();

  // Send ONLY the message (no AT commands)
  Serial2.println(message); // Use println to add newline
  delay(500);

  // Send Ctrl+Z (0x1A) to transmit
  Serial2.write(0x1A);
  delay(5000);

  Serial.println("✅ SMS sent!");
}

void setup()
{
  Serial.begin(115200);
  Serial2.begin(115200);
  delay(1000);

  Serial.println("AGOS HERO - Flood Monitoring System");
  Serial.println("===================================");

  // Simulate water level detection (replace with actual sensor reading)
  Serial.println("\n--- Simulating Water Level Detection ---");

  // Example 1: Simulate FOOT level
  Serial.println("\nTest 1: Simulating FOOT level flood");
  measuredDistanceCm = 35.0; // Distance to water surface (cm)
  waterLevelCm = zeroWaterLevelDistanceCm - measuredDistanceCm;
  WaterLevel currentLevel = calculateWaterLevel(measuredDistanceCm);

  Serial.print("Measured Distance: ");
  Serial.print(measuredDistanceCm);
  Serial.println(" cm");
  Serial.print("Calculated Water Level: ");
  Serial.print(waterLevelCm);
  Serial.println(" cm");
  Serial.print("Alert Level: ");
  Serial.println(getWaterLevelString(currentLevel));

  // Create and send SMS
  String smsMessage = createSMSMessage(measuredDistanceCm, waterLevelCm, currentLevel);
  delay(2000);
  sendSMS(RECEIVER_NUMBER, smsMessage);

  delay(3000);

  // Example 2: Simulate WAIST level
  Serial.println("\n--- Test 2: Simulating WAIST level flood ---");
  measuredDistanceCm = 10.0; // Distance to water surface (cm)
  waterLevelCm = zeroWaterLevelDistanceCm - measuredDistanceCm;
  currentLevel = calculateWaterLevel(measuredDistanceCm);

  Serial.print("Measured Distance: ");
  Serial.print(measuredDistanceCm);
  Serial.println(" cm");
  Serial.print("Calculated Water Level: ");
  Serial.print(waterLevelCm);
  Serial.println(" cm");
  Serial.print("Alert Level: ");
  Serial.println(getWaterLevelString(currentLevel));

  // Create and send second SMS
  smsMessage = createSMSMessage(measuredDistanceCm, waterLevelCm, currentLevel);
  delay(2000);
  sendSMS(RECEIVER_NUMBER, smsMessage);

  Serial.println("\n===================================");
  Serial.println("SIM Terminal Ready for AT commands:");
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