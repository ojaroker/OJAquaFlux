void scanI2cBus()
{
  LOG_STREAM.println(F("Scanning I2C bus for devices..."));
  for (byte address = 1; address < 127; address++)
  {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    if (error == 0)
    {
      LOG_STREAM.print(F("I2C device found at address 0x"));
      if (address < 16)
        LOG_STREAM.print(F("0"));
      LOG_STREAM.println(address, HEX);
    }
    else if (error == 4)
    {
      LOG_STREAM.print(F("Unknown error at address 0x"));
      if (address < 16)
        LOG_STREAM.print(F("0"));
      LOG_STREAM.println(address, HEX);
    }
  }
  LOG_STREAM.println(F("I2C scan complete."));
}