// xbeeCommands.ino
//
// Processes single-character commands received over XBee.
//
// Commands:
//   'U' — force the chamber open   (extend actuator, enter OPENING state)
//   'D' — force the chamber closed  (retract actuator, enter CLOSING state)
//
// Commands are non-blocking: they trigger the actuator pulse and hand control
// to the ChamberActuator state machine. The 24 s transition completes over
// subsequent loop() iterations via chamberActuator.update().
//
// If USE_ACTUATOR=0, 'U' and 'D' return an error message instead of acting.
// Unknown commands are echoed back with a warning so the operator knows the
// byte was received.

void xbeeCommands()
{
#if USE_XBEE
    if (!XBee.available())
        return;

    char c = XBee.read();

    switch (c)
    {
    case 'U':
#if USE_ACTUATOR
        DEBUG_PRINTLN(F("XBee Command Received: forced OPEN"));
        chamberActuator.forceOpen();
#else
        LOG_STREAM.println(F("Unable to Open. Actuator is not enabled."));
#endif
        break;

    case 'D':
#if USE_ACTUATOR
        DEBUG_PRINTLN(F("XBee Command Received: forced CLOSE"));
        chamberActuator.forceClose();
#else
        LOG_STREAM.println(F("Unable to Close. Actuator is not enabled."));
#endif
        break;

    default:
        LOG_STREAM.print(F("XBee: unknown command: "));
        LOG_STREAM.println(c);
        LOG_STREAM.println(F("XBee: Available Commands: "));
        LOG_STREAM.println(F("XBee: 'U' - Force the chamber open"));
        LOG_STREAM.println(F("XBee: 'D' - Force the chamber closed"));
        break;
    }
#endif // USE_XBEE
}
