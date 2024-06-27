#include <M5Stack.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// UUID設定
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789012"
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-210987654321"

// I2Cピン設定
#define SDA_PIN 21
#define SCL_PIN 22
#define I2C_ADDRESS 0x57

MAX30105 particleSensor;
BLECharacteristic *pCharacteristic;

// BPM計算用変数
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int beatAvg = 0;

void setup() {
  M5.begin();
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  M5.Power.begin();

  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.println("Power is ON");

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

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST, I2C_ADDRESS)) {
    Serial.println("Failed to initialize MAX30102 sensor!");
    M5.Lcd.setCursor(10, 50);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
    M5.Lcd.println("Failed to init sensor");
    return;
  }
  Serial.println("MAX30102 sensor OK!");

  byte ledBrightness = 0x1F;
  byte sampleAverage = 8;
  byte ledMode = 2;
  int sampleRate = 400;
  int pulseWidth = 411;
  int adcRange = 4096;

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
  Serial.println("MAX30102 sensor setup complete.");
}

void loop() {
  M5.update();

  long irValue = particleSensor.getIR();

  if (irValue > 7000) {
    if (checkForBeat(irValue) == true) {
      long delta = millis() - lastBeat;
      lastBeat = millis();

      beatsPerMinute = 60 / (delta / 1000.0);

      if (beatsPerMinute < 255 && beatsPerMinute > 20) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;

        beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++) {
          beatAvg += rates[x];
        }
        beatAvg /= RATE_SIZE;
      }

      M5.Lcd.fillRect(0, 30, 320, 210, TFT_BLACK);
      M5.Lcd.setCursor(10, 50);
      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
      M5.Lcd.print("BPM: ");
      M5.Lcd.println(beatAvg);

      String bpmData = "BPM: " + String(beatAvg);
      pCharacteristic->setValue(bpmData.c_str());
      pCharacteristic->notify();
    }

    Serial.print("IR Value: ");
    Serial.println(irValue);
    Serial.print("BPM: ");
    Serial.println(beatAvg);

  } else {
    beatAvg = 0;
    M5.Lcd.fillRect(0, 30, 320, 210, TFT_BLACK);
    M5.Lcd.setCursor(10, 50);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
    M5.Lcd.println("No Finger Detected");
  }
}
