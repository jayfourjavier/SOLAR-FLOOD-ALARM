#ifndef RELAY_H
#define RELAY_H

#include <Arduino.h>

/**
 * @file RELAY.h
 * @brief Simple relay control class with active LOW/HIGH support and debug logging.
 */

/**
 * @class Relay
 * @brief Relay control abstraction.
 *
 * Supports both active LOW and active HIGH relay modules.
 */
class Relay
{
private:
    int pin;          /**< GPIO pin connected to relay */
    const char *name; /**< Relay name (for debugging) */

    bool state;     /**< Current state (true = ON, false = OFF) */
    bool activeLow; /**< Logic type (true = LOW triggers ON) */

    /**
     * @brief Apply relay state to hardware.
     * @param on Desired state (true = ON, false = OFF)
     */
    void writeState(bool on)
    {
        state = on;

        if (activeLow)
        {
            digitalWrite(pin, on ? LOW : HIGH);
        }
        else
        {
            digitalWrite(pin, on ? HIGH : LOW);
        }
    }

public:
    /**
     * @brief Constructor for Relay.
     * @param relayPin GPIO pin connected to relay
     * @param relayName Name identifier for debugging
     * @param isActiveLow True if relay is active LOW (default: true)
     */
    Relay(int relayPin, const char *relayName, bool isActiveLow = true)
    {
        pin = relayPin;
        name = relayName;
        activeLow = isActiveLow;
        state = false;
    }

    /**
     * @brief Initialize relay pin and set default OFF state.
     */
    void begin()
    {
        pinMode(pin, OUTPUT);

        // default OFF state
        writeState(false);
    }

    /**
     * @brief Turn relay ON.
     */
    void on()
    {
        if (state)
            return;
        writeState(true);
    }

    /**
     * @brief Turn relay OFF.
     */
    void off()
    {
        if (!state)
            return;
        writeState(false);
    }

    /**
     * @brief Toggle relay state.
     */
    void toggle()
    {
        writeState(!state);
    }

    /**
     * @brief Get current relay state.
     * @return true if ON, false if OFF
     */
    bool getState()
    {
        return state;
    }
};

#endif