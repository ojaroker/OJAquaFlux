void setupI2c(void)
{
  // Initialize I2C
  Wire.begin();
  Wire.setClock(I2C_CLOCK_SPEED);

  // Put the I2C bus into a known state
  LOG_STREAM.print(F("Initializing I2C bus..."));
  recoverI2CBus();
}