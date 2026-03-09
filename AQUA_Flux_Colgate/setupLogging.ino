

void error(char *str) // Halt if error
{
  Serial.print("File error: ");
  Serial.println(str);
  while (1)
    ; // Halt command
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
    Serial.println("Couldn't find RTC");
    XBee.println("Couldn't find RTC");
    Serial.flush();
    while (1)
      delay(10);
  }

  if (!rtc.initialized() || rtc.lostPower())
  {
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

  // int countdownMS = Watchdog.enable(8000); // enable watchdog with timer of 8 seconds (max for Arduino Uno) - if the Arduino halts for more than 8 seconds, the watchdog will restart the sketch

  // List of data products: (1) milliseconds since Arduino was powered, (2) unique Unix stamp, (3) date and time, (4) [CO2] (ppm), (5) CH4 sensor output (mV),
  // (6) reference circuit output (mV), (7) relative humidity (%), (8) air temperature inside chamber (C), (9) AQUA-Flux ID
  logfile.println("millis, stampunix, datetime, K30_CO2, CH4smV, Vbat, SHT_RH, SHT_temp, AQUA_Flux1");
  LOG_STREAM.println("millis, stampunix, datetime, K30_CO2, CH4smV, Vbat, SHT_RH, SHT_temp, AQUA_Flux1");

  // Initialize the SD card
  Serial.print("Initializing SD card...");
  XBee.print("Initializing SD card...");
  // Make sure that the default chip select pin is set to OUTPUT
  pinMode(SD_CARD_CS, OUTPUT);

  // See if the SD card is present and can be initialized:
  if (!SD.begin(SD_CARD_CS))
  {
    error("Card failed or not present");
  }
  Serial.println("card initialized.");
  XBee.println("card initialized.");

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

  Serial.print("Logging to: ");
  XBee.print("Logging to: ");
  Serial.println(filename);
  XBee.println(filename);
#else
#if DEBUG
  LOG_STREAM.println(F("DEBUG - SD card logging disabled"));
  LOG_STREAM.println(F("DEBUG - RTC disabled"));
#endif
#endif
}