// SPDX-FileCopyrightText: 2023 Carter Nelson for Adafruit Industries
// SPDX-License-Identifier: MIT
//
// Modified to add:
//  - XBee output
//  - Watchdog timer
//  - Dual Serial/XBee printing

#include <Wire.h>
#include <SoftwareSerial.h>
#include <WDT.h>

// -----------------------------
// I2C bus
// -----------------------------
#define WIRE Wire

// -----------------------------
// XBee (RX, TX)
// -----------------------------
SoftwareSerial XBee(2, 3);

// -----------------------------
// Print helper (Serial + XBee)
// -----------------------------
void printBoth(const String &msg) {
  Serial.println(msg);
  XBee.println(msg);
}

void printBothInline(const String &msg) {
  Serial.print(msg);
  XBee.print(msg);
}

// -----------------------------
// SETUP
// -----------------------------
void setup() {

  Serial.begin(9600);
  XBee.begin(115200);

  delay(2000); // allow power rails to stabilize

  printBoth("\nI2C Scanner");

  // Enable Watchdog
  int countdownMS = WDT.begin(8000);
  printBoth("WDT enabled for " + String(countdownMS) + " ms");

  // Start I2C
  WIRE.begin();
  WIRE.setClock(100000);

  printBoth("Wire.begin() completed");
}

// -----------------------------
// LOOP
// -----------------------------
void loop() {

  WDT.refresh();

  byte error, address;
  int nDevices = 0;

  printBoth("Scanning...");

  for(address = 1; address < 127; address++)
  {
    WDT.refresh();

    WIRE.beginTransmission(address);
    error = WIRE.endTransmission();

    if (error == 0)
    {
      printBothInline("I2C device found at address 0x");

      if (address < 16) {
        Serial.print("0");
        XBee.print("0");
      }

      Serial.print(address, HEX);
      XBee.print(address, HEX);

      printBoth("  !");
      nDevices++;
    }
    else if (error == 4)
    {
      printBothInline("Unknown error at address 0x");

      if (address < 16) {
        Serial.print("0");
        XBee.print("0");
      }

      Serial.println(address, HEX);
      XBee.println(address, HEX);
    }
  }

  if (nDevices == 0)
    printBoth("No I2C devices found\n");
  else
    printBoth("done\n");

  delay(5000);   // wait 5 seconds for next scan
}