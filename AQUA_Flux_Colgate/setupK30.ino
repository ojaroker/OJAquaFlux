void setupK30() {
#if USE_K30

#if DEBUG
    LOG_STREAM.println(F("DEBUG - K30 CO2 sensor enabled"));
#endif
  // Power on K30
  // Wait for K30 boot up
#else

#if DEBUG
    LOG_STREAM.println(F("DEBUG - K30 CO2 sensor disabled"));
#endif

#endif
}