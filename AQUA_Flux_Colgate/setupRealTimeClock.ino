// Define the Real Time Clock (RTC) object
RTC_PCF8523 rtc; 

void setupRealTimeClock() {
#if USE_RTC
#if DEBUG
    LOG_STREAM.println(F("DEBUG - RTC enabled"));
#endif
  // Connect to RTC
//  #ifndef ESP8266 //    -------------------OJ COMMENTED THIS OUT
//   while (!Serial); // Wait for serial port to connect. Needed for native USB.
// #endif

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    XBee.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (! rtc.initialized() || rtc.lostPower()) {
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

#else
#if DEBUG
    LOG_STREAM.println(F("DEBUG - RTC disabled"));
#endif
#endif
}