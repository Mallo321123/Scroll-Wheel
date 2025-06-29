#include <Arduino.h>
#include <Wire.h>

#include "BleMouse.h"
#include "defaults.h"
#include "battery.h"
#include "main.h"
#include "globals.h"

unsigned long lastScrollUpdate = 0;
unsigned long lastBatteryTime = 0;

void setup()
{
    pinMode(PWR_SW_PIN, OUTPUT);
    digitalWrite(PWR_SW_PIN, HIGH); // Set PWR_SW_PIN high to keep the device on

    Serial.begin(115200);

    Serial.println("Scroll Wheel version " FIRMWARE_VERSION);
    Serial.println("Initializing...");

    bleMouse.begin();

    pinMode(BATTERY_SENSE_PIN, INPUT);
    pinMode(POWER_SENSE_PIN, INPUT);
    pinMode(CHARGE_STATE_SENSE_PIN, INPUT);
    analogReadResolution(12);

    Wire.begin(22, 21);

    if (!encoder.begin())
    {
        Serial.println("Rotary encoder not found!");
        while (1)
            ;
    }
    Serial.println("Scroll Wheel ready, waiting for client...");
}

void loop()
{
    if (bleMouse.isConnected() && (millis() - lastScrollUpdate >= SCROLL_UPDATE_INTERVAL))
    {
        lastScrollUpdate = millis();
        int value = getScrollValue();

        if (value != 0)
        {
            bleMouse.move(0, 0, value); // Send scroll value to BLE mouse
        }
    }
    delay(LOOP_SLEEP_TIME);
}