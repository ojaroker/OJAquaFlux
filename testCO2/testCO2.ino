#include <SoftWire.h>
#include <SoftwareSerial.h>
#define SOFT_SDA 8
#define SOFT_SCL 9

SoftwareSerial XBee(2, 3);  // RX = 2, TX = 3
SoftWire myWire(SOFT_SDA, SOFT_SCL); // SDA, SCL

int co2Addr = 0x69;

void setup() {
  XBee.begin(115200);
  XBee.println("Software I2C Test Ready");
  myWire.begin();
}

int readCO2()
{
 int co2_value = 0; // Store the CO2 value inside this variable.
 //////////////////////////
 /* Begin Write Sequence */
 //////////////////////////
 myWire.beginTransmission(co2Addr);
 myWire.write(0x22);
 myWire.write(0x00);
 myWire.write(0x08);
 myWire.write(0x2A);
 myWire.endTransmission();
 /////////////////////////
 /* End Write Sequence. */
 /////////////////////////
 /*
 Wait 10ms for the sensor to process our command. The sensors's
 primary duties are to accurately measure CO2 values. Waiting 10ms
 ensures the data is properly written to RAM
 */
 delay(10);
 /////////////////////////
 /* Begin Read Sequence */
 /////////////////////////
 /*
 Since we requested 2 bytes from the sensor we must read in 4 bytes.
 This includes the payload, checksum, and command status byte.
 */
 myWire.requestFrom(co2Addr, 4);
 byte i = 0;
 byte buffer[4] = {0, 0, 0, 0};
 /*
 Wire.available() is not necessary. Implementation is obscure but we
 leave it in here for portability and to future proof our code
 */
 while (myWire.available())
 {
 buffer[i] = myWire.read();
 #if DEBUG
// print the output of the sensor
    Serial.println("Sensor Response: ");
    Serial.print("0x");
    Serial.print(buffer[i], HEX);
    Serial.print(" ");
#endif
 i++;
 }

 #if DEBUG
// adding in a line feed to make it more readable
  Serial.println("");
#endif

 ///////////////////////
 /* End Read Sequence */
 /////////////////////// 
 /*
 Using some bitwise manipulation we will shift our buffer
 into an integer for general consumption
 */
 co2_value = 0;
 co2_value |= buffer[1] & 0xFF;
 co2_value = co2_value << 8;
 co2_value |= buffer[2] & 0xFF;
 byte sum = 0; //Checksum Byte
 sum = buffer[0] + buffer[1] + buffer[2]; //Byte addition utilizes overflow
 if (sum == buffer[3])
 {
 // Success!
 return co2_value;
 }
 else
 {
 // Failure!
 /*
 Checksum failure can be due to a number of factors,
 fuzzy electrons, sensor busy, etc.
 */
 return 0;
 }
}

void loop() {
  int co2 = readCO2();
  XBee.print("CO2: ");
  XBee.println(co2);
  delay(1000);
}
