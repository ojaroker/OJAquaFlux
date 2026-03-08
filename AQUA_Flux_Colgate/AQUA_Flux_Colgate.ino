// Arduino code for AQUA-Flux
// Colgate University

#include <SPI.h>            // to communicate with peripheral devices
#include <SD.h>             // for SD card
#include <Wire.h>           // for I2C communication
#include <RTClib.h>         // for real time clock (RTC)
#include <SoftwareSerial.h> // to communicate with XBee
#include "SHT85.h"          // for SHT85 sensor
// #include <Adafruit_SleepyDog.h> // for watchdog on R3
#include <WDT.h>   //for watchdog on R4
#include <Servo.h> // for linear actuator

#define DEBUG 1

// -----------------------------------------------------------------------------
// Subsystem Enabling
// -----------------------------------------------------------------------------
#define USE_XBEE 1       // 0 - No XBee, Log to Serial; 1 - Use XBee for logging
#define USE_K30 1        // 0 - No K30, 1 - Use K30 for CO2 measurements
#define USE_ACTUATOR 0   // 0 - No actuator, 1 - Use linear actuator to open/close chamber
#define USE_SHT85 1      // 0 - No SHT85, 1 - Use SHT85 for temperature and humidity
#define USE_DATALOGGER 0 // 0 - No data logger/RTC, 1 - Log to SD card and use RTC
#define USE_CH4 1        // 0 - No CH4 sensor, 1 - Use CH4 sensor for methane measurements
#define USE_TEMP 0       // 0 - No temperature sensor, 1 - Use thermistor for temperature measurements

// -----------------------------------------------------------------------------
// Arduino Configuration
// -----------------------------------------------------------------------------
// Pins: A4, A5 - I2C communication and bus recovery
#define I2C_SDA_PIN A4 // SDA
#define I2C_SCL_PIN A5 // SCL
// Pins: D2, D3 - XBee Software Serial RX/TX
#if USE_XBEE
SoftwareSerial XBee(2, 3); // Arduino RX, TX (XBee Dout, Din)
#endif
// Pin D4 - Linear Actuator control
#define ACTUATOR_PIN 4
// Pin D7 - Solenoid control
#define SOLENOID_PIN 7
// Pin D10 - SPI Chip Select
#define SD_CARD_CS 10 // Chip Select for data logging SD card

#if USE_SHT85
// Set-up SHT85 sensor
#define SHT85_ADDRESS         0x44
SHT85 sht(SHT85_ADDRESS);
#endif

#if USE_CH4
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
#endif


// -----------------------------------------------------------------------------
// Logger Configuration
// -----------------------------------------------------------------------------
#if USE_XBEE
#define LOG_STREAM XBee
#define XBEE_BAUD_RATE 115200 // Use 115200 to minimize interference with actuator servo
#else
#define LOG_STREAM Serial
#define SERIAL_BAUD_RATE 9600
#endif

///////////////////////////////////////////////////////////////////
// Configuration and Setup
///////////////////////////////////////////////////////////////////
void setup(void)
{
  setupLogging(); // Initialize XBee or Serial communication, RTC, Datalogger
  setupI2c();
  setupK30();
  scanI2cBus();
  setupActuator();
  setupSht85();
}

///////////////////////////////////////////////////////////////////
// Start Sampling Loop
///////////////////////////////////////////////////////////////////
void loop()
{

  uint32_t m = millis(); // Arduino uptime in milliseconds since last reset, used for timestamping and timing of chamber opening/closing

  // Watchdog.reset(); // Must reset the watchdog at least every 8 seconds

  // Delay for the amount of time we want between readings
  // delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL)); // doesn't work with watchdog enabled
  // with watchdog enabled, manually set the delay between readings:
  delay(4000);
  // Watchdog.reset();
  delay(3000);
  // Watchdog.reset();
  // delay(4000);
  // Watchdog.reset();
  // delay(1495);
  // Watchdog.reset();

#if USE_DATALOGGER
  // Log RTC time to SD card file and print to serial monitor and XBee
  DateTime now = rtc.now(); // Current date and time from RTC for timestamping
  static char timebuf[48];
  snprintf(timebuf, sizeof(timebuf), "%lu, %lu, %04d/%02d/%02d %02d:%02d:%02d",
           m,
           now.unixtime(),
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());
  logfile.print(timebuf);
  LOG_STREAM.print(timebuf);
#else
  LOG_STREAM.print(m); // milliseconds since start
#endif

  // Watchdog.reset();

  // delay(4000);
  // Watchdog.reset();
  // delay(4000);
  //  Watchdog.reset();
  //  delay(4000);
  //  Watchdog.reset();
  delay(4000);
  // Watchdog.reset();

  // delay(16000); // these delays are necessary for K33

  // -----------------------------------------------------------------------------
  // CO2 Sensor Measurement with K30
  // -----------------------------------------------------------------------------
#if USE_K30
  int16_t co2Value = readK30_CO2_withRetry();

  if (co2Value == 0)
  {
#if DEBUG
    LOG_STREAM.println(F("DEBUG - CO2 read failed after retries"));
    LOG_STREAM.print(F("DEBUG - Recovering I2C bus..."));
#endif
    recoverI2CBus();
  }

#if USE_DATALOGGER
  logfile.print(", ");
  logfile.print(co2Value);
#endif
  LOG_STREAM.print(", ");
  LOG_STREAM.print(co2Value);

  // Watchdog.reset();
#endif

  // -----------------------------------------------------------------------------
  // CH4 Sensor Measurement
  // -----------------------------------------------------------------------------
#if USE_CH4
  CH4s = analogRead(CH4sens);   // read CH4 sensor output
  CH4smV = CH4s * (mV / steps); // convert pin reading to mV
  delay(10);                    // delay between reading of different analog pins
  Vbat = analogRead(Vb);        // read reference circuit
  VbatmV = Vbat * (mV / steps); // convert pin reading to mV
  delay(10);                    // delay between reading of different analog pins
#if USE_DATALOGGER
  logfile.print(", ");
  logfile.print(CH4smV);
  logfile.print(", ");
  logfile.print(VbatmV);
#endif
  LOG_STREAM.print(", ");
  LOG_STREAM.print(CH4smV);
  LOG_STREAM.print(", ");
  LOG_STREAM.print(VbatmV);

  // Watchdog.reset();
#endif

  // -----------------------------------------------------------------------------
  // Temperature and Humidity Measurement with SHT85
  // -----------------------------------------------------------------------------
#if USE_SHT85
  sht.read(); // read SHT85 sensor
  delay(10);

#if USE_DATALOGGER
  static char shtbuf[24];
  snprintf(shtbuf, sizeof(shtbuf), ", %.1f, %.1f, ", sht.getHumidity(), sht.getTemperature());
  logfile.print(shtbuf);
  LOG_STREAM.print(shtbuf);
#else
  LOG_STREAM.print(", ");
  LOG_STREAM.print(sht.getHumidity(), 1);
  LOG_STREAM.print(", ");
  LOG_STREAM.print(sht.getTemperature(), 1);
#endif
  // Watchdog.reset();
#endif

  // -----------------------------------------------------------------------------
  // Pressure Measurement with Thermistor and Differential Pressure Sensor
  // -----------------------------------------------------------------------------
#if USE_TEMP
  temp_sensor();
  // Watchdog.reset();
#endif

  // -----------------------------------------------------------------------------
  // Bubble Trap Control
  // -----------------------------------------------------------------------------
#if USE_ACTUATOR
  // Open or close the chamber with the linear actuator, if time
  bubbleTrap();
  // Watchdog.reset();
#endif

  // -----------------------------------------------------------------------------
  // XBee Command Handling
  // -----------------------------------------------------------------------------
#if USE_XBEE
  // Define XBee commands (optional, can help with testing/trouble-shooting)
  xbeeCommands();
  // Watchdog.reset();
#endif

#if USE_DATALOGGER
  logfile.println();
  // Write data to the SD card
  logfile.flush();
#endif
  LOG_STREAM.println();
}
