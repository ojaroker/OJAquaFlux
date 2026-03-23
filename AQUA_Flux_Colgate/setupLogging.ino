// setupLogging.ino
//
// Initializes all logging infrastructure at startup:
//   1. Opens the XBee serial link (USE_XBEE=1) or USB Serial (USE_XBEE=0)
//   2. Connects to the PCF8523 RTC and starts it; warns if time is not set
//   3. Initializes the SD card and opens a date-named CSV log file
//      (YYYYMMDD.CSV). A same-day reboot appends to the existing file without
//      duplicating the header. Halts with an error message if the SD card
//      or file cannot be opened.
//
// Functions:
//   error()          - print error message, flush, and halt (manual reboot required)
//   openNextLogfile() - open or append to today's YYYYMMDD.CSV; write header if new
//   rotateLogfile()  - flush/close current file and open the next (called at midnight)
//   setupLogging()   - called once from setup(); initialises serial, RTC, and SD card

void error(char *str) // Halt if error
{
  LOG_STREAM.print(F("ERROR OCCURRED: "));
  LOG_STREAM.println(str);
  LOG_STREAM.println(F("Execution halted. Please reboot manually."));
  // Flush serial and clear the receive buffer if Xbee (which is a no-op)
  LOG_STREAM.flush();
  while (1)
    ; // Halt
}

#if USE_DATALOGGER
// Opens YYYYMMDD.CSV for today's date (appends if it already exists, e.g. same-day
// reboot). Writes the CSV header only for new files. Calls error() on failure.
void openNextLogfile()
{
  DateTime now = rtc.now();
  char filename[13]; // "YYYYMMDD.CSV"
  snprintf(filename, sizeof(filename), "%04d%02d%02d.CSV",
           now.year(), now.month(), now.day());

  bool isNewFile = !SD.exists(filename);
  logfile = SD.open(filename, FILE_WRITE); // FILE_WRITE appends if file exists
  if (!logfile)
  {
    error("Couldn't create file");
  }

  currentLogDay = now.day();
  DEBUG_PRINT(F("Logging to: "));
  DEBUG_PRINTLN(filename);

  if (isNewFile)
  {
    // Write header only once per file; skip on same-day reboot (append mode)
    // List of data products: (1) milliseconds since Arduino was powered, (2) unique Unix stamp, (3) date and time, (4) [CO2] (ppm), (5) CH4 sensor output (mV),
    // (6) reference circuit output (mV), (7) relative humidity (%), (8) air temperature inside chamber (C), (9) AQUA-Flux ID
    logfile.println(F("millis, stampunix, datetime, K30_CO2, CH4smV, Vbat, SHT_RH, SHT_temp, AQUA_Flux1"));
  }
}

// Closes the current log file and opens the next one.
void rotateLogfile()
{
  logfile.flush();
  logfile.close();
  LOG_STREAM.println(F("Log file rotated."));
  openNextLogfile();
}
#endif // USE_DATALOGGER

void setupLogging(void)
{
#if USE_XBEE
  XBee.begin(XBEE_BAUD_RATE);
#else
  Serial.begin(SERIAL_BAUD_RATE);
#endif

#if USE_DATALOGGER

  DEBUG_PRINTLN(F("DEBUG - Data Logging and RTC enabled"));
  // Connect to RTC
  //  #ifndef ESP8266 //    -------------------OJ COMMENTED THIS OUT
  //   while (!Serial); // Wait for serial port to connect. Needed for native USB.
  // #endif

  if (!rtc.begin())
  {
    error("Couldn't find RTC");
  }

  if (!rtc.initialized() || rtc.lostPower())
  {
    LOG_STREAM.println(F("RTC is NOT initialized. Time Must Be Set"));
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

  // int countdownMS = Watchdog.enable(8000); // enable watchdog with timer of 8 seconds (max for Arduino Uno) - if the Arduino halts for more than 8 seconds, the watchdog will restart the sketch

  // Initialize the SD card
  DEBUG_PRINT(F("Initializing SD card..."));
  // Make sure that the default chip select pin is set to OUTPUT
  pinMode(SD_CARD_CS, OUTPUT);

  // See if the SD card is present and can be initialized:
  if (!SD.begin(SD_CARD_CS))
  {
    error("Card failed or not present");
  }
  DEBUG_PRINTLN(F("card initialized."));

  openNextLogfile();

#else
  DEBUG_PRINTLN(F("DEBUG - SD card logging disabled"));
  DEBUG_PRINTLN(F("DEBUG - RTC disabled"));
#endif
}