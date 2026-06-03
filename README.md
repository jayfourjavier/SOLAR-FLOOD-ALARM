# A7670E GSM Module with ESP32 DevKit

This guide shows how to connect and use the A7670E GSM/LTE module with an ESP32 DevKit.

## Hardware Connections

Power Connections

A7670E| ESP32
VCC| 5V
GND| GND
VDD| 3.3V

UART Connections

A7670E| ESP32
TXD| RX2 (GPIO16)
RXD| TX2 (GPIO17)

«Note:

- Connect module TXD to ESP32 RX2.
- Connect module RXD to ESP32 TX2.
- VDD must be connected to 3.3V.
- Use a stable 5V power supply capable of providing at least 2A peak current.»

ESP32 Example Code
```cpp
#define MODEM_RX 16
#define MODEM_TX 17

HardwareSerial modem(2);

void setup() {
  Serial.begin(115200);

  modem.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

  Serial.println("A7670E Ready");
}

void loop() {
  while (modem.available()) {
    Serial.write(modem.read());
  }

  while (Serial.available()) {
    modem.write(Serial.read());
  }
}

```

## Serial Monitor Settings

Baud Rate: 115200
Line Ending: Both NL & CR

Basic AT Commands

Check Communication

AT

Expected Response:

OK

Module Information

ATI

SIM Card Status

AT+CPIN?

Expected:

+CPIN: READY

Signal Quality

AT+CSQ

Example:

+CSQ: 20,99

Signal Guide:

CSQ| Signal
0-9| Poor
10-14| Fair
15-19| Good
20-31| Excellent

Network Registration

AT+CREG?

Expected:

+CREG: 0,1

or

+CREG: 0,5

Operator Information

AT+COPS?

Module Functionality

AT+CFUN?

Enable Full Functionality:

AT+CFUN=1

SMS Commands

Set SMS Text Mode

AT+CMGF=1

Send SMS

AT+CMGS="+639123456789"

After the prompt:

Hello from ESP32

Press:

Ctrl+Z

Expected:

+CMGS: xx
OK

Read All SMS

AT+CMGL="ALL"

Delete All SMS

AT+CMGD=1,4

Mobile Data Commands

Attach to Packet Service

AT+CGATT=1

Check Attachment Status

AT+CGATT?

Activate PDP Context

AT+CGACT=1,1

Get IP Address

AT+CGPADDR=1

HTTP Commands

Initialize HTTP Service

AT+HTTPINIT

Set URL

AT+HTTPPARA="URL","https://example.com"

HTTP GET

AT+HTTPACTION=0

Read Response:

AT+HTTPREAD

Terminate HTTP

AT+HTTPTERM

GPS Commands (if supported by firmware)

Enable GPS:

AT+CGNSSPWR=1

Get Location:

AT+CGNSSINFO

Disable GPS:

AT+CGNSSPWR=0

Troubleshooting

No Response to AT

- Verify TX/RX wiring.
- Check baud rate (115200).
- Ensure common GND.
- Verify stable 5V supply.

SIM Not Detected

AT+CPIN?

Should return:

+CPIN: READY

Not Registered to Network

Check:

AT+CSQ
AT+CREG?

Move antenna to an area with better signal.

Tested Configuration

- ESP32 DevKit V1
- A7670E LTE Module
- UART2 (GPIO16/GPIO17)
- 115200 baud
- VCC = 5V
- VDD = 3.3V
- Common Ground