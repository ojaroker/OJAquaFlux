// setupLogging.ino
//
// Initializes all logging infrastructure at startup:
//   1. Opens the XBee serial link (USE_XBEE=1) or USB Serial (USE_XBEE=0)
//   2. Connects to the PCF8523 RTC and starts it; warns if time is not set
//   3. Initializes the SD card and creates a new sequentially-numbered CSV
//      log file (LOGGE0XX.CSV). Halts with an error message if the SD card
//      or file cannot be opened.
//
// error() - logs a message and halts, requiring a manual reboot.

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

  // Create a new CSV file on the SD card
  char filename[] = "LOGGER00.CSV";
  for (uint16_t i = 0; i < 100; i++)
  {
    filename[5] = i / 100 + '0';
    filename[6] = (i % 100) / 10 + '0';
    filename[7] = i % 10 + '0';
    if (!SD.exists(filename))
    {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE);
      break; // leave the loop!
    }
  }

  if (!logfile)
  {
    error("Couldn't create file");
  }
  DEBUG_PRINT(F("Logging to: "));
  DEBUG_PRINTLN(filename);

#else
  DEBUG_PRINTLN(F("DEBUG - SD card logging disabled"));
  DEBUG_PRINTLN(F("DEBUG - RTC disabled"));
#endif
}