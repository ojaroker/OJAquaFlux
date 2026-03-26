// activateK30.ino
//
// Controls power to a relay that turns K30 on and off
//
// K30's activation is delayed at startup to prevent short circuit
// faults occurring on the solar charger
//
// Static helpers (HAS_K30_RELAY only):
//   k30RelayOn/Off()   — relay power control

#define K30_STARTUP_DELAY 5000UL // ms after Arduino boot before powering K30
#define K30_BOOT_DELAY_MS 2000UL // ms after relay HIGH before sensor is ready

// ---------------------------------------------------------------------------
// Relay helpers  (HAS_K30_RELAY only)
// ---------------------------------------------------------------------------
#if HAS_K30_RELAY
// Drive relay HIGH.  startupDelayMs is waited before asserting HIGH so the
// Arduino has time to stabilise on initial boot (pass 0 for mid-run cycles).
static void k30RelayOn(unsigned long startupDelayMs = K30_STARTUP_DELAY)
{
    if (startupDelayMs > 0)
        delay(startupDelayMs);
    digitalWrite(K30_RELAY_PIN, HIGH);
    delay(K30_BOOT_DELAY_MS); // allow K30 firmware to initialise
}

// Drive relay LOW immediately.
static void k30RelayOff()
{
    digitalWrite(K30_RELAY_PIN, LOW);
}
#endif // HAS_K30_RELAY

// ---------------------------------------------------------------------------
// activateK30()  — called once from setup()
// ---------------------------------------------------------------------------
void activateK30()
{
#if HAS_K30_RELAY
    // Always configure the relay pin as OUTPUT so it doesn't float.
    pinMode(K30_RELAY_PIN, OUTPUT);
    k30RelayOff();
    DEBUG_PRINTLN(F("DEBUG - K30 relay enabled and power is OFF"));
#else
    DEBUG_PRINTLN(F("DEBUG - K30 relay disabled"));
#endif // HAS_K30_RELAY

#if USE_K30
    DEBUG_PRINTLN(F("DEBUG - K30 CO2 sensor enabled"));

#if HAS_K30_RELAY
    DEBUG_PRINT(F("DEBUG - Turning on K30..."));
    k30RelayOn(); // K30_STARTUP_DELAY + K30_BOOT_DELAY_MS
    DEBUG_PRINTLN(F("Done"));
#endif

#else
    DEBUG_PRINTLN(F("DEBUG - K30 CO2 sensor disabled"));
#endif // USE_K30
}