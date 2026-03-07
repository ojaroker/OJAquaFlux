// TODO
// Use LOG_STREAM instead of Serial and XBee for all logging, and remove redundant logging
// refactor 'open' and 'close' to use real time instead of seconds
//

// Set the timing when the chamber will open and close (here and lines 532 and 557)
unsigned long open = 600000; // 4 minutes open (milliseconds) (10 min = 600000)

// TODO: negative number is likely a bug
unsigned long close = -3000000; // 15 minutes closed (milliseconds) (50 min = 3000000)

// Define solenoid functions
// void solenoidOff() {
//     digitalWrite(SOLENOID_PIN, LOW);
// }

// void solenoidOn() {
//     digitalWrite(SOLENOID_PIN, HIGH);
// }

// Define linear actuator functions
#if USE_ACTUATOR
void extendActuator()
{
    actuator.writeMicroseconds(1850); // give the actuator a 2ms pulse to fully extend (1000us = 1ms), recommended to not reach full 2ms
}

void retractActuator()
{
    actuator.writeMicroseconds(1000); // 1ms pulse to fully retract, adjust the retraction time to compress the chamber gasket on the floating base
}
#endif
void bubbleTrap()
{
#if USE_ACTUATOR
    if (millis() - open >= 59.9 * 60 * 1000UL) // Change first number to minutes closed + minutes open - 0.1
    {
        open = millis();
        XBee.print("Opening");
        XBee.print(", ");
        logfile.print("Opening");
        logfile.print(", ");
        Serial.print("Opening");
        Serial.print(", ");
        extendActuator();
        delay(4000);
        // Watchdog.reset();
        delay(4000);
        // Watchdog.reset();
        delay(4000);
        // Watchdog.reset();
        delay(4000);
        // Watchdog.reset();
        delay(4000);
        // Watchdog.reset();
        delay(4000);
        // Watchdog.reset();
    }

    // Watchdog.reset();

    if (millis() - close >= 59.9 * 60 * 1000UL) // Change first number to minutes closed + minutes open - 0.1
    {
        close = millis();
        XBee.print("Closing");
        XBee.print(", ");
        logfile.print("Closing");
        logfile.print(", ");
        Serial.print("Closing");
        Serial.print(", ");
        retractActuator();
        delay(4000);
        // Watchdog.reset();
        delay(4000);
        // Watchdog.reset();
        delay(4000);
        // Watchdog.reset();
        delay(4000);
        // Watchdog.reset();
        delay(4000);
        // Watchdog.reset();
        delay(4000);
        // Watchdog.reset();
    }

    // Vent the bubble trap with the solenoid valve, if full (full mV may differ depending on size of cylinder)

    // Watchdog.reset();

    // if (PmV > 800) // set to 800 mV
    // {XBee.print("Venting");
    //  XBee.print(", ");
    //  logfile.print("Venting");
    //  logfile.print(", ");
    //  Serial.print("Venting");
    //  Serial.print(", ");
    //  solenoidOn();
    //  delay(5000); // vent/open for 5 seconds
    //  solenoidOff();
    // }
#endif
}