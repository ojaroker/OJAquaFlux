// K30 CO2 Sensor Setup
// See Readme for wiring instructions
// To overcome spurious short-circuit fault when powering K30 with relay,
// we wait to power K30 until after the Arduino has booted and stabilized.

#define K30_STARTUP_DELAY 5000 // Delay after arduino boot before powering K30 (milliseconds)

void setupK30()
{
#if HAS_K30_RELAY
  // Always configure the relay pin as OUTPUT so it doesn't float.
  DEBUG_PRINTLN(F("DEBUG - Setting "));
  pinMode(K30_RELAY_PIN, OUTPUT);
  digitalWrite(K30_RELAY_PIN, LOW);
#endif

#if USE_K30
  DEBUG_PRINTLN(F("DEBUG - K30 CO2 sensor enabled"));

#if HAS_K30_RELAY
  // Power on K30
  DEBUG_PRINT(F("DEBUG - Turning on K30..."));
  delay(K30_STARTUP_DELAY); // Wait for K30 to boot up
  digitalWrite(K30_RELAY_PIN, HIGH);
  delay(2000); // Wait for K30 to boot up
  DEBUG_PRINTLN(F("Done"));

#endif // HAS_K30_RELAY

  // TODO - Check "Error Status" at 0x1E (See section 8 of data sheet)
  // to detect sensor faults before continuing
  // K30 needs 60 seconds to boot up
  // K30 readings need 15 minutes to stabilize; discard first 15 minutes

#else

  DEBUG_PRINTLN(F("DEBUG - K30 CO2 sensor disabled"));

#endif // USE_K30
}