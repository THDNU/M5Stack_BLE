#include <M5Stack.h>
#include <Wire.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "MAX30105.h"

// UUID設定
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789012"
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-210987654321"

// I2Cピン設定
#define SDA_PIN 21
#define SCL_PIN 22
#define I2C_ADDRESS 0x57 // 正しいアドレスを設定

MAX30105 particleSensor;
BLECharacteristic *pCharacteristic;

void setup() {
  M5.begin();
  Serial.begin(115200); // シリアル通信の初期化
  Wire.begin(SDA_PIN, SCL_PIN); // I2C通信の初期化 (SDA, SCL)
  M5.Power.begin(); // 電源管理の初期化

  // ディスプレイに電源オンメッセージを表示
  M5.Lcd.fillScreen(TFT_BLACK); // 背景を黒に設定
  M5.Lcd.setCursor(10, 10); // 表示位置を調整
  M5.Lcd.setTextSize(2); // テキストサイズを設定
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK); // テキストカラーを白、背景を黒に設定
  M5.Lcd.println("Power is ON"); // メッセージを表示

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

  // MAX30102センサの初期化
  Serial.println("Initializing MAX30102 sensor...");
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST, I2C_ADDRESS)) {
    Serial.println("Failed to initialize MAX30102 sensor!");
    M5.Lcd.setCursor(10, 50); // 表示位置を調整
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(TFT_RED, TFT_BLACK); // 赤い文字、黒い背景
    M5.Lcd.println("Failed to init sensor");
    return;
  }
  Serial.println("MAX30102 sensor OK!");

  // センサの設定
  Serial.println("Setting up MAX30102 sensor...");
  byte ledBrightness = 0x1F; // 0=Off to 255=50mA
  byte sampleAverage = 8; // 1, 2, 4, 8, 16, 32
  byte ledMode = 2; // 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  int sampleRate = 400; // 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; // 69, 118, 215, 411
  int adcRange = 4096; // 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); // 設定を適用
  Serial.println("MAX30102 sensor setup complete.");
}

void loop() {
  M5.update(); // M5Stackの状態を更新
  
  if (M5.BtnA.wasPressed()) { // ボタンAが押されたときの処理
    Serial.println("Button A was pressed"); // デバッグ用メッセージ
    uint32_t irValue = particleSensor.getIR(); // 赤外線LEDのデータ取得
    uint32_t redValue = particleSensor.getRed(); // 赤色LEDのデータ取得

    if (irValue > 0 && redValue > 0) {
      String sensorData = "IR: " + String(irValue) + ", Red: " + String(redValue);
      pCharacteristic->setValue(sensorData.c_str());
      pCharacteristic->notify();
      Serial.print("IR: ");
      Serial.print(irValue);
      Serial.print(", Red: ");
      Serial.println(redValue);

      // センサデータをディスプレイに表示
      M5.Lcd.fillRect(0, 30, 320, 210, TFT_BLACK); // 画面の一部をクリア
      M5.Lcd.setCursor(10, 50); // 表示位置を調整
      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK); // 白い文字、黒い背景
      M5.Lcd.println("IR and Red:");
      M5.Lcd.setTextSize(3);
      M5.Lcd.print("IR: ");
      M5.Lcd.println(irValue);
      M5.Lcd.print("Red: ");
      M5.Lcd.println(redValue);
    } else {
      Serial.println("Failed to read heart rate sensor");
      // ディスプレイにエラーメッセージを表示
      M5.Lcd.fillRect(0, 30, 320, 210, TFT_BLACK); // 画面の一部をクリア
      M5.Lcd.setCursor(10, 50); // 表示位置を調整
      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
      M5.Lcd.println("Failed to read sensor");
    }
  }

  delay(500); // シリアル出力を見やすくするための遅延
}
