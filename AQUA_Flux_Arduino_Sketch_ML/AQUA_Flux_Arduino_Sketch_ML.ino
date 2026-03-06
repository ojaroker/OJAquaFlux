// Arduino code for AQUA-Flux 
// Modified from Bastviken et al. (2020)

// Load required libraries
#include <SPI.h> // to communicate with peripheral devices
#include <SD.h> // for SD card
#include <Wire.h> // for I2C communication
#include <RTClib.h> // for real time clock (RTC)
#include <SoftwareSerial.h> // to communicate with XBee
#include "SHT85.h" // for SHT85 sensor
#include <Adafruit_SleepyDog.h> // for watchdog
#include <Servo.h> // for linear actuator

// Set the timing when the chamber will open and close (here and lines 532 and 557)
unsigned long open = 600000; //4 minutes open (milliseconds) (10 min = 600000)
unsigned long close = -3000000; // 15 minutes closed (milliseconds) (50 min = 3000000)

// Define the Arduino digital pin controlling the solenoid 
//const int solpin = 7; // Arduino pin that triggers solenoid

// Set up XBee on digital pins 2 and 3
SoftwareSerial XBee(2, 3); // Arduino RX, TX (XBee Dout, Din)

// Set the log interval (milliseconds between sensor measurements)
int LOG_INTERVAL = 10000; // If implementing watchdog (line 184), go to lines 321-328 to manually set the log interval
  
#define ECHO_TO_SERIAL   1 // echo data to serial port

// Set-up SHT85 sensor
#define SHT85_ADDRESS         0x44
SHT85 sht(SHT85_ADDRESS);

// Define the Arduino analog pins that connect to the CH4 and pressure sensors
#define CH4sens A0 // CH4 sensor 
#define Vb A1 // CH4 sensor reference circuit
#define P A2 // differential pressure sensor
int CH4s = 0;
int Vbat = 0;
//int Press = 0;

float CH4smV = 0;
float VbatmV = 0;
//float PmV = 0;

float mV = 5000; // voltage (5V)
float steps = 1024; // steps for ADC

// Define parameters for the water temp. thermistor (from Adafruit: https://learn.adafruit.com/thermistor/using-a-thermistor)
// analog pin
// #define THERMISTORPIN A3         
// // resistance at 25 degrees C
// #define THERMISTORNOMINAL 10000      
// // temp. for nominal resistance (almost always 25 C)
// #define TEMPERATURENOMINAL 25   
// // how many samples to take and average, more takes longer, but is more 'smooth'
// #define NUMSAMPLES 5
// // the beta coefficient of the thermistor (usually 3000-4000)
// #define BCOEFFICIENT 3950
// // the value of the 'other' resistor
// #define SERIESRESISTOR 10000    
// // digital pin providing 5V to the thermistor
// #define therm_power_pin 6        

//int samples[NUMSAMPLES];

// Define the Real Time Clock (RTC) object
RTC_PCF8523 rtc; 

// Create a servo object named "actuator"
Servo actuator; 

// For the data logging shield, we use digital pin 10 for the SD cs line
const int chipSelect = 10;

// Set-up the logging file
File logfile;

void error(char *str) // Halt if error
{
  Serial.print("File error: ");
  Serial.println(str);
  while(1); // Halt command
}

// Set-up the CO2 sensor
int co2Addr = 0x7F;
#define DEBUG 1

// Start the set-up loop

void setup(void)
{
  Serial.begin(9600);
  Wire.begin ();
  Serial.println();

  // Set pinMode to OUTPUT for solenoid and thermistor pins
  // pinMode(solpin, OUTPUT);
  // pinMode(therm_power_pin, OUTPUT);

  // Attach the actuator to Arduino digital pin 4
  actuator.attach(4); 
  actuator.writeMicroseconds(1000); // 1ms pulse to fully retract the actuator, 2ms pulse to fully extend the actuator (1000us = 1ms)
  
  // Initialize XBee Software Serial port. Make sure the baud rate matches your XBee setting (9600 is default). Need baud rate of 115200 to minimize XBee interference with actuator servo.
  XBee.begin(115200); 

  // If you want the Arduino to halt if the SHT85 sensor is not working
  // if (! sht.begin(SHT85_ADDRESS)) {
    // Serial.println("Could not find SHT");
    // XBee.println("Could not find SHT");
    // while (1) delay(10);
  // }
  // Serial.println("SHT85 found");
  // XBee.println("SHT85 found");
  
  sht.begin(); // start SHT85 sensor   

  // Initialize the SD card
  Serial.print("Initializing SD card...");
  XBee.print("Initializing SD card...");
  // Make sure that the default chip select pin is set to OUTPUT
  pinMode(10, OUTPUT);
  
  // See if the SD card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
  error("Card failed or not present");
  }
  Serial.println("card initialized.");
  XBee.println("card initialized.");
  
  // Create a new CSV file on the SD card
   char filename[] = "LOGGER00.CSV";
   for (uint16_t i = 0; i < 100; i++) {
   filename[5] = i/100 + '0';
   filename[6] = (i%100)/10 + '0';
   filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // leave the loop!
    }
  }
  
  if (! logfile) {
    error("Couldn't create file");
  }
  
  Serial.print("Logging to: ");
  XBee.print("Logging to: ");
  Serial.println(filename);
  XBee.println(filename);

  // Connect to RTC
 #ifndef ESP8266
  while (!Serial); // Wait for serial port to connect. Needed for native USB.
#endif

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    XBee.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (! rtc.initialized() || rtc.lostPower()) {
    Serial.println("RTC is NOT initialized, let's set the time!");
    XBee.println("RTC is NOT initialized, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    //
    // Note: allow 2 seconds after inserting battery or applying external power
    // without battery before calling adjust(). This gives the PCF8523's
    // crystal oscillator time to stabilize. If you call adjust() very quickly
    // after the RTC is powered, lostPower() may still return true.
  }

  rtc.start(); // start the RTC

  int countdownMS = Watchdog.enable(8000); // enable watchdog with timer of 8 seconds (max for Arduino Uno) - if the Arduino halts for more than 8 seconds, the watchdog will restart the sketch

  // List of data products: (1) milliseconds since Arduino was powered, (2) unique Unix stamp, (3) date and time, (4) [CO2] (ppm), (5) CH4 sensor output (mV),
  // (6) reference circuit output (mV), (7) relative humidity (%), (8) air temperature inside chamber (C), (9) AQUA-Flux ID
  logfile.println("millis, stampunix, datetime, K30_CO2, CH4smV, Vbat, SHT_RH, SHT_temp, AQUA_Flux1");    
#if ECHO_TO_SERIAL
  Serial.println("millis, stampunix, datetime, K30_CO2, CH4smV, Vbat, SHT_RH, SHT_temp, AQUA_Flux1");
  XBee.println("millis, stampunix, datetime, K30_CO2, CH4smV, Vbat, SHT_RH, SHT_temp, AQUA_Flux1");
#endif //ECHO_TO_SERIAL
}

// Define linear actuator functions
void extendActuator() {
  actuator.writeMicroseconds(1850); // give the actuator a 2ms pulse to fully extend (1000us = 1ms), recommended to not reach full 2ms
}

void retractActuator() {
  actuator.writeMicroseconds(1000); // 1ms pulse to fully retract, adjust the retraction time to compress the chamber gasket on the floating base
}


// Define solenoid functions
// void solenoidOff() {
//     digitalWrite(solpin, LOW);
// }

// void solenoidOn() {
//     digitalWrite(solpin, HIGH);
// }

// Define CO2 sensor functions (from CO2 Meter: https://www.co2meter.com/blogs/news/arduino-co2-sensor-application-notes-update?srsltid=AfmBOorjk2wEjwo0dmR93q8CvLie9Tm6AxWMkbiFM6aaoy0IF2-oLG8o)

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

// Start sampling loop

void loop(){

  Watchdog.reset(); // Must reset the watchdog at least every 8 seconds


  DateTime now; // Take the date/time
  
  // Delay for the amount of time we want between readings
  // delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL)); // doesn't work with watchdog enabled
  // with watchdog enabled, manually set the delay between readings:
  delay(4000);
  Watchdog.reset();
  delay(3000);
  Watchdog.reset();
  //delay(4000);
 // Watchdog.reset();
 // delay(1495);
 // Watchdog.reset();
  
  // Log the milliseconds since starting
  uint32_t m = millis();
  logfile.print(m);           // milliseconds since start
  logfile.print(", ");   
#if ECHO_TO_SERIAL
  Serial.print(m);         // milliseconds since start
  Serial.print(", ");  
  XBee.print(m);         // milliseconds since start
  XBee.print(", ");  
#endif

  // Fetch the time
  now = rtc.now();
  // Log time to SD card file and print to serial monitor and XBee
  logfile.print(now.unixtime()); // seconds since 1/1/1970
  logfile.print(", ");
  //logfile.print('"');
  logfile.print(now.year(), DEC);
  logfile.print("/");
  logfile.print(now.month(), DEC);
  logfile.print("/");
  logfile.print(now.day(), DEC);
  logfile.print(" ");
  logfile.print(now.hour(), DEC);
  logfile.print(":");
  logfile.print(now.minute(), DEC);
  logfile.print(":");
  logfile.print(now.second(), DEC);
  //logfile.print('"');
#if ECHO_TO_SERIAL
  Serial.print(now.unixtime()); // seconds since 1/1/1970
  Serial.print(", ");
  //Serial.print('"');
  Serial.print(now.year(), DEC);
  Serial.print("/");
  Serial.print(now.month(), DEC);
  Serial.print("/");
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.print(":");
  Serial.print(now.second(), DEC);
  //Serial.print('"');

  XBee.print(now.unixtime()); // seconds since 1/1/1970
  XBee.print(", ");
  //Serial.print('"');
  XBee.print(now.year(), DEC);
  XBee.print("/");
  XBee.print(now.month(), DEC);
  XBee.print("/");
  XBee.print(now.day(), DEC);
  XBee.print(" ");
  XBee.print(now.hour(), DEC);
  XBee.print(":");
  XBee.print(now.minute(), DEC);
  XBee.print(":");
  XBee.print(now.second(), DEC);
  //Serial.print('"');  
#endif //ECHO_TO_SERIAL

  // Take CO2 sensor measurement

  Watchdog.reset();

  //delay(4000); 
  //Watchdog.reset();
  //delay(4000);
  // Watchdog.reset();
  delay(4000);
  Watchdog.reset();
  delay(4000);
  Watchdog.reset();
   
  //delay(16000); // these delays are necessary for K33

  delay(50);
  int co2Value = readCO2();

  XBee.print(", ");
  XBee.print(co2Value); // print [CO2] to XBee

  // Take other sensor measurements

  Watchdog.reset();

  CH4s = analogRead(CH4sens); // read CH4 sensor output
  CH4smV = CH4s*(mV/steps); // convert pin reading to mV
  delay(10); // delay between reading of different analog pins 
  Vbat = analogRead(Vb); //read reference circuit
  VbatmV = Vbat *(mV/steps); // convert pin reading to mV 
  delay(10); // delay between reading of different analog pins
  XBee.print(", ");    
  XBee.print(CH4smV);    
  XBee.print(", ");    
  XBee.print(VbatmV);
  sht.read(); // read SHT85 sensor
  delay(10);
  XBee.print(", ");    
  XBee.print(sht.getHumidity(), 1);
  XBee.print(", ");    
  XBee.print(sht.getTemperature(), 1);

  Watchdog.reset();

  // Take water temperature measurement
  // uint8_t i;
  // float average;
  // // Take N samples in a row, with a slight delay
  // digitalWrite(therm_power_pin,HIGH);
  // for (i=0; i< NUMSAMPLES; i++) {
  //  samples[i] = analogRead(THERMISTORPIN);
  //  delay(10);
  // }
  // digitalWrite(therm_power_pin,LOW);
  // // average all the samples 
  // average = 0;
  // for (i=0; i< NUMSAMPLES; i++) {
  //    average += samples[i];
  // }
  // average /= NUMSAMPLES;
  // average = 1023 / average - 1;
  // average = SERIESRESISTOR / average;
  // float steinhart;
  // steinhart = average / THERMISTORNOMINAL;     // (R/Ro)
  // steinhart = log(steinhart);                  // ln(R/Ro)
  // steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  // steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  // steinhart = 1.0 / steinhart;                 // Invert
  // steinhart -= 273.15; // convert absolute temp to C
  // delay(10);  
  // XBee.print(", "); 
  // XBee.print(steinhart);
  // XBee.print(", ");

  // Watchdog.reset();

  // // Take differential pressure measurement
  // float averageP;
  // // Take N samples in a row, with a slight delay
  // for (i=0; i< NUMSAMPLES; i++) {
  //  samples[i] = analogRead(P);
  //  delay(10);
  // }
  // // average all the samples 
  // averageP = 0;
  // for (i=0; i< NUMSAMPLES; i++) {
  //    averageP += samples[i];
  // }
  // averageP /= NUMSAMPLES;
  // PmV = averageP *(mV/steps);
  // delay(10);  
  // XBee.print(PmV);
  // XBee.print(", ");

  // Watchdog.reset();  

  // Print all of the data to the SD card file                    
  logfile.print(", "); 
  logfile.print(co2Value);
  logfile.print(", ");   
  logfile.print(CH4smV);    
  logfile.print(", ");    
  logfile.print(VbatmV);
  logfile.print(", ");
  logfile.print(sht.getHumidity(), 1);
  logfile.print(", ");    
  logfile.print(sht.getTemperature(), 1);
  logfile.print(", ");    
  // logfile.print(steinhart);
  // logfile.print(", "); 
  // logfile.print(PmV);
  // logfile.print(", ");   

  Watchdog.reset();
  
#if ECHO_TO_SERIAL
  // Print all of the data to the serial monitor (if computer connected to Arduino)
  Serial.print(", "); 
  Serial.print(co2Value);
  Serial.print(", ");   
  Serial.print(CH4smV);    
  Serial.print(", ");    
  Serial.print(VbatmV);
  Serial.print(", ");
  Serial.print(sht.getHumidity(), 1);
  Serial.print(", ");    
  Serial.print(sht.getTemperature(), 1);
  Serial.print(", ");    
  // Serial.print(steinhart);
  // Serial.print(", "); 
  // Serial.print(PmV);
  // Serial.print(", "); 
   
#endif //ECHO_TO_SERIAL

  // Open or close the chamber with the linear actuator, if time

  Watchdog.reset();

  if (millis() - open >= 59.9*60*1000UL) // Change first number to minutes closed + minutes open - 0.1
  {open = millis();
      XBee.print("Opening");
      XBee.print(", ");
      logfile.print("Opening");
      logfile.print(", "); 
      Serial.print("Opening"); 
      Serial.print(", ");  
      extendActuator();
      delay(4000);
      Watchdog.reset();
      delay(4000);
      Watchdog.reset();
      delay(4000);
      Watchdog.reset();
      delay(4000);
      Watchdog.reset();
      delay(4000);
      Watchdog.reset();
      delay(4000);
      Watchdog.reset();
  }

  Watchdog.reset();

  if (millis() - close >= 59.9*60*1000UL) // Change first number to minutes closed + minutes open - 0.1
  {close = millis();
      XBee.print("Closing");
      XBee.print(", ");
      logfile.print("Closing");
      logfile.print(", "); 
      Serial.print("Closing");
      Serial.print(", ");
      retractActuator();
      delay(4000);
      Watchdog.reset();
      delay(4000);
      Watchdog.reset();
      delay(4000);
      Watchdog.reset();
      delay(4000);
      Watchdog.reset();
      delay(4000);
      Watchdog.reset();
      delay(4000);
      Watchdog.reset();
  }

  // Vent the bubble trap with the solenoid valve, if full (full mV may differ depending on size of cylinder)

  Watchdog.reset();

  // if (PmV > 800) // set to 800 mV
  // {XBee.print("Venting");
  //  XBee.print(", ");
  //  logfile.print("Venting");
  //  logfile.print(", "); 
  //  Serial.print("Venting"); 
  //  Serial.print(", ");  
  //  solenoidOn();
  //  delay(5000); // vent/open for 5 seconds
  //  solenoidOff();
  // }

  // Watchdog.reset();

  // Define XBee commands (optional, can help with testing/trouble-shooting)

  if (XBee.available())  
    { 
    char c = XBee.read();
    if (c == 'U') // "U" for "up" (command to open the chamber when XBee receives "U")
    {
      XBee.print("XBee Opening");
      XBee.print(", ");
      logfile.print("XBee Opening");
      logfile.print(", "); 
      Serial.print("XBee Opening"); 
      Serial.print(", ");
      extendActuator();
      delay(4000);
      Watchdog.reset();
      delay(4000);
      Watchdog.reset();
      delay(4000);
      Watchdog.reset();
      delay(4000);
      Watchdog.reset();
      delay(4000);
      Watchdog.reset();
      delay(4000);
      Watchdog.reset();
    }
    if (c == 'D') // "D" for "down" (command to close the chamber when XBee receives "D")
    {
      XBee.print("XBee Closing");
      XBee.print(", ");
      logfile.print("XBee Closing");
      logfile.print(", "); 
      Serial.print("XBee Closing");
      Serial.print(", ");
      retractActuator();
      delay(4000);
      Watchdog.reset();
      delay(4000);
      Watchdog.reset();
      delay(4000);
      Watchdog.reset();
      delay(4000);
      Watchdog.reset();
      delay(4000);
      Watchdog.reset();
      delay(4000);
      Watchdog.reset();
    }
  //   if (c == 'Y') // "Y" for "yes" (command to open the solenoid when XBee receives "Y")
  //   {
  //     XBee.print("SolenoidOn");
  //     XBee.print(", ");
  //     logfile.print("SolenoidOn");
  //     logfile.print(", ");
  //     Serial.print("SolenoidOn");
  //     Serial.print(", ");
  //     solenoidOn();
  //   }
  //   if (c == 'N') // "N" for "no" (command to close the solenoid when XBee receives "N")
  //   {
  //     XBee.print("SolenoidOff");
  //     XBee.print(", ");
  //     logfile.print("SolenoidOff");
  //     logfile.print(", ");
  //     Serial.print("SolenoidOff");
  //     Serial.print(", ");
  //     solenoidOff();
  //   }
  }

  Watchdog.reset();

  logfile.println();
#if ECHO_TO_SERIAL
  Serial.println();
  XBee.println();
#endif // ECHO_TO_SERIAL

  // Write data to the SD card
  logfile.flush();

  Watchdog.reset();


}
