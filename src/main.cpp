#include <Arduino.h>
#include "BluetoothSerial.h"
#include "rgb_lcd.h"
#include <Wire.h>

#include <ESP32Encoder.h>
#include <ESP32Servo.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

Adafruit_MPU6050 mpu;
ESP32Encoder encoder;
Servo myservo;
rgb_lcd lcd;
BluetoothSerial SerialBT;

int Red_value=0;
int Green_value=0;
int Blue_value=0;
int Speed_value=0;
char revornot=LOW; //Direction variable, LOW for forward, HIGH for reverse

const int PWM_CHANNEL = 0;
const int PWM_FREQ = 25000;           // 25 kHz frequency
const int PWM_RESOLUTION = 11;        // 11 bits of resolution: 0-2047
const int servoPin = 13;              // Pin connected to the servo signal wire

void posinit(void);

void setup() {
  Serial.begin(115200);
  SerialBT.begin("Italian Smartfridge"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");

  Wire1.setPins(15, 5);
  lcd.begin(16, 2, LCD_5x8DOTS, Wire1);
  lcd.setRGB(0, 0, 0);

  // Initialize input/output pins
  pinMode(0, INPUT_PULLUP);   // Button 0
  pinMode(2, INPUT_PULLUP);   // Button 1
  pinMode(12, INPUT_PULLUP);  // Button 2
  pinMode(33, INPUT);         // Potentiometer

  pinMode(27, OUTPUT);        // PWM signal
  pinMode(26, OUTPUT);        // Motor direction
  pinMode(25, OUTPUT);        // Motor enable
  pinMode(13, OUTPUT);        // Servo motor

  pinMode(36, INPUT);         // Motor arm sensor (CNY70)

  encoder.attachFullQuad(23, 19);
  encoder.setCount(0);

  // Configure PWM on pin 27: 25 kHz, 11-bit resolution (0-2047)
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(27, PWM_CHANNEL);

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found");
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50);
  myservo.attach(servoPin);

  mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
  mpu.setMotionDetectionThreshold(1);
  mpu.setMotionDetectionDuration(20);
  mpu.setInterruptPinLatch(true);
  mpu.setInterruptPinPolarity(true);
  mpu.setMotionInterrupt(true);

  posinit();
  myservo.write(88);
}

void loop() {
  // Process Bluetooth data with timeout-based buffering
  static String bluetoothBuffer = "";
  static unsigned long lastDataTime = 0;
  const unsigned long COMMAND_TIMEOUT = 20;
  
  while (SerialBT.available()) {
    char c = SerialBT.read();
    bluetoothBuffer += c;
    lastDataTime = millis();
  }
  
  // Process buffer if we have data and timeout has elapsed
  if (bluetoothBuffer.length() > 0 && (millis() - lastDataTime) > COMMAND_TIMEOUT) {
    bluetoothBuffer.trim();
    char cmd = bluetoothBuffer[0];
    
    if (cmd == 'R' && bluetoothBuffer.length() > 1) {
      Red_value = bluetoothBuffer.substring(1).toInt();
      Serial.print("Red: ");
      Serial.println(Red_value);
    }
    else if (cmd == 'G' && bluetoothBuffer.length() > 1) {
      Green_value = bluetoothBuffer.substring(1).toInt();
      Serial.print("Green: ");
      Serial.println(Green_value);
    }
    else if (cmd == 'B' && bluetoothBuffer.length() > 1) {
      Blue_value = bluetoothBuffer.substring(1).toInt();
      Serial.print("Blue: ");
      Serial.println(Blue_value);
    }
    else if (cmd == 'C') {
      revornot = HIGH; // Reverse
      Serial.println("Reverse");
    }
    else if (cmd == 'F') {
      revornot = LOW;  // Forward
      Serial.println("Forward");
    }
    else if (cmd == 'A' && bluetoothBuffer.length() > 1) {
      Speed_value = bluetoothBuffer.substring(1).toInt();
      Serial.print("Speed: ");
      Serial.println(Speed_value);
    }
    else if (cmd == 'S') {
      Serial.println("Init position");
      posinit();
    }

    bluetoothBuffer = "";
  }

  // Check motor arm sensor and send signal continuously when detected
  int sensorValue = analogRead(36);

  if (sensorValue > 2000) {
    SerialBT.print("*LR255G200B0");
    Serial.println("Arm detected - sending L");
    encoder.setCount(0);
  } else {
    SerialBT.print("*LR0G0B0");
  }

  // Get encoder position and send gauge data
  int32_t encoderValue = encoder.getCount();
  // Wrap encoder value to 0-800 range (0 and 800 are the same position)
  int gaugValue = encoderValue % 800;
  if (gaugValue < 0) gaugValue += 800;

  SerialBT.print("*T");
  SerialBT.println(gaugValue);
  Serial.print("Gauge position: ");
  Serial.println(gaugValue);

  // Update LCD with current color values
  lcd.setRGB(Red_value, Green_value, Blue_value);

  // Scale Speed_value (0-255) to PWM range (0-2047 for 11-bit resolution)
  int PWM_value = map(Speed_value, 0, 255, 0, 2047);

  // Motor control
  if (PWM_value > 0) {
    digitalWrite(25, HIGH);
    ledcWrite(PWM_CHANNEL, PWM_value);
    digitalWrite(26, revornot);
  } else {
    digitalWrite(25, LOW);
    ledcWrite(PWM_CHANNEL, 0);
  }

  delay(50);

}


void posinit(void) {
  digitalWrite(25, HIGH);
  digitalWrite(26, HIGH);

  while (analogRead(36) < 2000) {
    ledcWrite(PWM_CHANNEL, 650);
    delay(10);
  }
  ledcWrite(PWM_CHANNEL, 0);
  encoder.setCount(0);
}