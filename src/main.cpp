#include <Arduino.h>
#include "BluetoothSerial.h"
#include "rgb_lcd.h"
#include <Wire.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

rgb_lcd lcd;
BluetoothSerial SerialBT;

int Red_value=0;
int Green_value=0;
int Blue_value=0;

char BluetoothData;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("Italian Smartfridge"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");

  Wire1.setPins(15, 5);
  lcd.begin(16, 2, LCD_5x8DOTS, Wire1);
  lcd.setRGB(0, 0, 0);
}

void loop() {
  if (SerialBT.available()) {
    BluetoothData=SerialBT.read();
    if(BluetoothData=='R') Red_value=SerialBT.parseInt(); //Read Red value
    if(BluetoothData=='G') Green_value=SerialBT.parseInt(); //Read Green Value
    if(BluetoothData=='B') Blue_value=SerialBT.parseInt(); //Read Blue Value
  }

lcd.setRGB(Red_value, Green_value, Blue_value); //Set RGB values

  delay(20);
}