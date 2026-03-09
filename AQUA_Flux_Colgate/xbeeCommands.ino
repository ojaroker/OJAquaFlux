void xbeeCommands()
{

#if USE_XBEE
    if (XBee.available())
    {
        char c = XBee.read();
        LOG_STREAM.print(F("DEBUG - XBee command received: "));
        LOG_STREAM.println(c);
        LOG_STREAM.println(F("WARNING: XBee command received but no command handler is implemented."));
        LOG_STREAM.println(F("TODO: refactor the mess in xbeeCommands()"));
    }
#endif
    // Early Return
    return;

    // TODO - Refactor this mess
    if (XBee.available())
    {
        char c = XBee.read();
        if (c == 'U') // "U" for "up" (command to open the chamber when XBee receives "U")
        {
            XBee.print("XBee Opening");
            XBee.print(", ");
            logfile.print("XBee Opening");
            logfile.print(", ");
            Serial.print("XBee Opening");
            Serial.print(", ");
#if USE_ACTUATOR
            extendActuator();
#else
            LOG_STREAM.println(F("XBee command received to open chamber, but actuator is disabled"));
#endif
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
        if (c == 'D') // "D" for "down" (command to close the chamber when XBee receives "D")
        {
            XBee.print("XBee Closing");
            XBee.print(", ");
            logfile.print("XBee Closing");
            logfile.print(", ");
            Serial.print("XBee Closing");
            Serial.print(", ");
#if USE_ACTUATOR
            retractActuator();
#else
            LOG_STREAM.println(F("XBee command received to close chamber, but actuator is disabled"));
#endif
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
        //   if (c == 'Y') // "Y" for "yes" (command to open the solenoid when XBee receives "Y")
        //   {
        //     XBee.print("SolenoidOn");
        //     XBee.print(", ");
        //     logfile.print("SolenoidOn");
        //     logfile.print(", ");
        //     Serial.print("SolenoidOn");
        //     Serial.print(", ");
        //     solenoidOn();
        //   }
        //   if (c == 'N') // "N" for "no" (command to close the solenoid when XBee receives "N")
        //   {
        //     XBee.print("SolenoidOff");
        //     XBee.print(", ");
        //     logfile.print("SolenoidOff");
        //     logfile.print(", ");
        //     Serial.print("SolenoidOff");
        //     Serial.print(", ");
        //     solenoidOff();
        //   }
    }
}