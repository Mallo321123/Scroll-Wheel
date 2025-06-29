#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "BLE2902.h"
#include "BLEHIDDevice.h"
#include "HIDTypes.h"
#include "HIDKeyboardTypes.h"
#include <driver/adc.h>
#include "sdkconfig.h"

#include "BleConnectionStatus.h"
#include "BleMouse.h"

#if defined(CONFIG_ARDUHAL_ESP_LOG)
  #include "esp32-hal-log.h"
  #define LOG_TAG ""
#else
  #include "esp_log.h"
  static const char* LOG_TAG = "BLEDevice";
#endif

static const uint8_t _hidReportDescriptor[] = {
  USAGE_PAGE(1),       0x01, // USAGE_PAGE (Generic Desktop)
  USAGE(1),            0x02, // USAGE (Mouse)
  COLLECTION(1),       0x01, // COLLECTION (Application)
  USAGE_PAGE(1),       0x01, //   USAGE_PAGE (Generic Desktop)
  USAGE(1),            0x02, //   USAGE (Mouse)
  COLLECTION(1),       0x02, //   COLLECTION (Logical)
  REPORT_ID(1),        0x01, //     REPORT_ID (1)
  USAGE(1),            0x01, //     USAGE (Pointer)
  COLLECTION(1),       0x00, //     COLLECTION (Physical)
  // ------------------------------------------------- Buttons (Left, Right, Middle, Back, Forward)
  USAGE_PAGE(1),       0x09, //     USAGE_PAGE (Button)
  USAGE_MINIMUM(1),    0x01, //     USAGE_MINIMUM (Button 1)
  USAGE_MAXIMUM(1),    0x05, //     USAGE_MAXIMUM (Button 5)
  LOGICAL_MINIMUM(1),  0x00, //     LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1),  0x01, //     LOGICAL_MAXIMUM (1)
  REPORT_SIZE(1),      0x01, //     REPORT_SIZE (1)
  REPORT_COUNT(1),     0x05, //     REPORT_COUNT (5)
  HIDINPUT(1),         0x02, //     INPUT (Data, Variable, Absolute) ;5 button bits
  // ------------------------------------------------- Padding
  REPORT_SIZE(1),      0x03, //     REPORT_SIZE (3)
  REPORT_COUNT(1),     0x01, //     REPORT_COUNT (1)
  HIDINPUT(1),         0x03, //     INPUT (Constant, Variable, Absolute) ;3 bit padding
  // ------------------------------------------------- X/Y position
  USAGE_PAGE(1),       0x01, //       USAGE_PAGE (Generic Desktop)
  USAGE(1),            0x30, //       USAGE (X)
  USAGE(1),            0x31, //       USAGE (Y)
  LOGICAL_MINIMUM(1),  0x81, //       LOGICAL_MINIMUM (-127)
  LOGICAL_MAXIMUM(1),  0x7f, //       LOGICAL_MAXIMUM (127)
  REPORT_SIZE(1),      0x08, //       REPORT_SIZE (8)
  REPORT_COUNT(1),     0x02, //       REPORT_COUNT (2)
  HIDINPUT(1),         0x06, //       INPUT (Data, Variable, Relative) ;3 bytes (X,Y,Wheel)
  // --------------------------------------------------- Wheel
  USAGE(1),            0x38, //       USAGE (Wheel)
  PHYSICAL_MINIMUM(1), 0x00, //       PHYSICAL_MINIMUM (0)
  PHYSICAL_MAXIMUM(1), 0x00, //       PHYSICAL_MAXIMUM (0)
  LOGICAL_MINIMUM(1),  0x81, //       LOGICAL_MINIMUM (-127)
  LOGICAL_MAXIMUM(1),  0x7f, //       LOGICAL_MAXIMUM (127)
  REPORT_SIZE(1),      0x08, //       REPORT_SIZE (8)
  REPORT_COUNT(1),     0x01, //       REPORT_COUNT (1)
  HIDINPUT(1),         0x06, //       INPUT (Data, Variable, Relative)
  // ------------------------------------------------- Resolution Multiplier
  USAGE_PAGE(1),       0x01, //       Generic Desktop
  USAGE(1),            0x48, //       Usage: Resolution Multiplier
  LOGICAL_MINIMUM(1),  0x00, //       Logical Min = 0
  LOGICAL_MAXIMUM(1),  0x01, //       Logical Max = 1
  PHYSICAL_MINIMUM(1), 0x01, //       Physical Min = 1
  PHYSICAL_MAXIMUM(1), 0x80, //       Physical Max = 128
  REPORT_SIZE(1),      0x08, //       REPORT_SIZE (8)
  REPORT_COUNT(1),     0x01, //       REPORT_COUNT (1)
  FEATURE(1),          0x02, //       Feature (Data,Var,Abs)
  END_COLLECTION(0),         //     END_COLLECTION (Physical)
  END_COLLECTION(0),         //   END_COLLECTION (Logical)
  END_COLLECTION(0),         // END_COLLECTION (Application)
};

BleMouse::BleMouse(std::string deviceName, std::string deviceManufacturer, uint8_t batteryLevel) : 
    _buttons(0),
    hid(0)
{
  this->deviceName = deviceName;
  this->deviceManufacturer = deviceManufacturer;
  this->batteryLevel = batteryLevel;
  this->connectionStatus = new BleConnectionStatus();
}

void BleMouse::begin(void)
{
  xTaskCreate(this->taskServer, "server", 20000, (void *)this, 5, NULL);
}

void BleMouse::end(void)
{
}

void BleMouse::click(uint8_t b)
{
  _buttons = b;
  move(0,0,0,0);
  _buttons = 0;
  move(0,0,0,0);
}

void BleMouse::move(signed char x, signed char y, signed char wheel, signed char hWheel)
{
  if (this->isConnected())
  {
    uint8_t m[] = {_buttons,x,y,wheel};
    this->inputMouse->setValue(m, sizeof(m));
    this->inputMouse->notify();
  }
}

void BleMouse::buttons(uint8_t b)
{
  if (b != _buttons)
  {
    _buttons = b;
    move(0,0,0,0);
  }
}

void BleMouse::press(uint8_t b)
{
  buttons(_buttons | b);
}

void BleMouse::release(uint8_t b)
{
  buttons(_buttons & ~b);
}

bool BleMouse::isPressed(uint8_t b)
{
  if ((b & _buttons) > 0)
    return true;
  return false;
}

bool BleMouse::isConnected(void) {
  return this->connectionStatus->connected;
}

void BleMouse::setBatteryLevel(uint8_t level) {
  this->batteryLevel = level;
  if (hid != 0)
      this->hid->setBatteryLevel(this->batteryLevel);
}

void BleMouse::taskServer(void* pvParameter) {
  BleMouse* bleMouseInstance = (BleMouse *) pvParameter; //static_cast<BleMouse *>(pvParameter);
  BLEDevice::init(bleMouseInstance->deviceName);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(bleMouseInstance->connectionStatus);

  bleMouseInstance->hid = new BLEHIDDevice(pServer);
  bleMouseInstance->inputMouse = bleMouseInstance->hid->inputReport(0x01); // <-- input REPORTID from report map
  bleMouseInstance->featureResolution = bleMouseInstance->hid->featureReport(0x01); // <-- feature REPORTID for resolution multiplier  
  bleMouseInstance->featureResolution->setValue(new uint8_t{0x80}, 1);
  bleMouseInstance->connectionStatus->inputMouse = bleMouseInstance->inputMouse;

  bleMouseInstance->hid->manufacturer()->setValue(bleMouseInstance->deviceManufacturer);

  bleMouseInstance->hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
  bleMouseInstance->hid->hidInfo(0x00,0x02);

  BLESecurity *pSecurity = new BLESecurity();

  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

  bleMouseInstance->hid->reportMap((uint8_t*)_hidReportDescriptor, sizeof(_hidReportDescriptor));
  bleMouseInstance->hid->startServices();

  bleMouseInstance->onStarted(pServer);

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->setAppearance(HID_MOUSE);
  pAdvertising->addServiceUUID(bleMouseInstance->hid->hidService()->getUUID());
  pAdvertising->start();
  bleMouseInstance->hid->setBatteryLevel(bleMouseInstance->batteryLevel);

  ESP_LOGD(LOG_TAG, "Advertising started!");
  vTaskDelay(portMAX_DELAY); //delay(portMAX_DELAY);
}
