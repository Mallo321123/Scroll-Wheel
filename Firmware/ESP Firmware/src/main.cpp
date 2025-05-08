#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// UUIDs für den BLE-Service und die Charakteristik
// Diese standardisierten UUIDs funktionieren besser mit verschiedenen Clients
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART Service
#define CHARACTERISTIC_RX   "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // RX Characteristic (zum Schreiben durch Client)
#define CHARACTERISTIC_TX   "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // TX Characteristic (zum Lesen durch Client)

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 100; // 2 Sekunden

// Callback für Verbindungsereignisse
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Client verbunden");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Client getrennt");
    }
};

// Callback für empfangene Daten
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("Empfangen von Client:");
        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }
        Serial.println();

        // Echo zurücksenden
        std::string response = "Empfangen: " + rxValue;
        pTxCharacteristic->setValue(response);
        pTxCharacteristic->notify();
      }
    }
};

int getScrollValue() {
  return 30;
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 BLE UART Sensor");

  // BLE initialisieren
  BLEDevice::init("ESP32_BLEuart");
  
  // Server erstellen
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  // Service erstellen
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // TX Charakteristik (für Benachrichtigungen an Client)
  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  pTxCharacteristic->addDescriptor(new BLE2902());
  
  // RX Charakteristik (für Befehle vom Client)
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_RX,
                        BLECharacteristic::PROPERTY_WRITE
                      );
  pRxCharacteristic->setCallbacks(new MyCallbacks());
  
  // Service starten
  pService->start();
  
  // Werbung starten
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  // Diese Einstellungen verbessern die Erkennbarkeit bei iOS und Android
  pAdvertising->setMinPreferred(0x06);  
  pAdvertising->setMaxPreferred(0x12);
  
  BLEDevice::startAdvertising();
  Serial.println("BLE UART Gerät bereit, warte auf Verbindung...");
}

void loop() {
  // Regelmäßige Sensorwerte senden wenn verbunden
  if (deviceConnected && (millis() - lastSendTime >= sendInterval)) {
    lastSendTime = millis();
    
    // Sensorwert lesen
    int value = getScrollValue();
    Serial.println("Sensorwert: " + String(value));
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d", value);
    
    // Wert als Benachrichtigung senden
    pTxCharacteristic->setValue(buffer);
    pTxCharacteristic->notify();
    
    Serial.println("Benachrichtigung gesendet: " + String(buffer));
  }
  
  // Verbindungsstatus verwalten
  if (!deviceConnected && oldDeviceConnected) {
    // Wenn Verbindung getrennt wurde
    delay(500); // Zeit für BLE-Stack zum Bereinigen
    pServer->startAdvertising(); // Werbung neu starten
    Serial.println("Werbung neu gestartet");
    oldDeviceConnected = deviceConnected;
  }
  
  // Neue Verbindung erkannt
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
  
  delay(10); // CPU-Last reduzieren
}
