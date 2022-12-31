// Arduino Unoに使用されているAVRはシングルスレッドのため、x86CPUのようなマルチスレッド処理はできないので
// 内部にステートを持ったクラスとライブラリを用いて擬似的に並列処理を表現する
// Arduino Playground - TimedAction Library
// https://playground.arduino.cc/Code/TimedAction/
#include <TimedAction.h>

// パッシブブザーで操作音を鳴らすライブラリ
#include "pitches.h"

// ジャイロセンサーとLCD利用のためのライブラリ
#include <Wire.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
const int MPU_addr=0x68;  // I2C address of the MPU-6050
int16_t AcX;

// 打ち返し音を鳴らすか?
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
int hane = 0;
int hane2 = 15;

// 羽子板の当たり判定を計測するための変数
int pastGyroAngle = 0;
int currentGyroAngle = 0;
boolean judgeSwing = false;
unsigned long previousTime = 0; // 羽根を打ってから何秒経っているかを計測する
unsigned long adjustMoment = 0;
boolean succeedSwing = false;

// プレイヤーが打ち返し中かを判定するためのフラグ 
boolean servedByPlayer = true;

boolean isSwing();
int sensingGyro();

void setup() {
  lcd.begin(16, 2); // LCDの桁数と行数を指定(16桁2行)
  Serial.begin(9600);
  setupGyro();
  // opening_title();
}

void loop() {
  // スイングの挙動があったか?
  pastGyroAngle = currentGyroAngle;
  currentGyroAngle = sensingGyro(); 
  judgeSwing = isSwing(pastGyroAngle, currentGyroAngle);
  
  // TODO: 相手が打ってから1.4 ~ 1.9秒の間にスイング判定があれば打ち返し成功とみなす処理
  adjustMoment = millis() - previousTime;

Serial.println(adjustMoment);
  // 打ち返し判定
  if (judgeSwing && hitBuckable(adjustMoment)) {
    succeedSwing = true;
    Serial.println("Good! hit bucked!!");
  } else {
    succeedSwing = false;
    Serial.println("Missed..");
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

// 羽根を打ち返したときに鳴らすサウンドを関数化
void shuttlecockSounds() {
    if (servedByPlayer && servedSounds) {
    // tone(6, NOTE_C5, 100);
    servedSounds=false; // 鳴りっぱなしを防ぐために打ち返した瞬間以外はfalseにする
  } else if (servedSounds) {
    // tone(6, NOTE_C6, 100);
    servedSounds=false; // 鳴りっぱなしを防ぐために打ち返した瞬間以外はfalseにする
  }
}

// 羽子板のスイング判定。前後の値を取って±が入れ替わったらスイングしたとみなす
boolean isSwing(int before, int after) {
  if (before > 0 && after < 0) {
    return true;
  } else if (before < 0 && after > 0) {
    return true;
  }
  return false;
}

// 打ち返し可能時間を判定。相手が打ってから1.4 ~ 1.9秒の間であるか?
boolean hitBuckable(unsigned long adjust_moment) {
  adjust_moment >= 1400 && adjust_moment <= 1900
}

void servedByPlayerAnimation(){
  lcd.clear();
  lcd.setCursor(hane, 0);
  lcd.print(">");
  hane++;
  if (hane == 16) { 
    hane = 0; 
    servedByPlayer=false;
    servedSounds=true;
    playerScore++;

    ending_title(playerScore, comScore);
  }
  previousTime = millis() - previousTime;
}

void servedByCpuAnimation(){
  lcd.clear();
  lcd.setCursor(hane2, 0);
  lcd.print("<");
  hane2--;
  if (hane2 == -1) { 
    hane2 = 15; 
    servedByPlayer=true;
    servedSounds=true;
    comScore++;
    
    ending_title(playerScore, comScore);
  }
  previousTime = millis(); // COMが打ち返した時刻を記録
}

void opening_title() {
  lcd.setCursor(0, 0);
  lcd.print(" THE HANETSUKI");
  lcd.setCursor(0, 1);
  lcd.print("  GAME START!");
  delay(2000);
}

void ending_title(int player_score, int com_score) {
  if (player_score < 3 || com_score < 3) {
    return;
  }

  lcd.setCursor(0, 1);
  lcd.print(" YOU "); lcd.print(playerScore); lcd.print(" -- "); lcd.print(comScore); lcd.print(" COM"); 

  // FIXME: スコアが3になった瞬間にTrueにする
  if (player_score == 100) {
    delay(3000);
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("   YOU WIN!!!");
    delay(99999999); // delayで止めてゲーム終了
  } 
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
  AcX=Wire.read()<<8|Wire.read();
  // Serial.print("AcX = "); Serial.println(AcX);

  return AcX;
}
