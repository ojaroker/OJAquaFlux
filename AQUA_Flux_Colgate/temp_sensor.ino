
// Define parameters for the water temp. thermistor (from Adafruit: https://learn.adafruit.com/thermistor/using-a-thermistor)
// analog pin
// #define THERMISTORPIN A3
// // resistance at 25 degrees C
// #define THERMISTORNOMINAL 10000
// // temp. for nominal resistance (almost always 25 C)
// #define TEMPERATURENOMINAL 25
// // how many samples to take and average, more takes longer, but is more 'smooth'
// #define NUMSAMPLES 5
// // the beta coefficient of the thermistor (usually 3000-4000)
// #define BCOEFFICIENT 3950
// // the value of the 'other' resistor
// #define SERIESRESISTOR 10000
// // digital pin providing 5V to the thermistor
// #define therm_power_pin 6

// int samples[NUMSAMPLES];

// pinMode(therm_power_pin, OUTPUT);

void temp_sensor()
{
  // Take water temperature measurement
  // uint8_t i;
  // float average;
  // // Take N samples in a row, with a slight delay
  // digitalWrite(therm_power_pin,HIGH);
  // for (i=0; i< NUMSAMPLES; i++) {
  //  samples[i] = analogRead(THERMISTORPIN);
  //  delay(10);
  // }
  // digitalWrite(therm_power_pin,LOW);
  // // average all the samples
  // average = 0;
  // for (i=0; i< NUMSAMPLES; i++) {
  //    average += samples[i];
  // }
  // average /= NUMSAMPLES;
  // average = 1023 / average - 1;
  // average = SERIESRESISTOR / average;
  // float steinhart;
  // steinhart = average / THERMISTORNOMINAL;     // (R/Ro)
  // steinhart = log(steinhart);                  // ln(R/Ro)
  // steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  // steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  // steinhart = 1.0 / steinhart;                 // Invert
  // steinhart -= 273.15; // convert absolute temp to C
  // delay(10);
  // XBee.print(", ");
  // XBee.print(steinhart);
  // XBee.print(", ");

  // Watchdog.reset();

  // // Take differential pressure measurement
  // float averageP;
  // // Take N samples in a row, with a slight delay
  // for (i=0; i< NUMSAMPLES; i++) {
  //  samples[i] = analogRead(P);
  //  delay(10);
  // }
  // // average all the samples
  // averageP = 0;
  // for (i=0; i< NUMSAMPLES; i++) {
  //    averageP += samples[i];
  // }
  // averageP /= NUMSAMPLES;
  // PmV = averageP *(mV/steps);
  // delay(10);
  // XBee.print(PmV);
  // XBee.print(", ");

  // Watchdog.reset();

  // Print all of the data to the SD card file
  // logfile.print(steinhart);
  // logfile.print(", ");
  // logfile.print(PmV);
  // logfile.print(", ");
}