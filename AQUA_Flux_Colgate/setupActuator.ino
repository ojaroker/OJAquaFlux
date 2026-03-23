// setupActuator.ino
//
// Actuator initialisation has moved into ChamberActuator::begin() in chamber.ino.
// That method attaches the servo to ACTUATOR_PIN, issues the retract pulse, and
// enters CLOSING state. It is called from setup() in AQUA_Flux_Colgate.ino.
