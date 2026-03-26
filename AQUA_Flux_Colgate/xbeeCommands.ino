// xbeeCommands.ino
//
// Processes single-character commands received over XBee.
//
// Commands:
//   'O' / 'o' — force the chamber open   (extend actuator, enter OPENING state)
//   'C' / 'c' — force the chamber closed  (retract actuator, enter CLOSING state)
//   'D' / 'd' — set RTC date and time interactively
//   'I' / 'i' — set the AQUA-Flux unit ID (1-255, logged as last CSV column)
//   'L' / 'l' — set the sensor log interval in seconds (3s min to CHAMBER_CLOSED_MS/10)
//   'P' / 'p' — print build configuration and pin assignments
//   'S' / 's' — suspend: flush and close log file; notify operator safe to power off
//   'R' / 'r' — resume: reopen log file and restart the sampling loop
//
// 'S' sets the global xbeeSuspended flag. While suspended, loop() skips all
// sensor reads and delays, calling only xbeeCommands() until 'R' is received.
// Unknown commands echo the received byte and print the help list.

void xbeeHelp()
{
    LOG_STREAM.println(F("===== XBee Commands ====="));
    LOG_STREAM.println(F(" 'O' - Force chamber Open"));
    LOG_STREAM.println(F(" 'C' - Force chamber Closed"));
    LOG_STREAM.println(F(" 'D' - Set RTC Date and Time"));
    LOG_STREAM.println(F(" 'I' - Set AQUA-Flux unit ID"));
    LOG_STREAM.println(F(" 'L' - Set log interval (seconds)"));
    LOG_STREAM.println(F(" 'P' - Print configuration"));
    LOG_STREAM.println(F(" 'S' - Suspend (safe power-off)"));
    LOG_STREAM.println(F(" 'R' - Resume operation"));
    LOG_STREAM.println(F("========================="));
}

// Prompt for a new unit ID (1-255) and update aquaFluxId.
#if USE_XBEE
static void setAquaFluxId()
{
    LOG_STREAM.print(F("Current ID: "));
    LOG_STREAM.println(aquaFluxId);
    LOG_STREAM.println(F("Enter new AQUA-Flux ID (1-255):"));

    char buf[4];
    uint8_t len = 0;
    unsigned long deadline = millis() + 30000UL;
    while (millis() < deadline)
    {
        if (!LOG_STREAM.available())
            continue;
        char c = (char)LOG_STREAM.read();
        if (c == '\r' || c == '\n')
            break;
        if (c >= '0' && c <= '9' && len < 3)
            buf[len++] = c;
    }
    buf[len] = '\0';

    if (len == 0)
    {
        LOG_STREAM.println(F("Timeout. ID unchanged."));
        return;
    }
    int val = atoi(buf);
    if (val < 1 || val > 255)
    {
        LOG_STREAM.println(F("Invalid. ID must be 1-255. ID unchanged."));
        return;
    }
    aquaFluxId = (uint8_t)val;
    LOG_STREAM.print(F("AQUA-Flux ID set to: "));
    LOG_STREAM.println(aquaFluxId);
}

// Prompt for a new sensor log interval (in seconds) and update LOG_INTERVAL.
// Allowable range: 3 s minimum, CHAMBER_CLOSED_MS/10 maximum (300 s at 50-min cycle).
static void setLogInterval()
{
    const unsigned long minS = 3UL;
    const unsigned long maxS = CHAMBER_CLOSED_MS / 10UL / 1000UL;

    LOG_STREAM.print(F("Current log interval: "));
    LOG_STREAM.print(LOG_INTERVAL / 1000UL);
    LOG_STREAM.print(F(" s ("));
    LOG_STREAM.print(minS);
    LOG_STREAM.print(F("-"));
    LOG_STREAM.print(maxS);
    LOG_STREAM.println(F(" s allowed):"));

    char buf[4]; // up to 3 digits (max 300)
    uint8_t len = 0;
    unsigned long deadline = millis() + 30000UL;
    while (millis() < deadline)
    {
        if (!LOG_STREAM.available())
            continue;
        char c = (char)LOG_STREAM.read();
        if (c == '\r' || c == '\n')
            break;
        if (c >= '0' && c <= '9' && len < 3)
            buf[len++] = c;
    }
    buf[len] = '\0';

    if (len == 0)
    {
        LOG_STREAM.println(F("Timeout. Interval unchanged."));
        return;
    }
    unsigned long val = (unsigned long)atoi(buf);
    if (val < minS || val > maxS)
    {
        LOG_STREAM.print(F("Invalid. Must be between"));
        LOG_STREAM.print(minS);
        LOG_STREAM.print(F("-"));
        LOG_STREAM.print(maxS);
        LOG_STREAM.println(F(" s. Interval unchanged."));
        return;
    }
    LOG_INTERVAL = val * 1000UL;
    LOG_STREAM.print(F("Log interval set to: "));
    LOG_STREAM.print(val);
    LOG_STREAM.println(F(" s"));
}
#endif // USE_XBEE

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

    // SET UNIT ID
    case 'i':
    case 'I':
        setAquaFluxId();
        break;

    // SET LOG INTERVAL
    case 'l':
    case 'L':
        setLogInterval();
        break;

    // PRINT CONFIG
    case 'p':
    case 'P':
        printConfig();
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
        if (xbeeSuspended)
        {
            LOG_STREAM.println(F("Already suspended."));
            break;
        }
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
