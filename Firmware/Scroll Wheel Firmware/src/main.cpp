#include <BleMouse.h>
#include <Arduino.h>
#include <Wire.h>
#include <AS5600.h>

#define FIRMWARE_VERSION "0.2.0"
#define BLE_DEVICE_NAME "Scroll Wheel" // Name of the BLE device

#define SCROLL_UPDATE_INTERVAL 100 // Interval for scroll value update in ms
#define BATTERY_UPDATE_INTERVAL 5000 // Set battery update interval in s
#define BATTERY_MAX_VOLTAGE 4.2      // Maximal Battery Voltage
#define BATTERY_MIN_VOLTAGE 3.3      // Minimal Battery Voltage
#define BATTERY_VALUE_CORRECTION 1   // Battery value correction

#define LOOP_SLEEP_TIME 5 // Sleep time in ms

#define PWR_SW_PIN 15             // Needs to be high for device to stay on
#define BATTERY_SENSE_PIN 32      // ADC pin for battery voltage sensing
#define POWER_SENSE_PIN 14        // ADC pin for sensing connected USB
#define CHARGE_STATE_SENSE_PIN 13 // ADC pin for sensing charge state

#define SCROLL_MULTIPLICATOR 1    // Multiplier for scroll value
#define JITTER_THRESHOLD 0.5      // Threshold for jitter in scroll angle
#define MAX_ROTATION_PER_READ 180 // Maximal rotation per read in degrees

BleMouse bleMouse;

unsigned long lastScrollUpdate = 0;
unsigned long lastBatteryTime = 0;

float angle_before = 380.0; // Initial angle

AS5600 encoder;

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

int getScrollValue()
{
    int rawAngle = encoder.readAngle(); // Value between 0 and 4095 (12-bit)
    float angleDeg = rawAngle * 360.0 / 4096.0;

    // set first angle after boot
    if (angle_before == 380.0)
    {
        angle_before = angleDeg;
    }

    float angleDiff = angleDeg - angle_before;

    if (fabs(angleDiff) < JITTER_THRESHOLD)
    {
        return 0; // Ignore small changes
    }

    // Handle wrap-around at 0/360 degrees
    if (angleDiff > MAX_ROTATION_PER_READ)
    {
        angleDiff -= 360.0;
    }
    else if (angleDiff < -MAX_ROTATION_PER_READ)
    {
        angleDiff += 360.0;
    }

    // Store current angle for next comparison
    angle_before = angleDeg;

    // Scale and round the value as needed
    int scrollValue = round(angleDiff * SCROLL_MULTIPLICATOR);

    return scrollValue;
}

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
    Serial.println("Encoder initialized");

    Serial.println("Scroll Wheel ready, waiting for client...");
}

void loop()
{
    if (bleMouse.isConnected() && (millis() - lastScrollUpdate >= SCROLL_UPDATE_INTERVAL))
    {
        lastScrollUpdate = millis();

        // read sensor value
        int value = getScrollValue();

        if (value != 0)
        {
            bleMouse.move(0, 0, value); // Send scroll value to BLE mouse
        }
    }
    delay(LOOP_SLEEP_TIME);
}