#include <SoftwareSerial.h>
#include <Servo.h>

// XBee pins
SoftwareSerial XBee(2, 3);  // RX = 2, TX = 3

// Actuator on pin 4
Servo actuator;

void setup() {
  Serial.begin(9600);
  XBee.begin(115200);   // match your XBee baud

  actuator.attach(4);
  actuator.writeMicroseconds(1500); // neutral starting position

  Serial.println("Actuator Test Ready. Send 'U' or 'D' via XBee.");
  XBee.println("Actuator Test Ready. Send 'U' or 'D'.");
}

void loop() {
  if (XBee.available()) {
    XBee.println("available");
    char c = XBee.read();
    XBee.println("just read: ");
    XBee.println(c);
    if (c == 'U') {
      Serial.println("Extending actuator...");
      XBee.println("Extending actuator...");
      actuator.writeMicroseconds(1850);  // extend
    }
    else if (c == 'D') {
      Serial.println("Retracting actuator...");
      XBee.println("Retracting actuator...");
      actuator.writeMicroseconds(1000);  // retract
    }
  } 
}
