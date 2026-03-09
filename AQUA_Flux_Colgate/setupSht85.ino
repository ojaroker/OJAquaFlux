void setupSht85()
{
#if USE_SHT85
#if DEBUG
    LOG_STREAM.println(F("DEBUG - SHT85 temp/humidity sensor enabled"));
#endif
    // If you want the Arduino to halt if the SHT85 sensor is not working
    // if (! sht.begin()) {
    //   Serial.println("Could not find SHT");
    //   XBee.println("Could not find SHT");
    //   while (1) delay(10);
    // }
    // Serial.println("SHT85 found");
    // XBee.println("SHT85 found");
    sht.begin(); // start SHT85 sensor
#else
#if DEBUG
    LOG_STREAM.println(F("DEBUG - SHT85 temp/humidity sensor disabled"));
#endif
#endif
}