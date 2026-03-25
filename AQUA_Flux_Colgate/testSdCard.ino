// testSdCard.ino
//
// Verifies SD card read/write integrity at startup.
// Writes a known string to a temporary file, reads it back, and confirms
// the content matches. Calls error() and halts if the card is absent,
// unwritable, or returns corrupt data.
// Called once from setupLogging() after SD.begin() succeeds.

#if USE_DATALOGGER
static void testSdCard()
{
  const char* TEST_FILE = "SDTEST.TMP";
  const char* TEST_STR  = "AQUA-Flux SD test";

  // --- write ---
  SD.remove(TEST_FILE); // clean slate in case a previous run left the file
  File f = SD.open(TEST_FILE, FILE_WRITE);
  if (!f) error("SD test: couldn't create test file");
  f.println(TEST_STR);
  f.flush();
  f.close();

  // --- read back ---
  f = SD.open(TEST_FILE, FILE_READ);
  if (!f) error("SD test: couldn't reopen test file");

  char buf[32];
  uint8_t len = 0;
  while (f.available() && len < sizeof(buf) - 1)
  {
    char c = f.read();
    if (c == '\r' || c == '\n') break;
    buf[len++] = c;
  }
  buf[len] = '\0';
  f.close();
  SD.remove(TEST_FILE);

  // --- verify ---
  if (strcmp(buf, TEST_STR) != 0)
  {
    LOG_STREAM.print(F("SD test FAIL. Got: "));
    LOG_STREAM.println(buf);
    error("SD read/write mismatch");
  }
  DEBUG_PRINTLN(F("SD test: PASS"));
}
#endif // USE_DATALOGGER
