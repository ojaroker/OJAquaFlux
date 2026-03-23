// Arduino code for AQUA-Flux
// Colgate University

#include <RTClib.h>         // for real time clock (RTC)
#include <SD.h>             // for SD card
#include <SPI.h>            // to communicate with peripheral devices
#include <SoftwareSerial.h> // to communicate with XBee
#include <Wire.h>           // for I2C communication

#include "SHT85.h" // for SHT85 sensor
// #include <Adafruit_SleepyDog.h> // for watchdog on R3
#include <Servo.h> // for linear actuator
#include <WDT.h>   //for watchdog on R4

#define DEBUG 1

// -----------------------------------------------------------------------------
// Subsystem Enabling
// -----------------------------------------------------------------------------
#define USE_XBEE 1       // 0 - No XBee, Log to Serial; 1 - Use XBee for logging
#define USE_K30 1        // 0 - No K30, 1 - Use K30 for CO2 measurements
#define USE_ACTUATOR 1   // 0 - No actuator, 1 - Use linear actuator to open/close chamber
#define USE_SHT85 1      // 0 - No SHT85, 1 - Use SHT85 for temperature and humidity
#define USE_DATALOGGER 1 // 0 - No data logger/RTC, 1 - Log to SD card and use RTC
#define USE_CH4 1        // 0 - No CH4 sensor, 1 - Use CH4 sensor for methane measurements
#define USE_TEMP 0       // 0 - No temperature sensor, 1 - Use thermistor for temperature measurements
#define HAS_K30_RELAY 1  // 0 - K30 hardwired to 12VDC, 1 - Relay used to turn on K30

// -----------------------------------------------------------------------------
// Arduino Configuration
// -----------------------------------------------------------------------------
// Pins: A0, A1, A2 - CH4 sensor sensor
#define CH4_SENS A0 // CH4 sensor
#define CH4_VB A1   // CH4 sensor reference circuit
#define P A2        // differential pressure sensor
// Pin: A3 UNUSED - reserved for future use
// Pins: A4, A5 - I2C communication and bus recovery
#define I2C_SDA_PIN A4 // SDA
#define I2C_SCL_PIN A5 // SCL
// Pin D0, D1 - UNUSED - reserved for future use
// Pins: D2, D3 - XBee Software Serial RX/TX
#if USE_XBEE
SoftwareSerial XBee(2, 3); // Arduino RX, TX (XBee Dout, Din)
#endif
// Pin D4 - Linear Actuator control
#define ACTUATOR_PIN 4
// Pins D5, D6 - UNUSED - reserved for future use
// Pin D7 - Solenoid control
#define SOLENOID_PIN 7
// Pins D8, D9 - UNUSED - reserved for future use
// Pin D10 - SPI Chip Select
#define SD_CARD_CS 10 // Chip Select for data logging SD card
// Pin D11 - K30 Power-interrupting Relay
#define K30_RELAY_PIN \
  11 // K30 turned on after delay to prevent spurious short-circuit fault

// -----------------------------------------------------------------------------
// SHT85 Humidity and Temperature Sensor Configuration
// -----------------------------------------------------------------------------
#if USE_SHT85
// Set-up SHT85 sensor
#define SHT85_ADDRESS 0x44
SHT85 sht(SHT85_ADDRESS);
#endif

// -----------------------------------------------------------------------------
// Methane Sensor Configuration
// -----------------------------------------------------------------------------
#if USE_CH4
// Define the Arduino analog pins that connect to the CH4 and pressure sensors
int CH4s = 0;
int Vbat = 0;
// int Press = 0;
float CH4smV = 0;
float VbatmV = 0;
// float PmV = 0;
float mV = 5000;    // voltage (5V)
float steps = 1024; // steps for ADC
#endif

// -----------------------------------------------------------------------------
// Datalogger Configuration
// -----------------------------------------------------------------------------
#if USE_DATALOGGER
// Set the log interval (milliseconds between sensor measurements)
int LOG_INTERVAL = 10000; // If implementing watchdog (line 184), go to lines
                          // 321-328 to manually set the log interval
uint8_t currentLogDay = 0; // RTC day of the current log file; rotate when it changes
RTC_PCF8523 rtc;
File logfile; // Set-up the logging file
#endif

// -----------------------------------------------------------------------------
// Actuator Configuration
// -----------------------------------------------------------------------------
#if USE_ACTUATOR
Servo actuator; // Create a servo object named "actuator"
#endif

// -----------------------------------------------------------------------------
// Logger Configuration
// -----------------------------------------------------------------------------
#if USE_XBEE
#define LOG_STREAM XBee
#define XBEE_BAUD_RATE \
  115200 // Use 115200 to minimize interference with actuator servo
#else
#define LOG_STREAM Serial
#define SERIAL_BAUD_RATE 9600
#endif

// -----------------------------------------------------------------------------
// Debug Macros
//
// Usage:  DEBUG_PRINT(F("msg"));  DEBUG_PRINTLN(F("msg"));
//
// When DEBUG=1, expands to LOG_STREAM.print/println — same stream used
// everywhere else (XBee or Serial depending on USE_XBEE).
//
// When DEBUG=0, expands to do{}while(0) — a standard C idiom for a safe
// no-op that behaves as a single statement in all contexts. Without it,
// using the macro in a bare if/else (no braces) would cause the else to
// attach to the wrong if, producing a silent logic bug:
//
//   if (condition)
//     DEBUG_PRINTLN(F("x"));  // expands to: if(0) println(...);
//   else                      // else attaches here, not to outer if!
//     doSomething();
//
// do{}while(0) forces the expansion to be a single statement, so the
// caller must add a semicolon and the else attaches correctly.
// -----------------------------------------------------------------------------
#if DEBUG
#define DEBUG_PRINT(x) LOG_STREAM.print(x)
#define DEBUG_PRINTLN(x) LOG_STREAM.println(x)
#else
// clang-format off
#define DEBUG_PRINT(x)   do {} while (0)
#define DEBUG_PRINTLN(x) do {} while (0)
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

  LOG_STREAM.println(F("=== Build Configuration ==="));
  LOG_STREAM.print(F("  XBee:        "));
  LOG_STREAM.println(USE_XBEE ? F("enabled") : F("disabled"));
  LOG_STREAM.print(F("  K30 CO2:     "));
  LOG_STREAM.println(USE_K30 ? F("enabled") : F("disabled"));
  LOG_STREAM.print(F("  K30 relay:   "));
  LOG_STREAM.println(HAS_K30_RELAY ? F("enabled") : F("disabled"));
  LOG_STREAM.print(F("  Actuator:    "));
  LOG_STREAM.println(USE_ACTUATOR ? F("enabled") : F("disabled"));
  LOG_STREAM.print(F("  SHT85:       "));
  LOG_STREAM.println(USE_SHT85 ? F("enabled") : F("disabled"));
  LOG_STREAM.print(F("  Datalogger:  "));
  LOG_STREAM.println(USE_DATALOGGER ? F("enabled") : F("disabled"));
  LOG_STREAM.print(F("  CH4 sensor:  "));
  LOG_STREAM.println(USE_CH4 ? F("enabled") : F("disabled"));
  LOG_STREAM.print(F("  Thermistor:  "));
  LOG_STREAM.println(USE_TEMP ? F("enabled") : F("disabled"));
  LOG_STREAM.println(F("=== Pin Assignments ==="));
  LOG_STREAM.print(F("  CH4 sensor:  A"));
  LOG_STREAM.println(CH4_SENS - A0);
  LOG_STREAM.print(F("  CH4 Vbat:    A"));
  LOG_STREAM.println(CH4_VB - A0);
  LOG_STREAM.print(F("  Pressure:    A"));
  LOG_STREAM.println(P - A0);
  LOG_STREAM.print(F("  Actuator:    D"));
  LOG_STREAM.println(ACTUATOR_PIN);
  LOG_STREAM.print(F("  Solenoid:    D"));
  LOG_STREAM.println(SOLENOID_PIN);
  LOG_STREAM.print(F("  SD card CS:  D"));
  LOG_STREAM.println(SD_CARD_CS);
  LOG_STREAM.print(F("  K30 relay:   D"));
  LOG_STREAM.println(K30_RELAY_PIN);
#if USE_XBEE
  LOG_STREAM.print(F("  XBee baud:   "));
  LOG_STREAM.println(XBEE_BAUD_RATE);
#endif
  LOG_STREAM.println(F("==========================="));

  // List of data products: (1) milliseconds since Arduino was powered, (2) unique Unix stamp, (3) date and time, (4) [CO2] (ppm), (5) CH4 sensor output (mV),
  // (6) reference circuit output (mV), (7) relative humidity (%), (8) air temperature inside chamber (C), (9) AQUA-Flux ID
  LOG_STREAM.println(F("millis, stampunix, datetime, K30_CO2, CH4smV, Vbat, SHT_RH, SHT_temp, AQUA_Flux1"));
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
    DEBUG_PRINTLN(F("DEBUG - CO2 read failed after retries"));
    DEBUG_PRINT(F("DEBUG - Recovering I2C bus..."));
    recoverI2CBus();
  }

  LOG_STREAM.print(", ");
  LOG_STREAM.print(co2Value);

#if USE_DATALOGGER
  logfile.print(", ");
  logfile.print(co2Value);
#endif

  // Watchdog.reset();
#endif

  // -----------------------------------------------------------------------------
  // CH4 Sensor Measurement
  // -----------------------------------------------------------------------------
#if USE_CH4
  CH4s = analogRead(CH4_SENS);  // read CH4 sensor output
  CH4smV = CH4s * (mV / steps); // convert pin reading to mV
  delay(10);                    // delay between reading of different analog pins
  Vbat = analogRead(CH4_VB);    // read reference circuit
  VbatmV = Vbat * (mV / steps); // convert pin reading to mV
  delay(10);                    // delay between reading of different analog pins

  static char ch4buf[24];
  snprintf(ch4buf, sizeof(ch4buf), ", %.1f, %.1f", CH4smV, VbatmV);
  LOG_STREAM.print(ch4buf);

#if USE_DATALOGGER
  logfile.print(ch4buf);
#endif

  // Watchdog.reset();
#endif

  // -----------------------------------------------------------------------------
  // Temperature and Humidity Measurement with SHT85
  // -----------------------------------------------------------------------------
#if USE_SHT85
  sht.read(); // read SHT85 sensor
  delay(10);

  static char shtbuf[24];
  snprintf(shtbuf, sizeof(shtbuf), ", %.1f, %.1f, ", sht.getHumidity(), sht.getTemperature());
  LOG_STREAM.print(shtbuf);

#if USE_DATALOGGER
  logfile.print(shtbuf);
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
  // Actuator Control
  // -----------------------------------------------------------------------------
#if USE_ACTUATOR
  // Open or close the chamber with the linear actuator, if time
  chamber();
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

  // -----------------------------------------------------------------------------
  // Terminate Logging Line and Flush Data
  // -----------------------------------------------------------------------------
  LOG_STREAM.println();

#if USE_DATALOGGER
  logfile.println();
  // Write data to the SD card
  logfile.flush();
  // Rotate to a new log file at midnight (date change)
  if (now.day() != currentLogDay)
  {
    rotateLogfile();
  }
#endif
}
