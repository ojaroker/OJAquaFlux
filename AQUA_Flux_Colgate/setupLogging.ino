// Setup Logging

void error(char *str) // Halt if error
{
  LOG_STREAM.print(F("ERROR OCCURRED: "));
  LOG_STREAM.println(str);
  LOG_STREAM.println(F("Execution halted. Please reboot manually."));

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

#if DEBUG
  LOG_STREAM.println(F("DEBUG - Data Logging and RTC enabled"));
#endif
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
#if DEBUG
  LOG_STREAM.print(F("Initializing SD card..."));
#endif
  // Make sure that the default chip select pin is set to OUTPUT
  pinMode(SD_CARD_CS, OUTPUT);

  // See if the SD card is present and can be initialized:
  if (!SD.begin(SD_CARD_CS))
  {
    error("Card failed or not present");
  }
#if DEBUG
  LOG_STREAM.println(F("card initialized."));
#endif

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
#if DEBUG
  LOG_STREAM.print(F("Logging to: "));
  LOG_STREAM.println(filename);
#endif

#else
#if DEBUG
  LOG_STREAM.println(F("DEBUG - SD card logging disabled"));
  LOG_STREAM.println(F("DEBUG - RTC disabled"));
#endif
#endif
}