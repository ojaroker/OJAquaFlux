// ══════════════════════════════════════════════════════════════════════════════
// recoverI2CBus()
//   Recovers a locked-up I2C bus by bit-banging up to 9 SCL pulses while SDA
//   is held high, then issuing a START followed by a STOP condition.
//
//   A lockup happens when the master resets (e.g. watchdog) mid-transaction:
//   the slave still holds SDA low waiting for more SCL pulses.  Nine clocks
//   are the worst-case number needed to clock out whatever byte the slave is
//   stuck on, regardless of which bit it was on when the master reset.
//
//   Reference: NXP UM10204 I2C-bus specification §3.1.16
// ══════════════════════════════════════════════════════════════════════════════
void recoverI2CBus() {

  // Release the Wire library before bit-banging the pins directly.
  Wire.end();

  // SCL driven by us; SDA must be INPUT_PULLUP so we can actually read the bus state
  pinMode(I2C_SCL_PIN, OUTPUT);
  pinMode(I2C_SDA_PIN, INPUT_PULLUP);
  delayMicroseconds(5);

  // Send 9 clock pulses to unstick any device holding SDA low
  for (uint8_t i = 0; i < 9; i++) {
    digitalWrite(I2C_SCL_PIN, HIGH);
    delayMicroseconds(5);
    // If SDA has gone high the slave has released the bus — we can stop early.
    if (digitalRead(I2C_SDA_PIN) == HIGH) break;
    digitalWrite(I2C_SCL_PIN, LOW);
    delayMicroseconds(5);
  }

  // Issue a STOP condition
  digitalWrite(I2C_SCL_PIN, LOW);   // ensure SCL is low before SDA manipulation
  delayMicroseconds(5);
  pinMode(I2C_SDA_PIN, OUTPUT);
  digitalWrite(I2C_SDA_PIN, LOW);   // SDA low (with SCL low — safe)
  delayMicroseconds(5);
  digitalWrite(I2C_SCL_PIN, HIGH);  // SCL high
  delayMicroseconds(5);
  digitalWrite(I2C_SDA_PIN, HIGH);  // SDA high while SCL high = STOP

  // Release both pins to INPUT_PULLUP before handing them back to Wire,
  // so Wire.begin() can cleanly take ownership without fighting an OUTPUT driver.
  pinMode(I2C_SCL_PIN, INPUT_PULLUP);
  pinMode(I2C_SDA_PIN, INPUT_PULLUP);
  delayMicroseconds(5);

  // Re-initialise Wire and restore reduced clock speed.
  Wire.begin();
  Wire.setClock(I2C_CLOCK_SPEED);

  LOG_STREAM.println(F("I2C bus initialized"));
}