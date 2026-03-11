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
const int PWM_FREQ = 25000;      // 25 kHz frequency
const int PWM_RESOLUTION = 11; // 11 bits of resolution: 0-2047
const int servoPin = 13; // Pin connected to the servo signal wire


unsigned long lastMovementTime = 0;
const unsigned long RETURN_DELAY = 3000; 

void posinit(void);
void posinitreverse(void);

char BluetoothData;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("Italian Smartfridge"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");

  Wire1.setPins(15, 5);
  lcd.begin(16, 2, LCD_5x8DOTS, Wire1);
  lcd.setRGB(0, 0, 0);

    // Initialise les entrées/sorties (enable internal pull-ups)
  pinMode(0, INPUT_PULLUP); // Bouton0 (active low)
  pinMode(2, INPUT_PULLUP); // Bouton1 (active low)
  pinMode(12, INPUT_PULLUP); // Bouton2 (active low)
  pinMode(33, INPUT); //Potentiomètre

  pinMode(27, OUTPUT); //PWM 
  pinMode(26, OUTPUT); //Signal Sens
  pinMode(25, OUTPUT); //Motor Disable
  pinMode(13, OUTPUT); //Servo Motor

  pinMode(36, INPUT); //Capteur CNY70

  encoder.attachFullQuad(23, 19);
  encoder.setCount(0); // Initialize encoder count to 0

  // Configure PWM on pin 27: 25 kHz, 8-bit resolution (0-255)
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
  mpu.setInterruptPinLatch(true);	// Keep it latched.  Will turn off when reinitialized.
  mpu.setInterruptPinPolarity(true);
  mpu.setMotionInterrupt(true);

 posinit();
 myservo.write(88); // Move servo to 90 degrees (neutral position)
}

void loop() {
  // Read encoder value
  int32_t encoderValue = encoder.getCount();

  // Process Bluetooth data with timeout-based buffering
  static String bluetoothBuffer = "";
  static unsigned long lastDataTime = 0;
  const unsigned long COMMAND_TIMEOUT = 20; // milliseconds
  
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
    // Send signal continuously while arm is detected
    SerialBT.print("*LR255G200B0");
    Serial.println("Arm detected - sending L");
    encoder.setCount(0); // Reset encoder count when arm is detected
  }
  if (sensorValue <= 2000) {
    // Send signal continuously while arm is not detected
    SerialBT.print("*LR0G0B0");
  }

  // Update LCD with current color values
  lcd.setRGB(Red_value, Green_value, Blue_value);

  // Scale Speed_value (0-255) to PWM range (0-2047 for 11-bit resolution)
  int PWM_value = map(Speed_value, 0, 255, 0, 2047);
  
  // Motor control - always keep motor enabled when PWM > 0
  if (PWM_value > 0) {
    digitalWrite(25, HIGH); // Enable motor
    ledcWrite(PWM_CHANNEL, PWM_value);
    digitalWrite(26, revornot); // Set direction
  } else {
    digitalWrite(25, LOW); // Disable motor when speed is 0
    ledcWrite(PWM_CHANNEL, 0);
  }

  //delay(50); // Allow LCD/I2C communication and Bluetooth buffering

}


void posinit(void){
  digitalWrite(25, HIGH); // Enable motor
  digitalWrite(26, HIGH); // Set direction
  
  while (analogRead(36) < 2000){
    ledcWrite(PWM_CHANNEL, 650); // Higher speed

    delay(10); // Small delay to allow sensor reading
  }
  ledcWrite(PWM_CHANNEL, 0); // Stop motor
  encoder.setCount(0); // Reset encoder count
  return;
}