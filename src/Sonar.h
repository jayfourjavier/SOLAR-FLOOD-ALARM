#ifndef SONAR_H
#define SONAR_H

#include <Arduino.h>

/**
 * @file Sonar.h
 * @brief Ultrasonic distance sensor (HC-SR04) control class.
 */

/**
 * @class Sonar
 * @brief HC-SR04 Ultrasonic sensor abstraction for distance measurement.
 */
class Sonar
{
private:
    int trigPin;             /**< GPIO pin connected to Trig */
    int echoPin;             /**< GPIO pin connected to Echo */
    unsigned long timeoutUs; /**< Pulse timeout in microseconds */
    int medianFilterSize;    /**< Number of samples for median filter */

    /**
     * @brief Send trigger pulse to start measurement.
     */
    void sendTrigger()
    {
        digitalWrite(trigPin, LOW);
        delayMicroseconds(2);
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
    }

    /**
     * @brief Simple bubble sort for median calculation.
     * @param arr Array of floats
     * @param n Array size
     */
    void sortArray(float arr[], int n)
    {
        for (int i = 0; i < n - 1; i++)
        {
            for (int j = 0; j < n - i - 1; j++)
            {
                if (arr[j] > arr[j + 1])
                {
                    float temp = arr[j];
                    arr[j] = arr[j + 1];
                    arr[j + 1] = temp;
                }
            }
        }
    }

public:
    /**
     * @brief Constructor for Sonar.
     * @param trig GPIO pin connected to Trig
     * @param echo GPIO pin connected to Echo
     * @param timeout Pulse timeout in microseconds (default: 30000 = 5m range)
     * @param medianSize Number of samples for median filter (default: 5)
     */
    Sonar(int trig, int echo, unsigned long timeout = 30000, int medianSize = 5)
    {
        trigPin = trig;
        echoPin = echo;
        timeoutUs = timeout;
        medianFilterSize = medianSize;
    }

    /**
     * @brief Initialize sonar pins.
     */
    void begin()
    {
        pinMode(trigPin, OUTPUT);
        pinMode(echoPin, INPUT);
        digitalWrite(trigPin, LOW);
    }

    /**
     * @brief Take a single distance measurement.
     * @return Distance in centimeters, or -1 if error/timeout
     */
    float measureDistance()
    {
        sendTrigger();

        // Measure echo pulse duration in microseconds
        unsigned long duration = pulseIn(echoPin, HIGH, timeoutUs);

        if (duration == 0)
        {
            return -1; // Timeout - no echo received
        }

        // Calculate distance: speed of sound = 343m/s = 0.0343cm/us
        // Distance = (duration * speed) / 2 (round trip)
        float distanceCm = duration * 0.0343 / 2.0;

        // Check for invalid readings (HC-SR04 range is 2cm - 400cm)
        if (distanceCm < 2.0 || distanceCm > 400.0)
        {
            return -1;
        }

        return distanceCm;
    }

    /**
     * @brief Get median-filtered distance reading.
     * @return Filtered distance in centimeters, or -1 if no valid readings
     */
    float getMedianDistance()
    {
        float distances[medianFilterSize];
        int validReadings = 0;

        // Take multiple readings
        for (int i = 0; i < medianFilterSize; i++)
        {
            float reading = measureDistance();
            if (reading > 0)
            {
                distances[validReadings++] = reading;
            }
            delay(50); // Small delay between readings
        }

        if (validReadings == 0)
        {
            return -1;
        }

        // Sort and get median
        sortArray(distances, validReadings);

        // Return median value
        return distances[validReadings / 2];
    }

    /**
     * @brief Set median filter size.
     * @param size Number of samples for median filter (1-20)
     */
    void setMedianFilterSize(int size)
    {
        if (size >= 1 && size <= 20)
        {
            medianFilterSize = size;
        }
    }

    /**
     * @brief Set timeout for pulse reading.
     * @param timeout Timeout in microseconds
     */
    void setTimeout(unsigned long timeout)
    {
        timeoutUs = timeout;
    }
};

#endif