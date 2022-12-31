// Arduino Unoに使用されているAVRはシングルスレッドのため、x86CPUのようなマルチスレッド処理はできないので
// 内部にステートを持ったクラスとライブラリを用いて擬似的に並列処理を表現する
// Arduino Playground - TimedAction Library
// https://playground.arduino.cc/Code/TimedAction/
#include <TimedAction.h>

// パッシブブザーで操作音を鳴らすライブラリ
#include "pitches.h"

#include <Wire.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

const int MPU_addr=0x68;  // I2C address of the MPU-6050
int16_t AcX;

// ドレミファソラシド
// NOTE_C5, NOTE_D5, NOTE_E5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_B5, NOTE_C6};
boolean servedSounds = true;

// プレイヤーとCPUが打った羽根が飛ぶアニメーションをLCDに表示する関数
void servedByPlayerAnimation();
void servedByCpuAnimation();

void swingAnimation();

// ※1 並列処理を行うため、ライブラリのクラスからインスタンスを生成
TimedAction playerAnimationAction = TimedAction(125, servedByPlayerAnimation); // ※1 
TimedAction cpuAnimationAction = TimedAction(125, servedByCpuAnimation); // ※1 

// プレイヤーとCOMの点数
int playerScore = 0;
int comScore = 0;

// 羽根が飛ぶアニメーション表現の為の変数
int hane = 0;
int hane2 = 15;

int pastGyroAngle = 0;
int currentGyroAngle = 0;
boolean servedByPlayer = true;
boolean judgeSwing = false;
boolean swung();
int sensing_gyro();

void setup() {
  lcd.begin(16, 2); // LCDの桁数と行数を指定(16桁2行)
  Serial.begin(9600);
  setup_gyro();
  // opening_title();
}

void loop() {
  pastGyroAngle = currentGyroAngle;
  currentGyroAngle = sensing_gyro();
  judgeSwing = swung(pastGyroAngle, currentGyroAngle);
  // Serial.println(judgeSwing);

  if (servedByPlayer) {
    playerAnimationAction.check();    
  } else {
    cpuAnimationAction.check();
  }

  if (servedByPlayer && servedSounds)  {
    tone(6, NOTE_C5, 100);
    servedSounds=false;
  } else if (servedSounds) {
    tone(6, NOTE_C6, 100);
    servedSounds=false;
  }
  
  lcd.setCursor(0, 1);
  lcd.print(" YOU "); lcd.print(playerScore); lcd.print(" -- "); lcd.print(comScore); lcd.print(" COM"); 
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

    Serial.println(playerScore);

    ending_title(playerScore, comScore);
  }
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

  // FIXME: スコアが3になった瞬間にTrueせる

  if (player_score == 3) {
    delay(3000);
    lcd.setCursor(0, 1);
    lcd.print("   YOU WIN!!!");
    delay(99999999); // delayで止めてゲーム終了
  } else {
    delay(3000);
    lcd.setCursor(0, 1);
    lcd.print("  YOU LOSE...");
    delay(99999999); // delayで止めてゲーム終了
  }
}

void setup_gyro() {
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
}

// ジャイロセンサーに必要な処理を関数化
int sensing_gyro() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,14,true); 
  AcX=Wire.read()<<8|Wire.read();
  // Serial.print("AcX = "); Serial.println(AcX);

  return AcX;
}

boolean swung(int before, int after) {
  if (before > 0 && after < 0) {
    return true;
  } else if (before < 0 && after > 0) {
    return true;
  }
  return false;
}
