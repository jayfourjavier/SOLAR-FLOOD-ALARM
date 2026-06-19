#ifndef SIM7670E_H
#define SIM7670E_H

#include <Arduino.h>

class Sim7670e : public HardwareSerial
{
private:
    uint32_t _baudrate;
    String _rxBuffer;
    int _newSmsIndex = -1;

public:
    inline Sim7670e(uint32_t baudrate = 115200)
        : HardwareSerial(2)
    {
        _baudrate = baudrate;
    }

    /**
     * @brief Sends an AT command and waits for a successful response.
     *
     * Sends an AT command to the SIM7670E module and waits until the expected
     * response string is detected or the timeout expires.
     *
     * The command can be retried multiple times if the expected response is
     * not received.
     *
     * @param command AT command string to send.
     * @param expected Expected response string (default: "OK").
     * @param timeout Maximum wait time per attempt in milliseconds (default: 5000 ms).
     * @param tries Number of attempts before returning failure (default: 3).
     *
     * @return true if the expected response is received, false otherwise.
     */
    inline bool sendCommand(
        const String &command,
        const String &expected = "OK",
        uint32_t timeout = 5000,
        uint8_t tries = 3)
    {
        for (uint8_t attempt = 0; attempt < tries; attempt++)
        {
            // Clear previous response data
            while (available())
            {
                read();
            }

            println(command);

            String response = "";

            uint32_t start = millis();

            while (millis() - start < timeout)
            {
                while (available())
                {
                    char c = read();
                    response += c;

                    if (response.indexOf(expected) != -1)
                    {
                        return true;
                    }
                }
            }

            delay(100);
        }

        return false;
    }

    /**
     * @brief Sends an AT command and returns the module response.
     *
     * Sends an AT command to the SIM7670E and captures the complete response
     * until the expected response string is received or the timeout expires.
     *
     * This method is used for commands where the returned data must be parsed,
     * such as signal strength, operator information, phone number, and SMS data.
     *
     * @param command AT command string to send.
     * @param expected Expected response string indicating command completion
     *        (default: "OK").
     * @param timeout Maximum wait time in milliseconds (default: 5000 ms).
     *
     * @return String containing the SIM7670E response.
     */
    inline String sendCommandResponse(
        const String &command,
        const String &expected = "OK",
        uint32_t timeout = 5000)
    {
        while (available())
        {
            read();
        }

        println(command);

        String response = "";

        uint32_t start = millis();

        while (millis() - start < timeout)
        {
            while (available())
            {
                response += (char)read();
            }

            if (response.indexOf(expected) != -1)
            {
                break;
            }
        }

        return response;
    }

    /**
     * @brief Initializes SIM7670E UART communication.
     *
     * Configures HardwareSerial UART2 communication with the SIM7670E module
     * using the configured baud rate and verifies communication by sending
     * an AT command.
     *
     * @return true if SIM7670E responds successfully, false otherwise.
     */
    inline bool begin()
    {
        HardwareSerial::begin(
            _baudrate,
            SERIAL_8N1,
            16,
            17);

        delay(1000);

        if (!sendCommand("AT"))
        {
            return false;
        }

        return setTextMode();
    }

    /**
     * @brief Sets SMS messaging mode to text mode.
     *
     * Configures the SIM7670E module to use SMS text mode.
     *
     * Text mode allows SMS messages to be sent and received as readable
     * text instead of PDU encoded format.
     *
     * Sends:
     *
     * AT+CMGF=1
     *
     * @param timeout Maximum wait time per attempt in milliseconds.
     * @param tries Number of attempts before failure.
     *
     * @return true if the SIM7670E accepts text mode configuration,
     *         false otherwise.
     */
    inline bool setTextMode(
        uint32_t timeout = 5000,
        uint8_t tries = 3)
    {
        return sendCommand(
            "AT+CMGF=1",
            "OK",
            timeout,
            tries);
    }

    /**
     * @brief Updates SIM7670E background communication handling.
     *
     * Reads incoming UART data from the SIM7670E and processes unsolicited
     * modem notifications.
     *
     * Currently detects incoming SMS notifications:
     *
     * +CMTI: "SM",<index>
     *
     * When a new SMS notification is detected, the SMS index is stored internally
     * and can be retrieved using getNewSmsIndex().
     *
     * This method should be called continuously inside the main loop.
     *
     * @note This method does not block waiting for modem responses.
     */
    inline void update()
    {
        while (available())
        {
            char c = read();

            Serial.write(c);

            _rxBuffer += c;
        }

        int cmti = _rxBuffer.indexOf("+CMTI:");

        if (cmti != -1)
        {
            int comma = _rxBuffer.indexOf(',', cmti);

            if (comma != -1)
            {
                _newSmsIndex = _rxBuffer.substring(comma + 1).toInt();

                Serial.print("SMS INDEX FOUND: ");
                Serial.println(_newSmsIndex);

                _rxBuffer = "";
            }
        }
    }

    /**
     * @brief Checks LTE network registration status.
     *
     * Sends AT+CEREG? command and checks if the SIM7670E is registered
     * to an LTE network.
     *
     * Registration states considered connected:
     * - +CEREG: 0,1 (registered to home network)
     * - +CEREG: 0,5 (registered while roaming)
     *
     * @return true if LTE network registration is active, false otherwise.
     */
    inline bool isConnectedToNetwork()
    {
        String response = sendCommandResponse("AT+CEREG?");

        return (
            response.indexOf("+CEREG: 0,1") != -1 ||
            response.indexOf("+CEREG: 0,5") != -1);
    }

    /**
     * @brief Retrieves the current mobile network operator.
     *
     * Sends AT+COPS? command and identifies the operator using the returned
     * PLMN code.
     *
     * Supported operators:
     * - 51503 : SMART/TNT
     * - 51502 : GLOBE/TM/GOMO
     * - 51566 : DITO
     *
     * @return String containing the detected operator name.
     *         Returns "UNKNOWN" if the operator cannot be identified.
     */
    inline String getNetworkName()
    {
        String response = sendCommandResponse("AT+COPS?");

        int start = response.indexOf("\"");

        if (start == -1)
            return "UNKNOWN";

        int end = response.indexOf("\"", start + 1);

        if (end == -1)
            return "UNKNOWN";

        String plmn = response.substring(start + 1, end);

        if (plmn == "51503")
        {
            return "SMART/TNT";
        }
        else if (plmn == "51502")
        {
            return "GLOBE/TM/GOMO";
        }
        else if (plmn == "51566")
        {
            return "DITO";
        }

        return plmn;
    }

    /**
     * @brief Retrieves the SIM card phone number.
     *
     * Sends AT+CNUM command and extracts the registered phone number from
     * the SIM card information.
     *
     * SIM7670E response format:
     *
     * +CNUM: "name","number",type
     *
     * @return String containing the phone number in international format.
     *         Returns an empty string if the SIM does not provide a number.
     */
    inline String getPhoneNumber()
    {
        String response = sendCommandResponse("AT+CNUM");

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

    /**
     * @brief Checks whether a new SMS notification is pending.
     *
     * The SIM7670E generates an unsolicited notification when a new SMS arrives:
     *
     * +CMTI: "SM",<index>
     *
     * The notification is captured by update() and stored internally.
     *
     * @return true if a new SMS is available for processing, false otherwise.
     */
    inline bool hasNewSms()
    {
        return _newSmsIndex >= 0;
    }

    /**
     * @brief Retrieves and clears the pending SMS index.
     *
     * Returns the SMS storage index detected from the last +CMTI notification.
     * After returning the value, the internal SMS notification state is cleared.
     *
     * @return SMS storage index, or -1 if no new SMS notification exists.
     */
    inline int getNewSmsIndex()
    {
        int index = _newSmsIndex;

        _newSmsIndex = -1;

        return index;
    }

    /**
     * @brief Reads a specific SMS message and returns the raw modem response.
     *
     * Sends AT+CMGR=<index> command to the SIM7670E module.
     *
     * The returned response contains SMS metadata and message content.
     *
     * Example:
     *
     * +CMGR: "REC UNREAD","+639632146348","","26/06/19,10:18:42+32"
     *
     * THIS IS THE SMS TEST MESSAGE
     *
     * OK
     *
     * @param index SMS storage index.
     * @param timeout Maximum wait time in milliseconds.
     *
     * @return Raw SIM7670E response string.
     */
    inline String getReadSmsResponse(uint16_t index, uint32_t timeout = 5000)
    {
        String command = "AT+CMGR=" + String(index);

        return sendCommandResponse(
            command,
            "OK",
            timeout);
    }

    /**
     * @brief Extracts SMS message body from a CMGR response.
     *
     * Removes:
     * - AT command echo
     * - +CMGR SMS header
     * - trailing OK response
     *
     * @param response Raw response from getReadSmsResponse().
     *
     * @return SMS message body.
     */
    inline String getSmsContent(const String &response)
    {
        int firstLineEnd = response.indexOf('\n');

        if (firstLineEnd == -1)
        {
            return "";
        }

        int headerEnd = response.indexOf('\n', firstLineEnd + 1);

        if (headerEnd == -1)
        {
            return "";
        }

        String content = response.substring(headerEnd + 1);

        content.trim();

        int okIndex = content.lastIndexOf("\nOK");

        if (okIndex != -1)
        {
            content = content.substring(0, okIndex);
            content.trim();
        }

        return content;
    }

    /**
     * @brief Reads all SMS messages stored in SIM memory.
     *
     * Sends AT+CMGL="ALL" command and returns the raw SIM7670E response.
     *
     * The response contains SMS indexes, sender numbers, timestamps,
     * and message bodies.
     *
     * @param timeout Maximum wait time in milliseconds.
     *
     * @return String containing the SMS list response.
     */
    inline String readAllSms(uint32_t timeout = 5000)
    {
        return sendCommandResponse(
            "AT+CMGL=\"ALL\"",
            "OK",
            timeout);
    }

    /**
     * @brief Deletes a specific SMS message from SIM memory.
     *
     * Sends AT+CMGD=<index> command to remove a single SMS message.
     *
     * @param index SMS storage index to delete.
     *
     * @return true if SIM7670E confirms successful deletion, false otherwise.
     */
    inline bool deleteSms(uint16_t index)
    {
        String command = "AT+CMGD=" + String(index);

        return sendCommand(command);
    }

    /**
     * @brief Deletes all SMS messages from SIM memory.
     *
     * Sends AT+CMGD=1,4 command to erase all stored SMS messages.
     *
     * @return true if SIM7670E confirms successful deletion, false otherwise.
     */
    inline bool deleteAllSms()
    {
        return sendCommand("AT+CMGD=1,4");
    }

    /**
     * @brief Sends an SMS message.
     *
     * Sends an SMS text message using the SIM7670E module.
     *
     * Process:
     * 1. Sends AT+CMGS command with recipient number.
     * 2. Waits for SMS input prompt (>).
     * 3. Sends message content.
     * 4. Terminates message using CTRL+Z.
     * 5. Waits for confirmation from modem.
     *
     * @param phoneNumber Recipient phone number in international format.
     * @param message SMS message body.
     * @param timeout Maximum wait time in milliseconds.
     *
     * @return true if SMS was successfully sent, false otherwise.
     */
    inline bool sendSms(
        const String &phoneNumber,
        const String &message,
        uint32_t timeout = 10000)
    {
        while (available())
        {
            read();
        }

        print("AT+CMGS=\"");
        print(phoneNumber);
        println("\"");

        String response = "";

        uint32_t start = millis();

        // Wait for SMS prompt >
        while (millis() - start < timeout)
        {
            while (available())
            {
                response += (char)read();
            }

            if (response.indexOf(">") != -1)
            {
                break;
            }
        }

        if (response.indexOf(">") == -1)
        {
            return false;
        }

        print(message);

        // CTRL+Z send SMS
        write(0x1A);

        response = "";

        start = millis();

        while (millis() - start < timeout)
        {
            while (available())
            {
                response += (char)read();
            }

            if (response.indexOf("OK") != -1)
            {
                return true;
            }

            if (response.indexOf("ERROR") != -1)
            {
                return false;
            }
        }

        return false;
    }
};

#endif