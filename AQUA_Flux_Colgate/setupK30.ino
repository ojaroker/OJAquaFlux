// setupK30.ino
//
// K30 CO2 sensor setup: relay control, I2C address verification/change,
// and error status check.
//
// Static helpers (USE_K30 only):
//   k30ReadRAM()       — shared Wire read-RAM protocol
//   k30WriteEEPROM()   — shared Wire write-EEPROM protocol (for address change)
//   k30RelayOn/Off()   — relay power control (HAS_K30_RELAY only)
//   readK30I2CAddress()  — read configured address from RAM 0x0020 via 0x7F
//   ensureK30Address()   — verify address == K30_I2C_ADDR; write + power-cycle if not
//   checkK30ErrorStatus() — read error status register (RAM 0x1E); halt on fatal bits
//
// Reference: SenseAir K30 datasheet TDE4700, PSP110

#define K30_STARTUP_DELAY 5000  // ms after Arduino boot before powering K30
#define K30_BOOT_DELAY_MS 2000  // ms after relay HIGH before sensor is ready
#define K30_POWER_DOWN_MS 10000 // ms to wait after relay OFF before turning back on

// Shared error message buffer used by all helpers below.
#if USE_K30
static char k30errbuf[64];
#endif

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
// Wire primitives  (USE_K30 only)
// ---------------------------------------------------------------------------
#if USE_K30

// Send a Read-RAM command and return the 2-byte result in `value`.
// Uses the K30 I2C address in `i2cAddr` (pass 0x7F for "any sensor").
// Calls error() and halts on any protocol failure.
static void k30ReadRAM(uint8_t i2cAddr, uint16_t ramAddr, uint16_t &value)
{
  byte cmd[4];
  cmd[0] = 0x22;
  cmd[1] = (ramAddr >> 8) & 0xFF;
  cmd[2] = ramAddr & 0xFF;
  cmd[3] = cmd[0] + cmd[1] + cmd[2];

  Wire.beginTransmission(i2cAddr);
  for (uint8_t i = 0; i < sizeof(cmd); i++)
    Wire.write(cmd[i]);
  byte err = Wire.endTransmission();
  if (err != 0)
  {
    snprintf(k30errbuf, sizeof(k30errbuf),
             "K30 readRAM 0x%04X: transmission error %d", ramAddr, err);
    error(k30errbuf);
  }

  delay(20); // K30 datasheet: ≥20 ms after write before reading response

  byte received = Wire.requestFrom(i2cAddr, (uint8_t)4);
  if (received != 4)
  {
    snprintf(k30errbuf, sizeof(k30errbuf),
             "K30 readRAM 0x%04X: expected 4 bytes, got %d", ramAddr, received);
    error(k30errbuf);
  }

  byte buf[4];
  for (uint8_t i = 0; i < 4; i++)
    buf[i] = Wire.read();

  if (buf[0] != 0x21)
  {
    snprintf(k30errbuf, sizeof(k30errbuf),
             "K30 readRAM 0x%04X: read incomplete, status 0x%02X", ramAddr, buf[0]);
    error(k30errbuf);
  }

  if ((byte)(buf[0] + buf[1] + buf[2]) != buf[3])
  {
    snprintf(k30errbuf, sizeof(k30errbuf),
             "K30 readRAM 0x%04X: checksum fail", ramAddr);
    error(k30errbuf);
  }

  value = ((uint16_t)buf[1] << 8) | buf[2];
}

// Send a Write-EEPROM command to set one byte in K30 EEPROM.
// `i2cAddr` — target sensor (pass 0x7F for "any sensor").
// `eepromReg` — K30 EEPROM register (0x31 = I2C address).
// `val`       — byte value to write.
// Calls error() and halts on transmission failure.
static void k30WriteEEPROM(uint8_t i2cAddr, uint8_t eepromReg, uint8_t val)
{
  // 6-byte command: { 0xD0, eepromReg, 0x00, 0x00, val, checksum }
  // Checksum covers first 5 bytes.
  byte cmd[6];
  cmd[0] = 0xD0;
  cmd[1] = eepromReg;
  cmd[2] = 0x00;
  cmd[3] = 0x00;
  cmd[4] = val;
  cmd[5] = cmd[0] + cmd[1] + cmd[2] + cmd[3] + cmd[4];

  Wire.beginTransmission(i2cAddr);
  for (uint8_t i = 0; i < sizeof(cmd); i++)
    Wire.write(cmd[i]);
  byte err = Wire.endTransmission();
  if (err != 0)
  {
    snprintf(k30errbuf, sizeof(k30errbuf),
             "K30 writeEEPROM 0x%02X: transmission error %d", eepromReg, err);
    error(k30errbuf);
  }
}

// ---------------------------------------------------------------------------
// Address management
// ---------------------------------------------------------------------------

// Read the K30's currently configured I2C address from RAM 0x0020.
// Uses 0x7F so this works regardless of the sensor's current address.
static uint8_t readK30I2CAddress()
{
  uint16_t value = 0;
  k30ReadRAM(0x7F, 0x0020, value);
  return (uint8_t)(value & 0xFF);
}

// Verify the K30's configured address matches K30_I2C_ADDR.
// If it does not match:
//   - Writes K30_I2C_ADDR to EEPROM register 0x31 via 0x7F.
//   - HAS_K30_RELAY=1: power-cycles the relay, then re-reads to confirm.
//   - HAS_K30_RELAY=0: instructs the operator to power-cycle manually, then halts.
static void ensureK30Address()
{
  uint8_t cur = readK30I2CAddress();

  if (cur == K30_I2C_ADDR)
  {
    LOG_STREAM.print(F("K30 I2C address: 0x"));
    LOG_STREAM.print(cur, HEX);
    LOG_STREAM.println(F(" OK"));
    return;
  }

  LOG_STREAM.print(F("K30 I2C address mismatch: found 0x"));
  LOG_STREAM.print(cur, HEX);
  LOG_STREAM.print(F(", expected 0x"));
  LOG_STREAM.println(K30_I2C_ADDR, HEX);
  LOG_STREAM.println(F("Writing new address to K30 EEPROM..."));

  k30WriteEEPROM(0x7F, 0x31, K30_I2C_ADDR);

#if HAS_K30_RELAY
  LOG_STREAM.println(F("Power cycling K30 to apply address change..."));
  k30RelayOff();
  delay(K30_POWER_DOWN_MS);
  k30RelayOn(0); // no extra startup delay — Arduino is already stable

  cur = readK30I2CAddress();
  if (cur != K30_I2C_ADDR)
  {
    snprintf(k30errbuf, sizeof(k30errbuf),
             "K30 address not set after power cycle (got 0x%02X)", cur);
    error(k30errbuf);
  }
  LOG_STREAM.print(F("K30 I2C address set to 0x"));
  LOG_STREAM.println(cur, HEX);
#else
  LOG_STREAM.println(F("Power cycle the K30 manually to apply the address change,"));
  LOG_STREAM.println(F("then reboot the Arduino."));
  error("K30 requires manual power cycle to apply address change");
#endif
}

// ---------------------------------------------------------------------------
// Error status check
// ---------------------------------------------------------------------------

// Reads the K30 error status register (RAM 0x1E) via K30_I2C_ADDR.
// Logs a human-readable report. Halts on any set bit (all indicate sensor fault).
static void checkK30ErrorStatus()
{
  uint16_t status = 0;
  k30ReadRAM(K30_I2C_ADDR, 0x001E, status);

  LOG_STREAM.print(F("K30 error status (0x1E): 0x"));
  if (status < 0x1000)
    LOG_STREAM.print(F("0"));
  if (status < 0x0100)
    LOG_STREAM.print(F("0"));
  if (status < 0x0010)
    LOG_STREAM.print(F("0"));
  LOG_STREAM.print(status, HEX);

  if (status == 0x0000)
  {
    LOG_STREAM.println(F("...K30: OK"));
    return;
  }

  LOG_STREAM.println(F("...K30: FAILED"));

  // Bit definitions
  // https://rmtplusstoragesenseair.blob.core.windows.net/docs/publicerat/PSP110.pdf
  // Page 8
  if (status & 0x0001)
    LOG_STREAM.println(F("Fatal error (bit 0) Error Code 1"));

  if (status & 0x0002)
    LOG_STREAM.println(F("Offset regulation error (bit 1) Error Code 2"));

  if (status & 0x0004)
  {
    LOG_STREAM.println(F("Algorithm error (bit 2) Error Code 4"));
    LOG_STREAM.println(F("  Indicates wrong EEPROM configuration"));
    LOG_STREAM.println(F("  Check settings & reconfigure with software tools"));
  }

  if (status & 0x0008)
  {
    LOG_STREAM.println(F("Output error (bit 3) Error Code 8"));
    LOG_STREAM.println(F("  Detected errors during output signals calculation & generation"));
    LOG_STREAM.println(F("  Check connections & loads of outputs"));
  }

  if (status & 0x0010)
  {
    LOG_STREAM.println(F("Self-diagnostic error (bit 4) Error Code 16"));
    LOG_STREAM.println(F("  May indicate the need to zero calibration or sensor replacement"));
    LOG_STREAM.println(F("  Check detailed self-diagnostic status with software tools"));
  }

  if (status & 0x0020)
  {
    LOG_STREAM.println(F("Out of range (bit 5) Error Code 32"));
    LOG_STREAM.println(F("  Accompanies most of the other errors. Resets automatically."));
    LOG_STREAM.println(F("  Can also indicate overload or failures of sensors and inputs."));
    LOG_STREAM.println(F("  Perform CO2 background calibration. See documentation."));
  }

  if (status & 0x0040)
    LOG_STREAM.println(F("Memory error (bit 6) Error Code 64"));

  if (status & 0xFF80)
  {
    LOG_STREAM.print(F("Reserved: unknown flags 0x"));
    LOG_STREAM.println(status & 0xFF80, HEX);
  }

  error("K30 status check failed");
}
#endif // USE_K30

// ---------------------------------------------------------------------------
// setupK30()  — called once from setup()
// ---------------------------------------------------------------------------
void setupK30()
{
#if HAS_K30_RELAY
  // Always configure the relay pin as OUTPUT so it doesn't float.
  pinMode(K30_RELAY_PIN, OUTPUT);
  k30RelayOff();
#endif

#if USE_K30
  DEBUG_PRINTLN(F("DEBUG - K30 CO2 sensor enabled"));

#if HAS_K30_RELAY
  DEBUG_PRINT(F("DEBUG - Turning on K30..."));
  k30RelayOn(); // K30_STARTUP_DELAY + K30_BOOT_DELAY_MS
  DEBUG_PRINTLN(F("Done"));
#endif

  ensureK30Address();    // verify/set I2C address via 0x7F; power-cycle if needed
  checkK30ErrorStatus(); // confirm no fatal faults at the confirmed address
  // Note: K30 readings need ~15 minutes to stabilize after power-on.

#else
  DEBUG_PRINTLN(F("DEBUG - K30 CO2 sensor disabled"));
#endif // USE_K30
}
