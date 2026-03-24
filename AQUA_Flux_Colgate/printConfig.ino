// printConfig.ino
//
// Prints the build configuration and pin assignments to LOG_STREAM at startup.
// Called once from setup() in AQUA_Flux_Colgate.ino.

void printConfig()
{
  LOG_STREAM.print(F("=== AQUA-Flux ID: "));
  LOG_STREAM.print(aquaFluxId);
  LOG_STREAM.println(F(" ==="));
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
  xbeeHelp(); // Show available commands
#endif
  LOG_STREAM.println(F("==========================="));

  // List of data products: (1) milliseconds since Arduino was powered, (2) unique Unix stamp, (3) date and time, (4) [CO2] (ppm), (5) CH4 sensor output (mV),
  // (6) reference circuit output (mV), (7) relative humidity (%), (8) air temperature inside chamber (C), (9) AQUA-Flux ID
  LOG_STREAM.println(F("millis, stampunix, datetime, K30_CO2, CH4smV, Vbat, SHT_RH, SHT_temp, AQUA_Flux1"));
}
