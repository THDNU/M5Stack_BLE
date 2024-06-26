#include <M5Stack.h>

void setup() {
  // M5Stackの初期化
  M5.begin();

  // シリアル通信の初期化
  Serial.begin(115200);
  Serial.println("Initializing M5Stack...");

  // ディスプレイにメッセージを表示
  M5.Lcd.fillScreen(TFT_BLACK); // 背景を黒に設定
  M5.Lcd.setCursor(10, 10); // 表示位置を調整
  M5.Lcd.setTextSize(2); // テキストサイズを設定
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK); // テキストカラーを白、背景を黒に設定
  M5.Lcd.println("Power is ON"); // メッセージを表示

  // シリアルモニタにメッセージを表示
  Serial.println("Power is ON displayed on the screen");
}

void loop() {
  // メインループには何も入れない
}
