#include <M5Stack.h>
#include <Wire.h>
#include "MAX30105.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <vector>

// UUID設定
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789012"
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-210987654321"

// I2Cピン設定
#define SDA_PIN 21
#define SCL_PIN 22
#define I2C_ADDRESS 0x57

MAX30105 particleSensor;
BLECharacteristic *pCharacteristic;

// データ収集用変数
std::vector<uint32_t> irData;
std::vector<uint32_t> redData;
unsigned long lastTime = 0;
const int samplingInterval = 1000; // 1秒ごとにサンプリング
const int collectionTime = 10; // データ収集時間（秒）

bool lastButtonState = false; // 前回のボタンの状態

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
  
  if (millis() - lastTime > samplingInterval) {
    lastTime = millis();
    uint32_t irValue = particleSensor.getIR();
    uint32_t redValue = particleSensor.getRed();
    
    if (irData.size() < collectionTime * (1000 / samplingInterval)) {
      irData.push_back(irValue);
      redData.push_back(redValue);
    } else {
      irData.erase(irData.begin());
      redData.erase(redData.begin());
      irData.push_back(irValue);
      redData.push_back(redValue);
    }
    
    Serial.print("IR Value: ");
    Serial.println(irValue);
    Serial.print("Red Value: ");
    Serial.println(redValue);

    // ボタンAの状態をデバッグメッセージで確認
    if (M5.BtnA.isPressed()) {
      Serial.println("Button A is pressed");
      if (!lastButtonState) {
        Serial.println("Button A pressed, calculating BPM...");
        calculateBPM();
      }
      lastButtonState = true;
    } else {
      lastButtonState = false;
    }
  }
}

void calculateBPM() {
  if (irData.size() < collectionTime * (1000 / samplingInterval)) {
    Serial.println("Not enough data to calculate BPM");
    return;
  }

  Serial.println("Calculating BPM...");
  
  // ピーク検出とBPM計算
  int peakCount = 0;
  for (int i = 1; i < irData.size() - 1; i++) {
    if (irData[i] > irData[i-1] && irData[i] > irData[i+1] && irData[i] > 20000) {
      peakCount++;
      Serial.print("Peak detected at index ");
      Serial.println(i);
    }
  }

  float bpm = (peakCount / (float)collectionTime) * 60;
  Serial.print("Peak Count: ");
  Serial.println(peakCount);
  Serial.print("BPM: ");
  Serial.println(bpm);

  M5.Lcd.fillRect(0, 30, 320, 210, TFT_BLACK);
  M5.Lcd.setCursor(10, 50);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.print("BPM: ");
  M5.Lcd.println(bpm);

  String bpmData = "BPM: " + String(bpm);
  pCharacteristic->setValue(bpmData.c_str());
  pCharacteristic->notify();
}
