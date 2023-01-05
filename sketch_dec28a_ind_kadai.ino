// Arduino Unoに使用されているAVRはシングルスレッドで、x86CPUのようなマルチスレッド処理はできないため
// 内部にステートを持ったクラスとライブラリを用いて擬似的に並列処理を表現する(プロトスレッド)
// Arduino Playground - TimedAction Library
// https://playground.arduino.cc/Code/TimedAction/
//
// e.g. arduinoで並列処理(ノンプリエンプティブ) - Qiita
// https://qiita.com/bunnyhopper_isolated/items/d4cf96895d665f5401af

#include <TimedAction.h>

// パッシブブザーで操作音を鳴らすライブラリ
#include "pitches.h"

// ジャイロセンサーとLCD利用のためのライブラリ
#include <Wire.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
const int MPU_addr=0x68;  // I2C address of the MPU-6050
int16_t AcX,AcY,AcZ;

// 打ち返し音を一瞬だけ鳴らすための変数
boolean servedSounds = true;

// プレイヤーとCPUが打った羽根が飛ぶアニメーションをLCDに表示する関数
void servedByPlayerAnimation();
void servedByCpuAnimation();

// ※1 並列処理を行うため、ライブラリのクラスからインスタンスを生成
TimedAction playerAnimationAction = TimedAction(125, servedByPlayerAnimation); // ※1 
TimedAction comAnimationAction = TimedAction(125, servedByCpuAnimation); // ※1 

// プレイヤーとCOMの点数
int playerScore = 0;
int comScore = 0;

// 羽根が飛ぶアニメーション表現のための変数
int playerShuttlecockPosition = 0;
int comShuttlecockPosition = 15;

// 羽子板の当たり判定を計測するための変数群
int pastGyroAngle = 0;
int currentGyroAngle = 0;
boolean judgeSwing = false;
unsigned long previousTime = 0; // 羽根を打ってから何秒経っているかを計測する
unsigned long adjustMoment = 0; // COMが打ってから経過した秒数(ミリセカンド)
boolean succeedSwing = true; // 打ち返しに成功したかを確認するフラグ

boolean servedByPlayer = true; // プレイヤーが打ち返し中かを判定するためのフラグ 

boolean isSwing();
int sensingGyro();

void setup() {
  lcd.begin(16, 2); // LCDの桁数と行数を指定(16桁2行)
  Serial.begin(9600);
  setupGyro();
  opening_title(); // オープニングタイトル表示処理
}

void loop() {
  // エンディング処理。プレイヤーまたはCOMが3点取ったらゲーム終了
  if (playerScore > 2) {
    delay(3000);
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("   YOU WIN!!!");
    delay(999999999); // delayで止めてゲーム終了
  } 
  if (comScore > 2) {
    delay(3000);
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("  YOU LOSE...");
    delay(999999999); // delayで止めてゲーム終了
  } 
  // エンディング処理ここまで


  // スイングの挙動があったか? を判定する為にジャイロセンサーの値を格納
  pastGyroAngle = currentGyroAngle;
  currentGyroAngle = sensingGyro(); 
  judgeSwing = isSwing(pastGyroAngle, currentGyroAngle);
  
  // Serial.print(judgeSwing); Serial.print("  "); Serial.println(adjustMoment);
  
  // 以下COMからの打球を打ち返せるかの判定
  if (!servedByPlayer) {
    
    // COMが打ってから経過した秒数を計測
    adjustMoment = millis() - previousTime;

    // COMが打ってから1.6~1.99秒の間にPlayerのスイング判定があれば、打ち返し成功とみなす処理
    if (judgeSwing && (adjustMoment >= 1600 && adjustMoment <= 1990)) {
      succeedSwing = true;
      Serial.println("Good! Hit bucked!!");
    }
  }
  
  // 羽根が飛ぶアニメーション
  if (servedByPlayer) {
    playerAnimationAction.check();
  } else {
    comAnimationAction.check();
  }

  // 羽根打ち返し音
  shuttlecockSounds();
  
  // ゲーム中LCDの2行目にスコアを表示させる
  lcd.setCursor(0, 1);
  lcd.print(" YOU "); lcd.print(playerScore); lcd.print(" -- "); lcd.print(comScore); lcd.print(" COM"); 
}

void servedByPlayerAnimation(){
  lcd.clear();
  lcd.setCursor(playerShuttlecockPosition, 0);
  lcd.print(">");
  playerShuttlecockPosition++;

  // COMがスイングミスした時の処理、ランダムに発生させる。ランダム要素: millis()で取得した値が割り切れたらミス判定
  if (playerShuttlecockPosition == 16 && (millis() % 7 == 0)) {
    playerScore++;
    playerShuttlecockPosition = 0; 
    succeedSwing = false;
    delay(3000);
    return;
  }

  if (playerShuttlecockPosition == 16) { 
    playerShuttlecockPosition = 0; 
    servedByPlayer = false;
    servedSounds = true;
    succeedSwing = false;
  }

  previousTime = millis(); // COMが打ち返しに成功: COMが打ち返してからの秒数を計測する為、打ち返し成功時刻を記録して変数で持っておく
}

void servedByCpuAnimation(){
  lcd.clear();
  lcd.setCursor(comShuttlecockPosition, 0);
  lcd.print("<");
  comShuttlecockPosition--;

  // Playerがスイングミスした時の処理
  if (comShuttlecockPosition == -1 && !succeedSwing) {
    comScore++;
    comShuttlecockPosition = 15;
    servedByPlayer = true;
    delay(3000);
    return;
  }

  if (comShuttlecockPosition == -1) { 
    comShuttlecockPosition = 15; 
    servedByPlayer = true;
    servedSounds = true;
  }
}

// 羽根を打ち返したときに鳴らすサウンドを関数化
void shuttlecockSounds() {
    if (servedByPlayer && servedSounds) {
    tone(6, NOTE_C5, 100);
    servedSounds = false; // 鳴りっぱなしを防ぐために打ち返した瞬間以外はfalseにする
  } else if (servedSounds) {
    tone(6, NOTE_C6, 100);
    servedSounds = false; // 鳴りっぱなしを防ぐために打ち返した瞬間以外はfalseにする
  }
}

// 羽子板のスイング判定。前後の値を取って±が入れ替わったらスイングしたとみなす
// 判定を少しゆるくするため±入れ替えに加えて、前後の値の差が1000以上あったらスイングしたとみなす
boolean isSwing(int before, int after) {
  if (before > 0 && after < 0) {
    return true;
  } else if (before < 0 && after > 0) {
    return true;
  } else if ((before * 2 / 2) - (after * 2 / 2) > 1000) { // 測定値の±を考慮しない
    return true;
  }
  return false;
}

void opening_title() {
  lcd.setCursor(0, 0);
  lcd.print(" THE HANETSUKI");
  lcd.setCursor(0, 1);
  lcd.print("  GAME START!");
  delay(3000);
}

// ジャイロセンサー使用の為の初期セットアップ
void setupGyro() {
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
}

// ジャイロセンサー(羽子板の動き)を計測する処理を関数化
int sensingGyro() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,14,true); 
  // AcX=Wire.read()<<8|Wire.read();
  // AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)


  // Serial.print("AcX = "); Serial.print(AcX);
  // Serial.print(" | AcY = "); Serial.print(AcY);
  Serial.print(" | AcZ = "); Serial.println(AcZ);

  return AcZ;
}
