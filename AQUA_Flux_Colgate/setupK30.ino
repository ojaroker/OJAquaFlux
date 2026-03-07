void setupK30()
{
#if USE_K30

#if DEBUG
    LOG_STREAM.println(F("DEBUG - K30 CO2 sensor enabled"));
#endif
    // Power on K30
    // Wait for K30 boot up

    // CO2 Sensor
    // TODO - Check "Error Status" at 0x1E (See section 8 of data sheet)
    // to detect sensor faults before continuing
    // K30 needs 60 seconds to boot up
    // K30 readings need 15 minutes to stabilize; discard first 15 minutes

#else

#if DEBUG
    LOG_STREAM.println(F("DEBUG - K30 CO2 sensor disabled"));
#endif

#endif
}