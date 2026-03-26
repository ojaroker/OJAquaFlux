// checkK30ErrorStatus.ino
//
// Standalone diagnostic sketch — reads the K30 CO2 sensor error status
// register at RAM address 0x1E and prints a human-readable report over
// the USB Serial monitor.
//
// Run this sketch BEFORE uploading AQUA_Flux_Colgate to verify the K30
// is healthy and configured at the expected I2C address.
//
// Wiring: K30 SDA → A4, SCL → A5 (standard Arduino Uno I2C pins)
//
// I2C address:
//   0x68 — K30 factory default
//   0x69 — after running the address-change sketch (AQUA_Flux_Colgate uses 0x69)
//
// Reference: SenseAir K30 datasheet TDE4700, §8 Error Status

#include <Wire.h>

#define K30_ADDR 0x69 // change to 0x68 if address has not yet been changed

// Returns the sum of `count` bytes starting at `buf`, truncated to 8 bits.
static byte checkSum(byte *buf, byte count)
{
  byte sum = 0;
  while (count-- > 0)
    sum += *buf++;
  return sum;
}

void setup()
{
  // Serial.begin(9600);
  Wire.begin();
  delay(1000); // allow K30 to finish booting

  // --- Send Read RAM command ---
  // Command 0x22: read 2 bytes from RAM
  // Address:      0x001E (Error Status)
  byte cmd[4] = {0x22, 0x00, 0x1E, 0x00};
  cmd[3] = checkSum(cmd, 3); // checksum covers command + address bytes

  Wire.beginTransmission(K30_ADDR);
  for (size_t i = 0; i < sizeof(cmd); i++)
    Wire.write(cmd[i]);
  byte err = Wire.endTransmission();
  if (err != 0)
  {
    Serial.print(F("Transmission error: "));
    Serial.println(err);
    Serial.println(F("Check wiring and I2C address."));
    return;
  }

  delay(20); // K30 datasheet: ≥20 ms after write before reading

  // --- Read 4-byte response ---
  byte received = Wire.requestFrom(K30_ADDR, (uint8_t)4);
  if (received != 4)
  {
    Serial.print(F("Error: expected 4 bytes, got "));
    Serial.println(received);
    return;
  }

  byte buf[4];
  for (int i = 0; i < 4; i++)
    buf[i] = Wire.read();

  // Status byte: 0x21 = read complete, 0x22 = read incomplete
  if (buf[0] != 0x21)
  {
    Serial.print(F("Read incomplete, status: 0x"));
    Serial.println(buf[0], HEX);
    return;
  }

  // Verify checksum
  if ((buf[0] + buf[1] + buf[2]) != buf[3])
  {
    Serial.println(F("Checksum fail"));
    return;
  }

  uint16_t errorStatus = ((uint16_t)buf[1] << 8) | buf[2];
  Serial.print(F("Error Status (0x1E): 0x"));
  if (errorStatus < 0x1000)
    Serial.print(F("0"));
  if (errorStatus < 0x0100)
    Serial.print(F("0"));
  if (errorStatus < 0x0010)
    Serial.print(F("0"));
  Serial.println(errorStatus, HEX);

  if (errorStatus == 0x0000)
  {
    Serial.println(F("OK — no errors"));
  }
  else
  {
    // Bit definitions per TDE4700 §8 — verify against your firmware revision
    if (errorStatus & 0x0001)
      Serial.println(F("  [bit 0] Fatal error"));
    if (errorStatus & 0x0002)
      Serial.println(F("  [bit 1] Algorithm error"));
    if (errorStatus & 0x0004)
      Serial.println(F("  [bit 2] Output error"));
    if (errorStatus & 0x0008)
      Serial.println(F("  [bit 3] Self-diagnostic error"));
    if (errorStatus & 0x0010)
      Serial.println(F("  [bit 4] Out of range"));
    if (errorStatus & 0x0020)
      Serial.println(F("  [bit 5] Memory error"));
    // Bits 6–15: reserved — print raw value if any unknown bits are set
    if (errorStatus & 0xFFC0)
    {
      Serial.print(F("  [bits 6-15] Unknown flags set: 0x"));
      Serial.println(errorStatus & 0xFFC0, HEX);
    }
  }
}

void loop() {}
