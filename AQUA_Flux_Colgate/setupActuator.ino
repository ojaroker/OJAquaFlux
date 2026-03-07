// TODO - POSSIBLE BUG
// 1850 µs is the extend position (same value used in extendActuator()).
// retractActuator() uses 1000 µs. The comment says "retract" but the value
// extends. Either the value or the comment is wrong — the actuator will
// extend on startup when USE_ACTUATOR=1.

// Actuator Servo

// pinMode(solpin, OUTPUT);

// Create a servo object named "actuator"
Servo actuator;

void setupActuator()
{
#if USE_ACTUATOR
#if DEBUG
  LOG_STREAM.println(F("DEBUG - Linear actuator enabled"));
#endif
  // Attach the actuator to Arduino digital pin 4
  actuator.attach(ACTUATOR_PIN);
  actuator.writeMicroseconds(1850); // 1ms pulse to fully retract the actuator, 2ms pulse to fully extend the actuator (1000us = 1ms)
#else
#if DEBUG
  LOG_STREAM.println(F("DEBUG - Linear actuator disabled"));
#endif
#endif
}