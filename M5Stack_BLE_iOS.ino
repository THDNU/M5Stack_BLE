#include <M5Stack.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// UUID設定
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789012"
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-210987654321"

BLECharacteristic *pCharacteristic;

void setup() {
  M5.begin();
  Serial.begin(115200); // シリアル通信の初期化
  BLEDevice::init("M5Stack");

  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("Waiting for a client connection to notify...");
}

void loop() {
  M5.update();
  
  if (M5.BtnA.wasPressed()) {
    pCharacteristic->setValue("1");
    pCharacteristic->notify();
    Serial.println("Button A pressed, sent 1");
  } else if (M5.BtnB.wasPressed()) {
    pCharacteristic->setValue("2");
    pCharacteristic->notify();
    Serial.println("Button B pressed, sent 2");
  } else if (M5.BtnC.wasPressed()) {
    pCharacteristic->setValue("3");
    pCharacteristic->notify();
    Serial.println("Button C pressed, sent 3");
  }

  delay(500); // シリアル出力を見やすくするための遅延
}
