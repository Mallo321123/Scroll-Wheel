#include <Arduino.h>

#include "defaults.h"

int getBatteryLevel()
{
    // read raw value from ADC
    int rawValue = analogRead(BATTERY_SENSE_PIN);
    float voltage = rawValue * (3.3 / 4095.0) * BATTERY_VALUE_CORRECTION;

    // Pcalculate battery percentage
    int percentage = map(voltage * 100,
                         BATTERY_MIN_VOLTAGE * 100,
                         BATTERY_MAX_VOLTAGE * 100,
                         0,
                         100);

    // limit percentage to 0-100
    percentage = constrain(percentage, 0, 100);

    return percentage;
}