#include "globals.h"
#include "defaults.h"
#include "battery.h"

AS5600 encoder;
BleMouse bleMouse(BLE_DEVICE_NAME, "Mario", getBatteryLevel());