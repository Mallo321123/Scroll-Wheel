#ifndef DEFAULTS_H
#define DEFAULTS_H

#define FIRMWARE_VERSION "0.2.0"
#define BLE_DEVICE_NAME "Scroll Wheel" // Name of the BLE device

#define SCROLL_UPDATE_INTERVAL 100   // Interval for scroll value update in ms
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

#endif