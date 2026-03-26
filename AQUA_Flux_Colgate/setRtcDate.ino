// setRtcDate.ino
//
// Interactive RTC date and time setter. Prompts the operator for a date and
// time over LOG_STREAM (XBee or Serial), shows the parsed result, asks for
// confirmation, writes it to the RTC, then reads back after 2 s to verify
// the write succeeded.
//
// Typical call site:
//   if (!rtc.initialized() || rtc.lostPower()) setRtcDate();
//
// Input format:
//   Date — "YYYYMMDD"  (e.g. 20240315 for March 15 2024)
//   Time — "HHMMSS"    (e.g. 143022 for 14:30:22 UTC)
//
// Each field allows SET_RTC_MAX_ATTEMPTS attempts before the function returns
// without changing the RTC. The confirmation step accepts 'Y' or 'y'; any
// other character cancels without changing the RTC.
//
// Verification: after rtc.adjust() + 2 s settle delay, reads back rtc.now()
// and compares unix timestamps. Notifies if the difference exceeds 60 s,
// which indicates the RTC did not retain the written time (dead battery, bad
// I2C connection, or hardware fault).
//
// Guards: entire file is compiled only when USE_DATALOGGER=1, because the
// RTC object (rtc) is declared under that guard in AQUA_Flux_Colgate.ino.

#if USE_DATALOGGER

#define SET_RTC_INPUT_TIMEOUT_MS 60000UL // seconds to enter each field before retry
#define SET_RTC_MAX_ATTEMPTS 3           // max invalid attempts per field before abort
#define SET_RTC_VERIFY_DELTA_S 60        // maximum allowed difference after readback (s)
#define SET_RTC_SETTLE_MS 2000           // delay after rtc.adjust() before readback

// Read up to maxLen printable characters from LOG_STREAM into buf[], stopping
// at CR, LF, or timeout. Null-terminates buf. Returns number of chars read
// (not counting the null). Times out after SET_RTC_INPUT_TIMEOUT_MS.
static uint8_t rtcReadLine(char *buf, uint8_t maxLen)
{
  uint8_t pos = 0;
  unsigned long deadline = millis() + SET_RTC_INPUT_TIMEOUT_MS;

  while (millis() < deadline)
  {
    if (!LOG_STREAM.available())
      continue;
    char c = (char)LOG_STREAM.read();
    if (c == '\r' || c == '\n')
      break;
    if (c >= ' ' && pos < maxLen) // ignore control characters
      buf[pos++] = c;
  }
  buf[pos] = '\0';
  return pos;
}

// Returns true if all len characters in buf are ASCII decimal digits.
static bool rtcAllDigits(const char *buf, uint8_t len)
{
  for (uint8_t i = 0; i < len; i++)
    if (buf[i] < '0' || buf[i] > '9')
      return false;
  return true;
}

// Returns true if dt holds a plausible deployment date and time.
// Bounds: year 2026–2040 (matches operator entry validation in setRtcDate()).
// A false result indicates the RTC was never set, lost its battery, or returned
// corrupt data over I2C.
bool isRtcDateValid(const DateTime &dt)
{
  return dt.year() >= 2026 && dt.year() <= 2040 && dt.month() >= 1 && dt.month() <= 12 && dt.day() >= 1 && dt.day() <= 31 && dt.hour() <= 23 && dt.minute() <= 59 && dt.second() <= 59;
}

// Prompt the operator for a date and time, write to RTC, and verify.
// Returns without calling error() if the operator cancels or exceeds the
// attempt limit. Calls error() only if the RTC write verification fails.
void setRtcDate()
{
  LOG_STREAM.println(F("--- Set RTC Date and Time ---"));

  char dateBuf[9]; // "YYYYMMDD" + null
  char timeBuf[7]; // "HHMMSS"  + null

  // ── Date input ──────────────────────────────────────────────────────────
  bool gotDate = false;
  for (uint8_t attempt = 0; attempt < SET_RTC_MAX_ATTEMPTS && !gotDate; attempt++)
  {
    LOG_STREAM.println(F("Enter date (YYYYMMDD):"));
    uint8_t len = rtcReadLine(dateBuf, 8);
    if (len == 8 && rtcAllDigits(dateBuf, 8))
      gotDate = true;
    else
      LOG_STREAM.println(F("Invalid — expected exactly 8 digits (e.g. 20240315)."));
  }
  if (!gotDate)
  {
    LOG_STREAM.println(F("Too many invalid attempts. RTC not changed."));
    return;
  }

  // ── Time input ──────────────────────────────────────────────────────────
  bool gotTime = false;
  for (uint8_t attempt = 0; attempt < SET_RTC_MAX_ATTEMPTS && !gotTime; attempt++)
  {
    LOG_STREAM.println(F("Enter time (HHMMSS):"));
    uint8_t len = rtcReadLine(timeBuf, 6);
    if (len == 6 && rtcAllDigits(timeBuf, 6))
      gotTime = true;
    else
      LOG_STREAM.println(F("Invalid — expected exactly 6 digits (e.g. 143022)."));
  }
  if (!gotTime)
  {
    LOG_STREAM.println(F("Too many invalid attempts. RTC not changed."));
    return;
  }

  // ── Parse ────────────────────────────────────────────────────────────────
  uint16_t year = (uint16_t)(dateBuf[0] - '0') * 1000 + (dateBuf[1] - '0') * 100 + (dateBuf[2] - '0') * 10 + (dateBuf[3] - '0');
  uint8_t month = (dateBuf[4] - '0') * 10 + (dateBuf[5] - '0');
  uint8_t day = (dateBuf[6] - '0') * 10 + (dateBuf[7] - '0');
  uint8_t hour = (timeBuf[0] - '0') * 10 + (timeBuf[1] - '0');
  uint8_t minute = (timeBuf[2] - '0') * 10 + (timeBuf[3] - '0');
  uint8_t second = (timeBuf[4] - '0') * 10 + (timeBuf[5] - '0');

  // ── Range validation ─────────────────────────────────────────────────────
  DateTime proposed(year, month, day, hour, minute, second);
  if (!isRtcDateValid(proposed))
  {
    LOG_STREAM.println(F("Date/time values out of range. RTC not changed."));
    return;
  }

  // ── Show and confirm ──────────────────────────────────────────────────────
  char preview[24]; // "YYYY-MM-DD HH:MM:SS UTC" = 23 chars + null
  snprintf(preview, sizeof(preview), "%04d-%02d-%02d %02d:%02d:%02d UTC",
           year, month, day, hour, minute, second);
  LOG_STREAM.print(F("Set RTC to: "));
  LOG_STREAM.println(preview);
  LOG_STREAM.println(F("Confirm? Enter Y to accept, any other key to cancel:"));

  char confirmBuf[4];
  rtcReadLine(confirmBuf, 3);
  if (confirmBuf[0] != 'Y' && confirmBuf[0] != 'y')
  {
    LOG_STREAM.println(F("Cancelled. RTC not changed."));
    return;
  }

  // ── Write to RTC ──────────────────────────────────────────────────────────
  DateTime toSet(year, month, day, hour, minute, second);
  rtc.adjust(toSet);
  LOG_STREAM.println(F("RTC updated. Waiting 2 s for crystal to stabilize..."));
  delay(SET_RTC_SETTLE_MS);

  // ── Read back and verify ──────────────────────────────────────────────────
  DateTime readBack = rtc.now();

  // Compute absolute difference in seconds using unsigned arithmetic to avoid
  // signed overflow. unixtime() returns uint32_t so subtraction is well-defined.
  uint32_t setUnix = toSet.unixtime();
  uint32_t readUnix = readBack.unixtime();
  uint32_t diff = (readUnix >= setUnix) ? (readUnix - setUnix)
                                        : (setUnix - readUnix);

  if (diff > SET_RTC_VERIFY_DELTA_S)
  {
    LOG_STREAM.print(F("RTC verification failed. Readback differs by "));
    LOG_STREAM.print(diff);
    LOG_STREAM.println(F(" s from the set time."));
    error("RTC did not retain the written time");
  }

  char verifyBuf[24]; // "YYYY-MM-DD HH:MM:SS UTC" = 23 chars + null
  snprintf(verifyBuf, sizeof(verifyBuf), "%04d-%02d-%02d %02d:%02d:%02d UTC",
           readBack.year(), readBack.month(), readBack.day(),
           readBack.hour(), readBack.minute(), readBack.second());
  LOG_STREAM.print(F("RTC verified: "));
  LOG_STREAM.println(verifyBuf);
  LOG_STREAM.println(F("----------------------------"));
}

#endif // USE_DATALOGGER