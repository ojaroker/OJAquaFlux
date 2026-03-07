// Set the log interval (milliseconds between sensor measurements)
int LOG_INTERVAL = 10000; // If implementing watchdog (line 184), go to lines 321-328 to manually set the log interval

// Set-up the logging file
File logfile;

void error(char *str) // Halt if error
{
  Serial.print("File error: ");
  Serial.println(str);
  while(1); // Halt command
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
    LOG_STREAM.println(F("DEBUG - SD card logging enabled"));
#endif

    // Initialize the SD card
  Serial.print("Initializing SD card...");
  XBee.print("Initializing SD card...");
  // Make sure that the default chip select pin is set to OUTPUT
  pinMode(SD_CARD_CS, OUTPUT);
  
  // See if the SD card is present and can be initialized:
  if (!SD.begin(SD_CARD_CS)) {
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
#else
#if DEBUG
    LOG_STREAM.println(F("DEBUG - SD card logging disabled"));
#endif
#endif
}