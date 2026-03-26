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
//   openNextLogfile() - open or append to today's YYYYMMDD.CSV; write header if new
//   rotateLogfile()   - flush/close current file and open the next (called at midnight)
//   setupLogging()    - called once from setup(); initialises serial, RTC, and SD card

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
    error("Couldn't create file");

  currentLogDay = now.day();
  DEBUG_PRINT(F("Logging to: "));
  DEBUG_PRINTLN(filename);

  if (isNewFile)
  {
    // Write header only once per file; skip on same-day reboot (append mode)
    logfile.println(F("millis, stampunix, datetime, K30_CO2, CH4smV, Vbat, SHT_RH, SHT_temp, AQUA_Flux1"));
    logfile.flush(); // persist header immediately — first data row may not arrive for >30 s
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
    setRtcDate();
  }

  rtc.start(); // start the RTC

  // Validate the RTC date before opening any log file.
  // An implausible date (such as in the past) indicates a problem
  // Request operator enter a valid date and then reboot
  {
    DateTime rtcNow = rtc.now();
    if (!isRtcDateValid(rtcNow))
    {
      LOG_STREAM.println(F("The RTC has an implausible date. Reset date and reboot."));
      setRtcDate();
      error("Reboot to confirm RTC will maintain the correct date.");
    }
  }

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
  // Validate SD Card by writing a test file
  testSdCard();
  DEBUG_PRINTLN(F("card initialized."));

  openNextLogfile();

#else
  DEBUG_PRINTLN(F("DEBUG - SD card logging disabled"));
  DEBUG_PRINTLN(F("DEBUG - RTC disabled"));
#endif
}