# AquaFlux

AQUA-Flux floating/autonomous chamber system designed to measure greenhouse gas fluxes (primarily CO₂ and CH₄) at air-water interfaces in aquatic environments (e.g., lakes, wetlands, ponds). This is an adaptation of methods described by Bastviken et al. (2020) and others, utilizing a floating chamber that periodically opens to allow ambient air exchange and then closes to accumulate gases for measurement. The system is designed for remote deployment and data logging in the field.

This setup logs data to SD card, transmits via XBee for remote access, controls a chamber lid via linear actuator (to periodically open for ambient air exchange and close for flux accumulation), reads multiple sensors, and includes basic error recovery for the I²C bus.

## Modifications to Original Design - March 2026

The following software and hardware modifications were made to the original design to address issues encountered during testing and deployment.

### K30 CO₂ Sensor Integration
- [Level Shifter](./design_files/LevelShifter.md): Added a level shifter to interface the 3.3V sensors with the 5V microcontroller, ensuring proper communication and preventing damage to components.
- **Header Pins**: Only use the header pins, not the board's factory side-connector, to connect the K30 sensor, ensuring a more stable and direct connection for data communication.
- **I²C Address**: Changed the default I²C address of the K30 sensor to 0x69 to avoid conflict with the Real Time Clock (RTC) module, which typically uses 0x68. This allows both devices to coexist on the same I²C bus without address conflicts.

### Renogy Wanderer Solar Panel Integration
- [Power Relay](./design_files/PowerRelay.md): Integrated a relay to control power to the K30 CO₂ sensor, allowing it to be powered up after Arduino bootup to prevent spurious short-circuit issues "E04".

### Software
- **Sensor Timing**: Implemented a startup sequence that powers the K30 sensor after a delay, allowing the sensor to initialize properly before taking measurements.  Adusted timing according to the K30's data sheet to ensure reliable readings.  Added error handling routines to work around K30's I²C bus limitations, such as handling timeouts and retries for communication failures.
- **I²C Bus Initialization and Recovery**: Implemented a routine for the I²C bus to be initialized at startup and handle potential bus lockups, which can occur if a sensor fails to respond or if there are communication issues. This routine attempts to reset the bus and reinitialize communication with the sensors.  Bus lockup faults occur periodically and are believed to be related to the K30 sensor's I²C communication limitations. The implemented recovery routine helps to mitigate these issues and maintain reliable operation of the system.
- **Sub-Files and Sub-Systems**: Refactored the Arduino sketch into multiple sub-files for better organization and readability. This includes separating sensor reading functions, data logging, and communication routines into distinct files and allowing sub-systems to be individually enabled or disabled via preprocessor directives. This modular approach allows for easier maintenance and the ability to disable certain sensors or features if needed without affecting the overall functionality of the system.

## Technical References
- Bastviken, D., et al. (2020). "AQUA-Flux: A floating chamber system for measuring greenhouse gas fluxes at the air-water interface in aquatic environments." Methods in Ecology and Evolution 11(2): 276-287.
- K30 CO₂ Sensor Datasheet: https://rmtplusstoragesenseair.blob.core.windows.net/docs/Dev/publicerat/TDE4700.pdf
- Renogy Wanderer Solar Panel: https://www.renogy.com/products/wanderer-10a-pwm-charge-controller?_pos=1&_psq=wande&_ss=e&_v=1.0