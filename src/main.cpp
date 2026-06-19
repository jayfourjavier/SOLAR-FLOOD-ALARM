#include <Arduino.h>
#include <EEPROM.h>
#include "Relay.h"
#include "Sonar.h"
#include "Sim7670e.h"

#define PROJECT_NAME "SOLAR FLOOD ALARM"
#define SECURITY_CODE "147963"
#define HELP_SMS_GUIDE                           \
  "SOLAR FLOOD ALERT COMMANDS:\n"                \
  "[SECURITY_CODE,SETUP_HELP,HELP]\n"            \
  "[SECURITY_CODE,SETUP_SENSOR,ZERO]\n"          \
  "[SECURITY_CODE,SETUP_NUMBER,+639XXXXXXXXX]\n" \
  "[SECURITY_CODE,GET_SENSOR_DATA,DATA]"
#define SETUP_SMS_MESSAGE "SETUP TEST MESSAGE JAYFOUR JAVIER"

#define FLASH_SIZE 128
#define FLASH_MAGIC_ADDR 0
#define FLASH_MAGIC_VALUE 0x14826301
#define FLASH_ZERO_DISTANCE_ADDR 10
#define FLASH_DEFAULT_ZERO_DISTANCE 300.00f
#define FLASH_RECEIVER_ADDR 20
#define FLASH_RECEIVER_SIZE 16
#define FLASH_DEFAULT_RECEIVER_NUMBER "+639632146348"

#define FLASH_INITIALIZED_VALUE 0x01

enum CommandType
{
  COMMAND_UNKNOWN,
  SETUP_SENSOR,
  SETUP_NUMBER,
  SETUP_HELP,
  GET_SENSOR_DATA
};

enum FloodState
{
  STATE_NORMAL,
  STATE_FLOOD_LEVEL_1,
  STATE_FLOOD_LEVEL_2,
  STATE_FLOOD_LEVEL_3,
  STATE_FLOOD_LEVEL_4
};

FloodState currentFloodState = STATE_NORMAL;

#define LEVEL1_ALARM_DURATION 10000UL // 10 seconds
#define LEVEL4_CONTINUOUS_ALARM true

// ACTUAL DURATION AND INTERVAL. NOTE: 1 MINUTE = 60000, 1 SECOND = 1000. UL MEANS UNSIGNED LONG.
// #define LEVEL1_ALARM_INTERVAL 600000UL // 10 minutes
// #define LEVEL2_ALARM_DURATION 30000UL   // 30 seconds
// #define LEVEL2_ALARM_INTERVAL 300000UL // 5 minutes
// #define LEVEL3_ALARM_DURATION 60000UL   // 1 minute
// #define LEVEL3_ALARM_INTERVAL 120000UL // 2 minutes
// #define LEVEL1_SMS_INTERVAL 600000UL // 10 minutes
// #define LEVEL2_SMS_INTERVAL 300000UL // 5 minutes
// #define LEVEL3_SMS_INTERVAL 120000UL // 2 minutes
// #define LEVEL4_SMS_INTERVAL 60000UL  // 1 minute

// TEST DURATIONS AND INTERVAL
#define LEVEL1_ALARM_DURATION 10000UL // 10 seconds
#define LEVEL1_ALARM_INTERVAL 5000UL  // 5 seconds
#define LEVEL2_ALARM_DURATION 20000UL // 20 seconds
#define LEVEL2_ALARM_INTERVAL 5000UL  // 5 seconds
#define LEVEL3_ALARM_DURATION 30000UL // 30 seconds
#define LEVEL3_ALARM_INTERVAL 5000UL  // 5 seconds

#define LEVEL1_SMS_INTERVAL 60000UL // 1 minute
#define LEVEL2_SMS_INTERVAL 60000UL // 1 minute
#define LEVEL3_SMS_INTERVAL 60000UL // 1 minute
#define LEVEL4_SMS_INTERVAL 60000UL // 1 minute

#define ALARM_RELAY_PIN 4
#define SONAR_TRIG_PIN 19
#define SONAR_ECHO_PIN 18
const char *ALARM_RELAY_NAME = "SIREN_WITH_STROBE_LIGHT";

Relay Alarm(ALARM_RELAY_PIN, ALARM_RELAY_NAME, true);
Sonar FloodSensor(SONAR_TRIG_PIN, SONAR_ECHO_PIN, 10);
Sim7670e Sim(115200);

// Macro for receiver phone number

// Variables
float zeroWaterLevelDistanceCm = 0;
float measuredDistanceCm = 0.0;
float waterLevelCm = 0.0;
String ReceiverNumber = "";

const char *getFloodStateName(FloodState state)
{
  switch (state)
  {
  case STATE_NORMAL:
    return "NORMAL";

  case STATE_FLOOD_LEVEL_1:
    return "LEVEL 1";

  case STATE_FLOOD_LEVEL_2:
    return "LEVEL 2";

  case STATE_FLOOD_LEVEL_3:
    return "LEVEL 3";

  case STATE_FLOOD_LEVEL_4:
    return "LEVEL 4";

  default:
    return "UNKNOWN";
  }
}

String createFloodAlertMessage(FloodState state)
{
  String message = PROJECT_NAME;

  message += "\n\nFLOOD ALERT";
  message += "\nLEVEL: ";
  message += getFloodStateName(state);

  message += "\nWATER HEIGHT: ";
  message += String(waterLevelCm, 1);
  message += " cm";

  if (state == STATE_FLOOD_LEVEL_4)
  {
    message += "\n\nDANGER! CRITICAL FLOOD LEVEL";
  }

  return message;
}

FloodState calculateFloodState(float waterHeight)
{
  if (waterHeight < 10.16)
  {
    return STATE_NORMAL;
  }

  else if (waterHeight <= 30.48)
  {
    return STATE_FLOOD_LEVEL_1;
  }

  else if (waterHeight <= 76.2)
  {
    return STATE_FLOOD_LEVEL_2;
  }

  else if (waterHeight <= 121.92)
  {
    return STATE_FLOOD_LEVEL_3;
  }

  else
  {
    return STATE_FLOOD_LEVEL_4;
  }
}

/**
 * @brief Checks if SMS contains a valid security code.
 *
 * Expected SMS format:
 *
 * [SECURITY_CODE,COMMAND]
 *
 * Example:
 *
 * [123456,SETUP_SENSOR,ZERO]
 *
 * @param sms SMS content string.
 *
 * @return true if the security code matches, false otherwise.
 */
bool hasSecureCommand(const String &sms)
{
  String expected = "[" + String(SECURITY_CODE) + ",";

  return sms.indexOf(expected) != -1;
}

struct SmsCommand
{
  String securityCode;
  CommandType type;
  String content;
  String senderNumber;
};

/**
 * @brief Extracts sender phone number from AT+CMGR response.
 *
 * Expected format:
 *
 * +CMGR: "REC UNREAD","+639XXXXXXXXX","","date"
 *
 * @param response Raw SIM7670E SMS read response.
 *
 * @return Sender phone number.
 *         Returns empty string if unavailable.
 */
String getSmsSenderNumber(const String &response)
{
  int firstQuote = response.indexOf("\"");

  if (firstQuote == -1)
    return "";

  int secondQuote = response.indexOf("\"", firstQuote + 1);

  if (secondQuote == -1)
    return "";

  int thirdQuote = response.indexOf("\"", secondQuote + 1);

  if (thirdQuote == -1)
    return "";

  int fourthQuote = response.indexOf("\"", thirdQuote + 1);

  if (fourthQuote == -1)
    return "";

  String number = response.substring(
      thirdQuote + 1,
      fourthQuote);

  if (number.length() > 0 && number[0] != '+')
  {
    number = "+" + number;
  }

  return number;
}

CommandType getCommandType(const String &command)
{
  if (command == "SETUP_SENSOR")
  {
    return SETUP_SENSOR;
  }
  else if (command == "SETUP_NUMBER")
  {
    return SETUP_NUMBER;
  }
  else if (command == "SETUP_HELP")
  {
    return SETUP_HELP;
  }
  else if (command == "GET_SENSOR_DATA")
  {
    return GET_SENSOR_DATA;
  }

  return COMMAND_UNKNOWN;
}

/**
 * @brief Parses SMS command format.
 *
 * Expected format:
 *
 * [SECURITY_CODE,COMMAND_TYPE,COMMAND_CONTENT]
 *
 * Example:
 *
 * [147963,SETUP_SENSOR,ZERO]
 *
 * @param sms SMS content.
 *
 * @return Parsed SmsCommand structure.
 */
SmsCommand parseSmsCommand(const String &sms, const String &sender)

{
  SmsCommand command;
  command.senderNumber = sender;

  command.type = COMMAND_UNKNOWN;

  String data = sms;

  data.replace("[", "");
  data.replace("]", "");

  int firstComma = data.indexOf(',');

  if (firstComma == -1)
    return command;

  int secondComma = data.indexOf(',', firstComma + 1);

  if (secondComma == -1)
    return command;

  command.securityCode = data.substring(
      0,
      firstComma);

  String commandType = data.substring(
      firstComma + 1,
      secondComma);

  command.type = getCommandType(commandType);

  command.content = data.substring(
      secondComma + 1);

  command.content.trim();

  return command;
}

void sendSensorData(SmsCommand _command)
{
  float measuredDistance = FloodSensor.getMedianDistance();
  float waterLevel = zeroWaterLevelDistanceCm - measuredDistance;

  float waterLevelFeet = waterLevel / 30.48;
  int feet = (int)waterLevelFeet;
  int inches = (int)((waterLevelFeet - feet) * 12);

  String message = PROJECT_NAME;

  message += "\nZERO LEVEL: ";
  message += String(zeroWaterLevelDistanceCm, 1);
  message += " cm\n";

  message += "DISTANCE: ";
  message += String(measuredDistance, 1);
  message += " cm\n";

  message += "WATER LEVEL: ";
  message += String(waterLevel, 1);
  message += " cm (";
  message += String(feet);
  message += "ft ";
  message += String(inches);
  message += "in)";

  Serial.println(message);

  if (Sim.sendSms(
          _command.senderNumber,
          message))
  {
    Serial.println("SENSOR DATA SENT");
  }
  else
  {
    Serial.println("SENSOR DATA FAILED");
  }
}

void sendStartUpSms()
{
  if (Sim.sendSms("+639632146348", SETUP_SMS_MESSAGE))
  {
    Serial.println("SMS SENT");
  }
  else
  {
    Serial.println("SMS FAILED");
  }
}

void writeZeroDistance(float distance)
{
  EEPROM.put(
      FLASH_ZERO_DISTANCE_ADDR,
      distance);

  EEPROM.commit();
}

float readZeroDistance()
{
  float distance;

  EEPROM.get(
      FLASH_ZERO_DISTANCE_ADDR,
      distance);

  return distance;
}

void writeReceiverNumber(const String &number)
{
  char buffer[FLASH_RECEIVER_SIZE];

  memset(
      buffer,
      0,
      sizeof(buffer));

  strncpy(
      buffer,
      number.c_str(),
      FLASH_RECEIVER_SIZE - 1);

  EEPROM.put(
      FLASH_RECEIVER_ADDR,
      buffer);

  EEPROM.commit();
}

String readReceiverNumber()
{
  char buffer[FLASH_RECEIVER_SIZE];

  EEPROM.get(
      FLASH_RECEIVER_ADDR,
      buffer);

  // Force termination safety
  buffer[FLASH_RECEIVER_SIZE - 1] = '\0';

  return String(buffer);
}

void loadZeroWaterLevelDistance()
{
  zeroWaterLevelDistanceCm = readZeroDistance();
}

void loadSmsReceiverNumber()
{
  ReceiverNumber = readReceiverNumber();
}

void setupFlashMemory()
{
  EEPROM.begin(FLASH_SIZE);

  uint32_t magic;

  EEPROM.get(
      FLASH_MAGIC_ADDR,
      magic);

  if (magic != FLASH_MAGIC_VALUE)
  {
    Serial.println("FLASH MEMORY INITIALIZED TO FACTORY DEFAULT");

    EEPROM.put(
        FLASH_MAGIC_ADDR,
        (uint32_t)FLASH_MAGIC_VALUE);

    // Default zero distance
    float zeroDistance = FLASH_DEFAULT_ZERO_DISTANCE;

    EEPROM.put(
        FLASH_ZERO_DISTANCE_ADDR,
        zeroDistance);

    // Default receiver number
    char receiver[FLASH_RECEIVER_SIZE];

    memset(
        receiver,
        0,
        sizeof(receiver));

    strncpy(
        receiver,
        FLASH_DEFAULT_RECEIVER_NUMBER,
        FLASH_RECEIVER_SIZE - 1);

    EEPROM.put(
        FLASH_RECEIVER_ADDR,
        receiver);

    EEPROM.commit();
  }
  else
  {
    Serial.println("LOADED CONFIGURATION DATA FROM FLASH MEMORY ");
  }
}

void sendSmsFloodAlert(FloodState state)
{
  String receiver = ReceiverNumber;

  String message = createFloodAlertMessage(state);

  Serial.println("SMS ALERT:");
  Serial.println(message);

  if (Sim.sendSms(receiver, message))
  {
    Serial.println("FLOOD SMS SENT");
  }
  else
  {
    Serial.println("FLOOD SMS FAILED");
  }
}

void processCommand(const SmsCommand &command)
{
  switch (command.type)
  {
  case SETUP_SENSOR:
  {
    Serial.println("COMMAND: SETUP_SENSOR");
    if (command.content == "ZERO")
    {
      Serial.println("SETTING ZERO DISTANCE");
      measuredDistanceCm = FloodSensor.getMedianDistance();                // Read distance from sensor
      writeZeroDistance(measuredDistanceCm);                               // Write new zero distance to flash memory
      Serial.printf("ZERO DISTANCE SAVED: %.2f cm\n", measuredDistanceCm); // Print to serial monitor
    }
    break;
  }

  case SETUP_NUMBER:
  {
    Serial.println("COMMAND: SETUP_NUMBER");
    String newReceiverNumber = command.content;
    Serial.printf("NEW NUMBER: %s\n", newReceiverNumber.c_str());
    writeReceiverNumber(newReceiverNumber);
    String confirmationMessage = String(PROJECT_NAME);
    confirmationMessage += "\nAlert Recipient Number successfully updated to:\n";
    confirmationMessage += newReceiverNumber;
    Sim.sendSms(command.senderNumber, confirmationMessage);

    break;
  }

  case SETUP_HELP:
  {
    Serial.println("COMMAND: SETUP_HELP");
    Serial.println(HELP_SMS_GUIDE);
    Sim.sendSms(command.senderNumber, HELP_SMS_GUIDE);
    break;
  }

  case GET_SENSOR_DATA:
  {
    Serial.println("COMMAND: GET_SENSOR_DATA");
    sendSensorData(command);
    break;
  }

  default:
    Serial.println("UNKNOWN COMMAND");
    break;
  }
}

// ========== WATER LEVEL FUNCTIONS ==========

void inboxController()
{
  if (Sim.hasNewSms())
  {
    Serial.println("\n============= NEW SMS RECEIVED =============");

    int index = Sim.getNewSmsIndex();
    String response = Sim.getReadSmsResponse(index);
    String content = Sim.getSmsContent(response);
    String sender = getSmsSenderNumber(response);

    Serial.printf("SENDER: %s\nCONTENT: %s\n", sender.c_str(), content.c_str());

    Serial.println("\nCHECKING SECURITY CODE...\n");

    if (hasSecureCommand(content))
    {
      Serial.println("SECURITY CODE: VALID");

      SmsCommand command = parseSmsCommand(content, sender);

      Serial.print("COMMAND TYPE ENUM: ");
      Serial.println(command.type);

      Serial.print("COMMAND CONTENT: ");
      Serial.println(command.content);
      Serial.println("");

      processCommand(command);
    }
    else
    {
      Serial.println("INCORRECT SECURITY CODE");
    }

    Serial.println("=============================================");
  }
}

void alarmController()
{
  static unsigned long previousMillis = 0;
  static bool alarmState = false;

  unsigned long currentMillis = millis();

  if (currentFloodState == STATE_NORMAL)
  {
    if (alarmState)
    {
      Alarm.off();
      alarmState = false;
    }

    return;
  }

  // LEVEL 4 = continuous alarm
  if (currentFloodState == STATE_FLOOD_LEVEL_4)
  {
    if (!alarmState)
    {
      Alarm.on();
      alarmState = true;

      Serial.println("LEVEL 4: CONTINUOUS ALARM");
    }

    return;
  }

  unsigned long onTime;
  unsigned long offTime;

  switch (currentFloodState)
  {
  case STATE_FLOOD_LEVEL_1:
  {
    onTime = LEVEL1_ALARM_DURATION;
    offTime = LEVEL1_ALARM_INTERVAL;
    break;
  }

  case STATE_FLOOD_LEVEL_2:
  {
    onTime = LEVEL2_ALARM_DURATION;
    offTime = LEVEL2_ALARM_INTERVAL;
    break;
  }

  case STATE_FLOOD_LEVEL_3:
  {
    onTime = LEVEL3_ALARM_DURATION;
    offTime = LEVEL3_ALARM_INTERVAL;
    break;
  }

  default:
    return;
  }

  unsigned long interval =
      alarmState ? onTime : offTime;

  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;

    alarmState = !alarmState;

    if (alarmState)
    {
      Alarm.on();
      Serial.println("ALARM ON");
    }
    else
    {
      Alarm.off();
      Serial.println("ALARM OFF");
    }
  }
}

void smsAlertController()
{
  static unsigned long previousMillis = 0;
  static FloodState lastSentState = STATE_NORMAL;

  unsigned long currentMillis = millis();

  unsigned long interval = 0;

  switch (currentFloodState)
  {
  case STATE_NORMAL:
  {
    if (lastSentState != STATE_NORMAL)
    {
      sendSmsFloodAlert(STATE_NORMAL);
      lastSentState = STATE_NORMAL;
    }

    return;
  }

  case STATE_FLOOD_LEVEL_1:
    interval = LEVEL1_SMS_INTERVAL;
    break;

  case STATE_FLOOD_LEVEL_2:
    interval = LEVEL2_SMS_INTERVAL;
    break;

  case STATE_FLOOD_LEVEL_3:
    interval = LEVEL3_SMS_INTERVAL;
    break;

  case STATE_FLOOD_LEVEL_4:
    interval = LEVEL4_SMS_INTERVAL;
    break;
  }

  // immediate notification when level changes
  if (currentFloodState != lastSentState)
  {
    sendSmsFloodAlert(currentFloodState);

    lastSentState = currentFloodState;
    previousMillis = currentMillis;

    return;
  }

  // periodic reminder
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;

    sendSmsFloodAlert(currentFloodState);
  }
}

// ========== SETUP ==========
void setup()
{
  Serial.begin(115200);
  Alarm.begin();
  FloodSensor.begin();

  Serial.println("\n\n=============================================");
  Serial.printf("%s - Flood Monitoring System\n", PROJECT_NAME);
  Serial.println("---------------------------------------------");

  Serial.printf("SIM MODULE\t: %s\n", Sim.begin() ? "OK" : "FAIL");                                      // INITIATE SERIAL COMMUNICATION WITH SIM MODULE. PRINT  "SIM MODULE: OK" IF SUCCESSFUL, OTHERWISE PRINT "SIM MODULE: FAIL"
  Serial.printf("MY SIM CARD\t: %s (%s)\n", Sim.getPhoneNumber().c_str(), Sim.getNetworkName().c_str()); // GET SIM CARD NUMBER AND NETWORK OPERATOR NAME
  Serial.printf("CONNECTED\t: %s \n", Sim.isConnectedToNetwork() ? "YES" : "NO");                        // CHECK IF CONNECTED TO LTE NETWORK OR NOT
  Serial.println("---------------------------------------------");

  setupFlashMemory();
  loadSmsReceiverNumber();
  loadZeroWaterLevelDistance();

  Serial.print("ZERO DISTANCE\t: ");
  Serial.println(zeroWaterLevelDistanceCm);
  Serial.print("RECEIVER\t: ");
  Serial.println(ReceiverNumber);
  // Serial.println("---------------------------------------------");

  Serial.println("=============================================");

  // sendStartUpSms();

  // Sim.deleteAllSms();
  // Serial.println(Sim.readAllSms());

  Serial.println("ON LOOP");

  currentFloodState = STATE_NORMAL;
}

// ========== LOOP ==========
void loop()
{

  Sim.update();
  inboxController();
  alarmController();
  smsAlertController();
}