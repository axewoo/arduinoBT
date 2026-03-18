#include <Arduino.h>
#include <CAN.h>
#define TX_GPIO_NUM 5
#define RX_GPIO_NUM 4
void setup()
{
Serial.begin(115200);
Serial.println("Emission CAN");
// Set the pins
CAN.setPins(RX_GPIO_NUM, TX_GPIO_NUM);
// start the CAN bus at 250 kbps
if (!CAN.begin(250E3))
{
Serial.println("Starting CAN failed!");
while (1);
}
}
void loop()
{
 // send packet: id is 11 bits, packet can contain up to 8 bytes of data
Serial.print("Sending packet ... ");
CAN.beginPacket(0x123);
CAN.write('A');
CAN.write('B');
CAN.endPacket();
Serial.println("done");
delay(1000);
}