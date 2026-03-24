// interruptibleWait.ino
//
// Waits for `ms` milliseconds. While waiting, polls xbeeCommands() every
// second (USE_XBEE=1 only) so the device stays responsive during long sensor
// delays. Falls back to a plain delay() when USE_XBEE=0 to avoid a busy-wait
// spin that wastes cycles when XBee is not compiled in.

void interruptibleWait(unsigned long ms)
{
#if USE_XBEE
    unsigned long start = millis();
    unsigned long lastXbeeMs = start;
    while (millis() - start < ms)
    {
        if (millis() - lastXbeeMs >= 1000)
        {
            xbeeCommands();
            lastXbeeMs = millis();
        }
    }
#else
    delay(ms);
#endif
}
