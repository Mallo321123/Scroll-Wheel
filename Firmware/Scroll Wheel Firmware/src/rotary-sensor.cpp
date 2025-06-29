#include <Wire.h>
#include <AS5600.h>

#include "defaults.h"
#include "globals.h"

float angle_before = 380.0; // Initial angle

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