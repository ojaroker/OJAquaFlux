#include <Wire.h>
#include "SHT85.h"


#define SHT85_ADDRESS 0x44
int K30_ADDR = 0x68;


SHT85 sht(SHT85_ADDRESS);


int readCO2()
{
  byte buffer[4];

  Wire.beginTransmission(K30_ADDR);
  Wire.write(0x22);   // Read command
  Wire.write(0x00);
  Wire.write(0x08);
  Wire.write(0x2A);
  Wire.endTransmission();

  delay(10);

  Wire.requestFrom(K30_ADDR, 4);
  for (byte i = 0; i < 4; i++)
    buffer[i] = Wire.read();

  // Construct CO₂ value
  int co2_value = (buffer[1] << 8) | buffer[2];

  // Checksum check
  byte sum = buffer[0] + buffer[1] + buffer[2];
  if (sum == buffer[3])
    return co2_value;
  else
    return -1; // Fail
}

void setup()
{
  Serial.begin(9600);
  Wire.begin();

  sht.begin();
  Wire.setClock(100000);

  delay(500);
  Serial.println("SHT85 + K30 Combined Sensor Program");
}

void loop()
{
  sht.read();
  float tempC = sht.getTemperature();
  float humRH = sht.getHumidity();

  int co2ppm = readCO2();


  Serial.print("Temp(C): ");
  Serial.print(tempC, 2);
  Serial.print(" | Hum(%): ");
  Serial.print(humRH, 2);
  Serial.print(" | CO2(ppm): ");

  if (co2ppm > 0)
    Serial.println(co2ppm);
  else
    Serial.println("ERROR");

  delay(1000);  
}
