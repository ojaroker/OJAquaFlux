# AquaFlux

AQUA-Flux floating/autonomous chamber system designed to measure greenhouse gas fluxes (primarily CO₂ and CH₄) at air-water interfaces in aquatic environments (e.g., lakes, wetlands, ponds). This is an adaptation of methods described by Bastviken et al. (2020) and others, utilizing a floating chamber that periodically opens to allow ambient air exchange and then closes to accumulate gases for measurement. The system is designed for remote deployment and data logging in the field.

This setup logs data to SD card, transmits via XBee for remote access, controls a chamber lid via linear actuator (to periodically open for ambient air exchange and close for flux accumulation), reads multiple sensors, and includes basic error recovery for the I²C bus.

## Modifications to Original Design - March 2026

The following software and hardware modifications were made to the original design to address issues encountered during testing and deployment.

### K30 CO₂ Sensor Integration
- [Level Shifter](./design_files/LevelShifter.md): Added a level shifter to interface the 3.3V sensors with the 5V microcontroller, ensuring proper communication and preventing damage to components.
- **Header Pins**: Only use the header pins, not the board's factory side-connector, to connect the K30 sensor, ensuring a more stable and direct connection for data communication.
- **I²C Address**: Changed the default I²C address of the K30 sensor to 0x69 to avoid conflict with the Real Time Clock (RTC) module, which typically uses 0x68. This allows both devices to coexist on the same I²C bus without address conflicts.
- [Error Handling](./design_files/K30.md): To improve reliability, several software improvements were added to the K30's startup and operation.

### Renogy Wanderer Solar Panel Integration
- [Power Relay](./design_files/PowerRelay.md): Integrated a relay to control power to the K30 CO₂ sensor, allowing it to be powered up after Arduino bootup to prevent spurious short-circuit issues "E04".

### Software
- **Sensor Timing**: Implemented a startup sequence that powers the K30 sensor after a delay, allowing the sensor to initialize properly before taking measurements.  Adusted timing according to the K30's data sheet to ensure reliable readings.  Added error handling routines to work around K30's I²C bus limitations, such as handling timeouts and retries for communication failures.
- **I²C Bus Initialization and Recovery**: Implemented a routine for the I²C bus to be initialized at startup and handle potential bus lockups, which can occur if a sensor fails to respond or if there are communication issues. This routine attempts to reset the bus and reinitialize communication with the sensors.  Bus lockup faults occur periodically and are believed to be related to the K30 sensor's I²C communication limitations. The implemented recovery routine helps to mitigate these issues and maintain reliable operation of the system.
- **Sub-Files and Sub-Systems**: Refactored the Arduino sketch into multiple sub-files for better organization and readability. This includes separating sensor reading functions, data logging, and communication routines into distinct files and allowing sub-systems to be individually enabled or disabled via preprocessor directives. This modular approach allows for easier maintenance and the ability to disable certain sensors or features if needed without affecting the overall functionality of the system.
- **Unified Serial Output via `LOG_STREAM`**: All output (sensors, debug messages, errors) is routed through a single `LOG_STREAM` macro that resolves to `XBee` (SoftwareSerial) or `Serial` (USB) depending on the `USE_XBEE` compile flag. Switching between XBee and USB logging requires no code changes beyond the flag.
- **`DEBUG_PRINT` / `DEBUG_PRINTLN` Macros**: Replaced scattered `#if DEBUG` blocks with two macros. When `DEBUG=0` they expand to a `do{}while(0)` no-op, eliminating all debug output with no runtime cost.
- **Date-Based Log File Rotation**: The SD card logger writes to a file named `YYYYMMDD.CSV`, derived from the PCF8523 RTC. A new file is opened automatically at midnight; a same-day reboot appends to the existing file without duplicating the CSV header. The sketch halts with an error message if the SD card or file cannot be opened.
- **Non-Blocking Chamber Actuator State Machine** (`chamber.ino`): The linear actuator is now controlled by a `ChamberActuator` class implementing a five-state machine (`UNKNOWN > CLOSING > CLOSED > OPENING > OPEN`). Timing is driven by named constants (`CHAMBER_CLOSED_MS`, `CHAMBER_OPEN_MS`, `CHAMBER_TRANSITION_MS`) rather than magic numbers. The previous design blocked the main loop for 24 s (6 × `delay(4000)`) during each transition; the new design is fully non-blocking. During a transition the main loop skips sensor reads and delays but continues processing XBee commands, printing a `.` once per second to show the program is still running.
- **Startup Configuration Report** (`printConfig.ino`): A `printConfig()` function called from `setup()` prints the enabled/disabled status of all subsystems and the Arduino pin assignments to `LOG_STREAM`, making it easy to verify the build configuration in the field.

## Technical References
- K30 CO₂ Sensor Datasheet: https://rmtplusstoragesenseair.blob.core.windows.net/docs/Dev/publicerat/TDE4700.pdf
- K30 CO₂ Sensor Product Specification: https://rmtplusstoragesenseair.blob.core.windows.net/docs/publicerat/PSP110.pdf
- Renogy Wanderer Solar Panel: https://www.renogy.com/products/wanderer-10a-pwm-charge-controller?_pos=1&_psq=wande&_ss=e&_v=1.0