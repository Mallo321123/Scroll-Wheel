#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include <AS5600.h>

#define FIRMWARE_VERSION "0.1.0"

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"      // UART Service
#define CHARACTERISTIC_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // RX Characteristic
#define CHARACTERISTIC_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // TX Characteristic

#define BLE_DEVICE_NAME "Scroll Wheel" // Name of the BLE device

#define LOOP_SLEEP_TIME 10        // Sleep time in ms
#define RECONNECT_SLEEP_TIME 1000 // Sleep time in ms

#define SCROLL_UPDATE_INTERVAL 100 // Interval for scroll value update in ms

#define BATTERY_UPDATE_INTERVAL 5000 // Set battery update interval in s
#define BATTERY_MAX_VOLTAGE 4.2      // Maximal Battery Voltage
#define BATTERY_MIN_VOLTAGE 3.3      // Minimal Battery Voltage
#define BATTERY_VALUE_CORRECTION 1   // Battery value correction

#define PWR_SW_PIN 15             // Needs to be high for device to stay on
#define BATTERY_SENSE_PIN 32      // ADC pin for battery voltage sensing
#define POWER_SENSE_PIN 14        // ADC pin for sensing connected USB
#define CHARGE_STATE_SENSE_PIN 13 // ADC pin for sensing charge state

#define SCROLL_MULTIPLICATOR 1    // Multiplier for scroll value
#define JITTER_THRESHOLD 0.5      // Threshold for jitter in scroll angle
#define MAX_ROTATION_PER_READ 180 // Maximal rotation per read in degrees

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long lastScrollUpdate = 0;
unsigned long lastBatteryTime = 0;

float angle_before = 380.0; // Initial angle

AS5600 encoder;

// Callback for BLE server connection
class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
    Serial.println("Client connected");
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
    Serial.println("Client disconnected");
  }
};

// Callback for received data
class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0)
    {
      Serial.println("Recieved from client:");
      for (int i = 0; i < rxValue.length(); i++)
      {
        Serial.print(rxValue[i]);
      }
      Serial.println();
    }
  }
};

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

  pinMode(BATTERY_SENSE_PIN, INPUT);
  pinMode(POWER_SENSE_PIN, INPUT);
  pinMode(CHARGE_STATE_SENSE_PIN, INPUT);
  analogReadResolution(12);

  Wire.begin(22, 21);

  // BLE Initialisation
  BLEDevice::init(BLE_DEVICE_NAME);

  // Create Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // create Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // TX Charakteristics
  pTxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_TX,
      BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());

  // RX Charakteristcs
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_RX,
      BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // start service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);

  // Option for better visibility for android and IOS
  /*pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x12);*/

  BLEDevice::startAdvertising();

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
  // Regularly send scroll value
  if (deviceConnected && (millis() - lastScrollUpdate >= SCROLL_UPDATE_INTERVAL))
  {
    lastScrollUpdate = millis();

    // read sensor value
    int value = getScrollValue();
    char buffer[32];

    if (!(value == 0))
    {
      snprintf(buffer, sizeof(buffer), "SCR:%d", value);

      // Send value as notification
      pTxCharacteristic->setValue(buffer);
      pTxCharacteristic->notify();
    }
  }

  // Regularly send battery level
  if (deviceConnected && (millis() - lastBatteryTime >= BATTERY_UPDATE_INTERVAL))
  {
    lastBatteryTime = millis();

    // Read battery level
    int batteryLevel = getBatteryLevel();
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "BAT:%d", batteryLevel);

    // send battery level as notification
    pTxCharacteristic->setValue(buffer);
    pTxCharacteristic->notify();

    Serial.printf("Akkuladung: %d%%\n", batteryLevel);
  }

  // Verbindungsstatus verwalten
  if (!deviceConnected && oldDeviceConnected)
  {
    // on reconnect we can start advertising again
    delay(RECONNECT_SLEEP_TIME);

    pServer->startAdvertising(); // Start advertising
    Serial.println("BLE advertising started");
    oldDeviceConnected = deviceConnected;
  }

  // Detect new connections
  if (deviceConnected && !oldDeviceConnected)
  {
    oldDeviceConnected = deviceConnected;
  }

  delay(LOOP_SLEEP_TIME);
}
