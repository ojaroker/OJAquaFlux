// Arduino code for AQUA-Flux 

#include <SPI.h> // to communicate with peripheral devices
#include <SD.h> // for SD card
#include <Wire.h> // for I2C communication
#include <RTClib.h> // for real time clock (RTC)
#include <SoftwareSerial.h> // to communicate with XBee
#include "SHT85.h" // for SHT85 sensor
//#include <Adafruit_SleepyDog.h> // for watchdog on R3
#include <WDT.h> //for watchdog on R4
#include <Servo.h> // for linear actuator


#define DEBUG 1

// -----------------------------------------------------------------------------
// Subsystem Enabling
// -----------------------------------------------------------------------------
#define USE_XBEE 1        // 0 - No XBee, Log to Serial; 1 - Use XBee for logging
#define USE_K30 1         // 0 - No K30, 1 - Use K30 for CO2 measurements
#define USE_ACTUATOR 0    // 0 - No actuator, 1 - Use linear actuator to open/close chamber
#define USE_SHT85 0       // 0 - No SHT85, 1 - Use SHT85 for temperature and humidity
#define USE_DATALOGGER 0  // 0 - No data logger, 1 - Log to SD card (also logs to XBee or Serial depending on USE_XBEE)
#define USE_RTC 0         // 0 - No RTC, 1 - Use RTC for timestamping 
// -----------------------------------------------------------------------------
// Arduino Configuration
// -----------------------------------------------------------------------------
// Pins: A4, A5 - I2C communication and bus recovery
#define I2C_SDA_PIN  A4 // SDA
#define I2C_SCL_PIN  A5 // SCL
// Pins: D2, D3 - XBee Software Serial RX/TX
#if USE_XBEE
  SoftwareSerial XBee(2, 3); // Arduino RX, TX (XBee Dout, Din)
#endif
// Pin D4 - Linear Actuator control
#define ACTUATOR_PIN 4
// Pin D7 - Solenoid control
#define SOLENOID_PIN 7
// Pin D10 - SPI Chip Select
#define SD_CARD_CS 10; // Chip Select for data logging SD card

// -----------------------------------------------------------------------------
// Logger Configuration
// -----------------------------------------------------------------------------
#if USE_XBEE
  #define LOG_STREAM XBee
  #define XBEE_BAUD_RATE 115200  // Use 115200 to minimize interference with actuator servo
#else
  #define LOG_STREAM Serial
  #define SERIAL_BAUD_RATE 9600
#endif




// Set the timing when the chamber will open and close (here and lines 532 and 557)
unsigned long open = 600000; //4 minutes open (milliseconds) (10 min = 600000)
unsigned long close = -3000000; // 15 minutes closed (milliseconds) (50 min = 3000000)


  
#define ECHO_TO_SERIAL   1 // echo data to serial port



void setup(void)
{
  setupLogging();

  setupI2c();
 
  setupK30();

  scanI2cBus();

  // CO2 Sensor
  // TODO - Check "Error Status" at 0x1E (See section 8 of data sheet) 
  // to detect sensor faults before continuing
  // K30 needs 60 seconds to boot up
  // K30 readings need 15 minutes to stabilize; discard first 15 minutes    

  setupActuator();

  setupSht85();

  setupRealTimeClock();

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
//     digitalWrite(SOLENOID_PIN, LOW);
// }

// void solenoidOn() {
//     digitalWrite(SOLENOID_PIN, HIGH);
// }

// Define CO2 sensor functions (from CO2 Meter: https://www.co2meter.com/blogs/news/arduino-co2-sensor-application-notes-update?srsltid=AfmBOorjk2wEjwo0dmR93q8CvLie9Tm6AxWMkbiFM6aaoy0IF2-oLG8o)

char time_to_read_CO2 = 1;
char n_delay_wait = 0;  


///////////////////////////////////////////////////////////////////

// Start sampling loop

void loop(){

  // Watchdog.reset(); // Must reset the watchdog at least every 8 seconds


  DateTime now; // Take the date/time
  
  // Delay for the amount of time we want between readings
  // delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL)); // doesn't work with watchdog enabled
  // with watchdog enabled, manually set the delay between readings:
  delay(4000);
  // Watchdog.reset();
  delay(3000);
  // Watchdog.reset();
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

  // Watchdog.reset();

  //delay(4000); 
  //Watchdog.reset();
  //delay(4000);
  // Watchdog.reset();
  // delay(4000);
  // Watchdog.reset();
  delay(4000);
  // Watchdog.reset();
   
  //delay(16000); // these delays are necessary for K33


  //
  // CO2 Sensor Measurement with K30
  //
  int16_t co2Value = readK30_CO2_withRetry();
  if (co2Value == 0) {
#if DEBUG
    LOG_STREAM.println(F("DEBUG - CO2 read failed after retries"));
    LOG_STREAM.print(F("DEBUG - Recovering I2C bus..."));
#endif    
    recoverI2CBus();
  }

  XBee.print(", ");
  XBee.print(co2Value); // print [CO2] to XBee



  // Take other sensor measurements

  // Watchdog.reset();

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

  // Watchdog.reset();

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

  // Watchdog.reset();
  
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

  // Watchdog.reset();

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
      // Watchdog.reset();
      delay(4000);
      // Watchdog.reset();
      delay(4000);
      // Watchdog.reset();
      delay(4000);
      // Watchdog.reset();
      delay(4000);
      // Watchdog.reset();
      delay(4000);
      // Watchdog.reset();
  }

  // Watchdog.reset();

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
      // Watchdog.reset();
      delay(4000);
      // Watchdog.reset();
      delay(4000);
      // Watchdog.reset();
      delay(4000);
      // Watchdog.reset();
      delay(4000);
      // Watchdog.reset();
      delay(4000);
      // Watchdog.reset();
  }

  // Vent the bubble trap with the solenoid valve, if full (full mV may differ depending on size of cylinder)

  // Watchdog.reset();

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
      // Watchdog.reset();
      delay(4000);
      // Watchdog.reset();
      delay(4000);
      // Watchdog.reset();
      delay(4000);
      // Watchdog.reset();
      delay(4000);
      // Watchdog.reset();
      delay(4000);
      // Watchdog.reset();
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
      // Watchdog.reset();
      delay(4000);
      // Watchdog.reset();
      delay(4000);
      // Watchdog.reset();
      delay(4000);
      // Watchdog.reset();
      delay(4000);
      // Watchdog.reset();
      delay(4000);
      // Watchdog.reset();
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

  // Watchdog.reset();

  logfile.println();
#if ECHO_TO_SERIAL
  Serial.println();
  XBee.println();
#endif // ECHO_TO_SERIAL

  // Write data to the SD card
  logfile.flush();

  // Watchdog.reset();


}


