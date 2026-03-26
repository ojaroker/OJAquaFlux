// error.ino
//
// Prints an error message to LOG_STREAM, flushes the output, and halts
// execution in an infinite loop. A manual reboot is required to recover.
//
// Called throughout the sketch when an unrecoverable condition is detected
// (e.g. SD card missing, RTC not found, SD read/write mismatch).

void error(const char *str)
{
  LOG_STREAM.print(F("ERROR OCCURRED: "));
  LOG_STREAM.println(str);
  LOG_STREAM.println(F("Execution halted. Please reboot manually."));
  LOG_STREAM.flush();
  while (1)
    ; // Halt
}
