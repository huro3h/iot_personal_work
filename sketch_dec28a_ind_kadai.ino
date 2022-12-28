// Arduino Unoに使用されているAVRはシングルスレッドのため、x86CPUのようなマルチスレッド処理はできないので
// 内部にステートを持ったクラスとライブラリを用いて擬似的に並列処理を表現する
// Arduino Playground - TimedAction Library
// https://playground.arduino.cc/Code/TimedAction/
#include <TimedAction.h>

#include <Wire.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

const int MPU_addr=0x68;  // I2C address of the MPU-6050
int16_t AcX;

void blink();
void blink2();
void blink3();

void sensing_gyro();

boolean hogeState = false;
boolean hogeState2 = false;
boolean hogeState3 = false;
int hane = 0;
int hane2 = 16;
boolean servedByPlayer = true;

TimedAction hogeAction = TimedAction(250, blink); // ※1 
TimedAction hogeAction2 = TimedAction(250, blink2); // ※1 
TimedAction hogeAction3 = TimedAction(1250, blink3); // ※1 


// ※1 並列処理を行うため、ライブラリのクラスからインスタンスを生成

void setup() {
  setup_gyro();
  Serial.begin(9600);

  // LCD
  lcd.begin(16, 2); // LCDの桁数と行数を指定(16桁2行)
  // lcd.setCursor(0, 0); // カーソルの位置を1列目先頭に指定
  // lcd.print("+"); 
  // lcd.setCursor(0, 1); // カーソルの位置を2列目先頭に指定
}

void loop() {
  servedByPlayer ? hogeAction.check() : hogeAction2.check();
  sensing_gyro();
}

void blink(){
  lcd.clear();
  lcd.setCursor(hane, 0);
  lcd.print("@");
  hane++;
  if (hane == 16) { 
    hane = 0; 
    servedByPlayer=false;
  }
}

void blink2(){
  lcd.clear();
  lcd.setCursor(hane2, 1);
  lcd.print("+");
  hane2--;
  if (hane2 == 0) { 
    hane2 = 16; 
    servedByPlayer=true;
  }
}

void blink3(){
  hogeState3 ? hogeState3=false : hogeState3=true;
  lcd.setCursor(6, 1); // カーソル位置2列目
  lcd.print(hogeState3);
}

void setup_gyro() {
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
}

void sensing_gyro() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,14,true); 
  AcX=Wire.read()<<8|Wire.read();
  
  Serial.print("AcX = "); Serial.println(AcX);
}
