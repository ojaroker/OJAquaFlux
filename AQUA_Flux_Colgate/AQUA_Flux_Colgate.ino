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

// Milliseconds between sensor measurements.
// Minimum: 3 seconds, limited by K30 measurment period
// Maximum: Determined by CHAMBER_CLOSED_MS
unsigned long LOG_INTERVAL = 10000UL;

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

// I2C bus configuration
// Lower I2C clock to 50 kHz — more reliable on long wires than the 100 kHz default
#define I2C_CLOCK_SPEED 50000UL
#define I2C_TIMEOUT_MS 50UL // Timeout for I2C reads (prevents infinite loop if sensor stops clocking)

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
uint8_t currentLogDay = 0; // RTC day of the current log file; rotate when it changes
RTC_PCF8523 rtc;
File logfile; // Set-up the logging file
#endif

// -----------------------------------------------------------------------------
// AQUA-Flux Unit ID
// -----------------------------------------------------------------------------
// Logged as the last CSV column. Set at runtime via XBee 'I' command.
uint8_t aquaFluxId = 1;

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

// -----------------------------------------------------------------------------
// Chamber Actuator State Machine
// Must be included after LOG_STREAM and DEBUG macros are defined, and before
// the ChamberActuator global instance is declared below.
// -----------------------------------------------------------------------------
#include "chamber.h"

// -----------------------------------------------------------------------------
// Actuator Configuration
// -----------------------------------------------------------------------------
#if USE_ACTUATOR
Servo actuator;                  // servo object used by ChamberActuator
ChamberActuator chamberActuator; // state machine instance
#endif

///////////////////////////////////////////////////////////////////
// Configuration and Setup
///////////////////////////////////////////////////////////////////
void setup(void)
{
  setupLogging(); // Initialize XBee or Serial communication, RTC, Datalogger
  printConfig();
  setupI2c();
  setupK30();
  scanI2cBus();
  setupSht85();
#if USE_ACTUATOR
  chamberActuator.begin();
#else
  DEBUG_PRINTLN(F("DEBUG - Linear actuator disabled"));
#endif
  }

///////////////////////////////////////////////////////////////////
// Start Sampling Loop
///////////////////////////////////////////////////////////////////
void loop()
{

  //
  // NO LOGGING -- LOOP RETURNS EARLY
  // - When actuator in opening/closing state
  // - Suspended via xbee 'S' command
  //
#if USE_ACTUATOR
  // Advance the state machine on every iteration. This must run before any
  // delay so that CLOSING/OPENING transitions are detected promptly.
  chamberActuator.update();

  if (chamberActuator.isTransitioning())
  {
    // During the 24 s actuator travel we skip the 7 s sensor delays and all
    // sensor reads — there is no useful data while the chamber is moving.
    // Print a dot once per second so the log shows the program is still running.
    // XBee commands are still processed so the device remains responsive.
    static unsigned long lastDotMs = 0;
    if (millis() - lastDotMs >= 1000)
    {
      LOG_STREAM.print('.');
      lastDotMs = millis();
    }
#if USE_XBEE
    xbeeCommands();
#endif
    // Restart the loop while Chamber is opening or closing
    // Only take measurements when in Open or Closed state
    return;
  }
#endif //USE_ACTUATOR

  // While suspended via the 'S' XBee command, skip all sensor reads and delays.
  // Only process XBee commands so 'R' can be received to resume.
#if USE_XBEE
  if (xbeeSuspended)
  {
    xbeeCommands();
    return;
  }
#endif

  //
  // SENSOR MEASUREMENT AND LOGGING -- NORMAL LOOPING
  // - Normal Operation
  //
  interruptibleWait(LOG_INTERVAL); // xbee commands handled during wait
  uint32_t m = millis(); // Arduino uptime in milliseconds since last reset

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

  // -----------------------------------------------------------------------------
  // CO2 Sensor Measurement with K30
  // -----------------------------------------------------------------------------
#if USE_K30
  int16_t co2Value = readK30_CO2_withRetry();

  // K30 locks the I2C bus periodically, creating problems with RTC communication
  // Only option is robust error handling to reset and retry
  if (co2Value == 0)
  {
    DEBUG_PRINTLN(F("DEBUG - CO2 read failed after retries"));
    DEBUG_PRINT(F("DEBUG - Recovering I2C bus..."));
    recoverI2CBus();
    // Terminate the partial CSV row (timestamp already written above) with a
    // recovery marker so the next iteration starts on a clean line. Then return
    // immediately — this skips the rotation check, which uses the `now` captured
    // while the bus was locked and may contain corrupt date values.
    LOG_STREAM.println(F(", <<<I2C_RECOVERY>>>"));
#if USE_DATALOGGER
    logfile.println(F(", <<<I2C_RECOVERY>>>"));
    logfile.flush();
#endif
    return;
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
  snprintf(shtbuf, sizeof(shtbuf), ", %.1f, %.1f", sht.getHumidity(), sht.getTemperature());
  LOG_STREAM.print(shtbuf);

#if USE_DATALOGGER
  logfile.print(shtbuf);
#endif

#endif

  // -----------------------------------------------------------------------------
  // Pressure Measurement with Thermistor and Differential Pressure Sensor
  // -----------------------------------------------------------------------------
#if USE_TEMP
  temp_sensor();
#endif

  // -----------------------------------------------------------------------------
  // AQUA-Flux Unit ID
  // -----------------------------------------------------------------------------
  LOG_STREAM.print(F(", "));
  LOG_STREAM.print(aquaFluxId);
#if USE_DATALOGGER
  logfile.print(F(", "));
  logfile.print(aquaFluxId);
#endif

  // -----------------------------------------------------------------------------
  // Terminate Logging Line and Flush Data
  // -----------------------------------------------------------------------------
  LOG_STREAM.println();

#if USE_DATALOGGER
  logfile.println();
  // Write data to the SD card
  logfile.flush();
  // Rotate to a new log file at midnight (date change).
  // Guard with isRtcDateValid(): if the RTC returned garbage (e.g. day=45),
  // skip rotation and continue logging to the already-open file
  if (isRtcDateValid(now) && now.day() != currentLogDay)
  {
    rotateLogfile();
  }
#endif
}
