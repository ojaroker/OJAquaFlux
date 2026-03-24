// xbeeCommands.ino
//
// Processes single-character commands received over XBee.
//
// Commands:
//   'O' / 'o' — force the chamber open   (extend actuator, enter OPENING state)
//   'C' / 'c' — force the chamber closed  (retract actuator, enter CLOSING state)
//   'D' / 'd' — set RTC date and time interactively
//   'S' / 's' — suspend: flush and close log file; notify operator safe to power off
//   'R' / 'r' — resume: reopen log file and restart the sampling loop
//
// 'S' sets the global xbeeSuspended flag. While suspended, loop() skips all
// sensor reads and delays, calling only xbeeCommands() until 'R' is received.
// Unknown commands echo the received byte and print the help list.

// Global flag — true while the unit is suspended via the 'S' command.
// Checked in loop() to skip sensor reads until 'R' resumes operation.
bool xbeeSuspended = false;

void xbeeHelp()
{
    LOG_STREAM.println(F("===== XBee Commands ====="));
    LOG_STREAM.println(F(" 'O' - Force chamber Open"));
    LOG_STREAM.println(F(" 'C' - Force chamber Closed"));
    LOG_STREAM.println(F(" 'D' - Set RTC Date and Time"));
    LOG_STREAM.println(F(" 'S' - Suspend (safe power-off)"));
    LOG_STREAM.println(F(" 'R' - Resume operation"));
    LOG_STREAM.println(F("========================="));
}

void xbeeCommands()
{
#if USE_XBEE
    if (!XBee.available())
        return;

    char c = XBee.read();

    switch (c)
    {
    // OPEN
    case 'o':
    case 'O':
#if USE_ACTUATOR
        DEBUG_PRINTLN(F("XBee Command Received: forced OPEN"));
        chamberActuator.forceOpen();
#else
        LOG_STREAM.println(F("Unable to Open. Actuator is not enabled."));
#endif
        break;

    // CLOSE
    case 'c':
    case 'C':
#if USE_ACTUATOR
        DEBUG_PRINTLN(F("XBee Command Received: forced CLOSE"));
        chamberActuator.forceClose();
#else
        LOG_STREAM.println(F("Unable to Close. Actuator is not enabled."));
#endif
        break;

    // SET DATE
    case 'd':
    case 'D':
#if USE_DATALOGGER
        setRtcDate();
#else
        LOG_STREAM.println(F("Data Logger is not enabled."));
#endif
        break;

    // SUSPEND — flush log, notify operator it is safe to power off
    case 's':
    case 'S':
#if USE_DATALOGGER
        logfile.flush();
        logfile.close();
#endif
        xbeeSuspended = true;
        LOG_STREAM.println(F("Logging suspended. Unit can be safely powered off."));
        LOG_STREAM.println(F("Send 'R' to resume logging."));
        LOG_STREAM.flush();
        break;

    // RESUME — reopen log file and restart the sampling loop
    case 'r':
    case 'R':
        if (!xbeeSuspended)
        {
            LOG_STREAM.println(F("Already running."));
            break;
        }
        xbeeSuspended = false;
#if USE_DATALOGGER
        openNextLogfile();
#endif
        LOG_STREAM.println(F("Logging resumed."));
        break;

    default:
        LOG_STREAM.print(F("XBee: unknown command: "));
        LOG_STREAM.println(c);
        xbeeHelp();
        break;
    }
#endif // USE_XBEE
}
