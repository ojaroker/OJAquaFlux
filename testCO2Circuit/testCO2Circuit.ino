#include <SPI.h> // to communicate with peripheral devices
#include <SD.h> // for SD card
#include <Wire.h> // for I2C communication
#include <RTClib.h> // for real time clock (RTC)
#include "SHT85.h" // for SHT85 sensor

#define SHT85_ADDRESS         0x44
SHT85 sht(SHT85_ADDRESS);

#define CH4sens A0 // CH4 sensor 
#define Vb A1 // CH4 sensor reference circuit
#define P A2 // differential pressure sensor
int CH4s = 0;
int Vbat = 0;

#define DEBUG true

float CH4smV = 0;
float VbatmV = 0;


float mV = 5000; // voltage (5V)
float steps = 1024; // steps for ADC

int co2Addr = 0x68;

#define RED 4
#define GREEN1 5
#define GREEN2 6


void setup() {
  // sht.begin(); // start SHT85 sensor   
  Serial.begin(9600);
  Wire.begin();
  pinMode(RED,OUTPUT);
  pinMode(GREEN1,OUTPUT);
  pinMode(GREEN2,OUTPUT);

Serial.println("starting");


}


char time_to_read_CO2 = 1;
char n_delay_wait = 0; 


///////////////////////////////////////////////////////////////////
// Function : int readCO2()
// Returns : CO2 Value upon success, 0 upon checksum failure
// Assumes : - Wire library has been imported successfully.
// - LED is connected to IO pin 13
// - CO2 sensor address is defined in co2_addr
///////////////////////////////////////////////////////////////////
int readCO2()
{
 int co2_value = 0; // Store the CO2 value inside this variable.
 //////////////////////////
 /* Begin Write Sequence */
 //////////////////////////
 Wire.beginTransmission(co2Addr);
 Wire.write(0x22);
 Wire.write(0x00);
 Wire.write(0x08);
 Wire.write(0x2A);
 Wire.endTransmission();
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
 Wire.requestFrom(co2Addr, 4);
 byte i = 0;
 byte buffer[4] = {0, 0, 0, 0};
 /*
 Wire.available() is not necessary. Implementation is obscure but we
 leave it in here for portability and to future proof our code
 */
 while (Wire.available())
 {
 buffer[i] = Wire.read();
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

digitalWrite(RED,HIGH);
  digitalWrite(GREEN1,HIGH);
  digitalWrite(GREEN2,HIGH);
  delay(2000);

  int co2Value = readCO2();

  if(co2Value==0){
    digitalWrite(RED,HIGH);
    digitalWrite(GREEN1,LOW);
    digitalWrite(GREEN2,LOW);
  } else if(co2Value<5000){ //5000 is max value
    digitalWrite(RED,LOW);
    digitalWrite(GREEN1,HIGH);
    digitalWrite(GREEN2,HIGH);
  } else{
    digitalWrite(RED,LOW);
    digitalWrite(GREEN1,HIGH);
    digitalWrite(GREEN2,LOW);
  }



  delay(5000);
  







}
