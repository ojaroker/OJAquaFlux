/*
  Robust Arduino example for K-Series sensor (K-30, K-33, etc.)
  Based on code by Jason Berger, Co2meter.com  
  UART
*/

#include <SoftwareSerial.h>

SoftwareSerial K_30_Serial(12, 13);  // pin 12 = RX, pin 13 = TX

byte readCO2[] = {0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25};  // Command packet
byte response[7];  // Array to hold the response

// Multiplier for value. Default is 1. Set to 3 for K-30 3% and 10 for K-33 ICB
int valMultiplier = 1;

void setup() {
  Serial.begin(9600);         
  K_30_Serial.begin(9600);    
  Serial.println("starting");
}

void loop() {
  if (sendRequest(readCO2)) {
    unsigned long valCO2 = getValue(response);
    Serial.print("CO2 ppm = ");
    Serial.println(valCO2);
  } else {
    Serial.println("Error: no valid response");
  }
  delay(2000);
}

bool sendRequest(byte packet[]) {
  // Send the request
  K_30_Serial.write(readCO2, 7);

  // Wait until we see a header byte (0xFE) or timeout
  unsigned long start = millis();
  while (millis() - start < 500) {
    if (K_30_Serial.available()) {
      int b = K_30_Serial.read();
      if (b == 0xFE) {
        response[0] = (byte)b;
        // Read the next 6 bytes
        for (int i = 1; i < 7; i++) {
          while (!K_30_Serial.available()) {
            if (millis() - start > 500) return false; // timeout
          }
          response[i] = (byte)K_30_Serial.read();
        }
        break;
      }
    }
  }

  // Debug: print raw response
  Serial.print("Response: ");
  for (int i = 0; i < 7; i++) {
    Serial.print(response[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Validate header
  if (response[0] != 0xFE || response[1] != 0x44) {
    Serial.println("Invalid packet header");
    return false;
  }

  return true;
}


unsigned long getValue(byte packet[]) {
  int high = packet[3];  // high byte for value is 4th byte
  int low = packet[4];   // low byte for value is 5th byte
  unsigned long val = (high << 8) | low;  // combine high and low
  return val * valMultiplier;
}
