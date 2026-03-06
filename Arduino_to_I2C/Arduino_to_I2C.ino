// CO2 Meter K-series Example Interface
// Sends identical output to Serial Monitor and XBee

#include <Wire.h>
#include <SoftwareSerial.h>

// XBee on pins 2 (RX), 3 (TX)
SoftwareSerial XBee(2, 3);

// Default I2C address of K-series CO2 sensor
int co2Addr = 0x69;

void setup() {

  Serial.begin(9600);
  XBee.begin(115200);     
  Wire.begin();

  pinMode(13, OUTPUT);

  Serial.println("Application Note AN-102: Interface Arduino to K-30");
  XBee.println("Application Note AN-102: Interface Arduino to K-30");
}

///////////////////////////////////////////////////////////////////
// readCO2()
// Returns CO2 value on success, 0 on checksum failure
///////////////////////////////////////////////////////////////////

int readCO2()
{
  int co2_value = 0;

  digitalWrite(13, HIGH);  // LED on

  // ---- Write request to sensor ----
  Wire.beginTransmission(co2Addr);
  delay(1);
  Wire.write(0x22);
  delay(1);
  Wire.write(0x00);
  delay(1);
  Wire.write(0x08);
  delay(1);
  Wire.write(0x2A);
  delay(1);
  Wire.endTransmission();

  delay(50); // sensor processing time

  // ---- Read response ----
  Wire.requestFrom(co2Addr, 4);
  delay(5);
  byte i = 0;
  byte buffer[4] = {0, 0, 0, 0};

  while (Wire.available())
  {
    buffer[i] = Wire.read();
    i++;
    delay(1);
  }

  // Convert to CO2 value
  co2_value = buffer[1];
  co2_value <<= 8;
  co2_value |= buffer[2];

  // Checksum
  byte sum = buffer[0] + buffer[1] + buffer[2];

  digitalWrite(13, LOW); // LED off

  if (sum == buffer[3])
    return co2_value;
  else
    return 0;
}

///////////////////////////////////////////////////////////////////
// Main loop
///////////////////////////////////////////////////////////////////

void loop() {

  int co2Value = readCO2();

  if (co2Value > 0)
  {
    Serial.print("CO2 Value: ");
    Serial.println(co2Value);

    XBee.print("CO2 Value: ");
    XBee.println(co2Value);
  }
  else
  {
    Serial.println("Checksum failed / Communication failure");
    XBee.println("Checksum failed / Communication failure");
  }

  delay(2000); // Read every 2 seconds
}