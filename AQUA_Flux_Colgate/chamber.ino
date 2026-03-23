// chamber.ino
//
// Non-blocking chamber actuator state machine.
//
// States and transitions:
//
//   CLOSING ──(CHAMBER_TRANSITION_MS)──> CLOSED
//   CLOSED  ──(CHAMBER_CLOSED_MS)──────> OPENING  [extends actuator]
//   OPENING ──(CHAMBER_TRANSITION_MS)──> OPEN
//   OPEN    ──(CHAMBER_OPEN_MS)────────> CLOSING  [retracts actuator]
//
// Timing constants (adjust to match deployment):
//   CHAMBER_CLOSED_MS     — how long the chamber stays sealed between measurements
//   CHAMBER_OPEN_MS       — how long the chamber stays open for equilibration
//   CHAMBER_TRANSITION_MS — travel time for the linear actuator to fully extend/retract
//
// Design notes:
//   - begin() is non-blocking: it attaches the servo, issues the retract pulse, and
//     immediately sets state to CLOSING. The transition completes in update().
//   - update() must be called on every loop() iteration. It compares elapsed time
//     against the current-state timeout and advances the state when the timeout fires.
//   - During CLOSING or OPENING the main loop() returns early (after processing XBee
//     commands), skipping the 7 s sensor delays and all sensor reads. This keeps the
//     Arduino responsive during the 24 s actuator travel without blocking.
//   - millis() rollover safety: elapsed = millis() - _stateEnteredMs uses unsigned
//     subtraction, which wraps correctly as long as no single state lasts > 49.7 days.
//   - State changes are logged as standalone lines (not embedded in a CSV row) to both
//     LOG_STREAM and (when USE_DATALOGGER=1) the SD card log file.

// -----------------------------------------------------------------------------
// Timing Constants
// -----------------------------------------------------------------------------
#define CHAMBER_CLOSED_MS (50UL * 60 * 1000) // 50 minutes sealed between measurements
#define CHAMBER_OPEN_MS (10UL * 60 * 1000)   // 10 minutes open for equilibration
#define CHAMBER_TRANSITION_MS (24UL * 1000)  // 24 s actuator travel time

// Actuator pulse widths — adjust these two values to calibrate for your actuator
#define ACTUATOR_RETRACT_US 1000 // retract pulse width (µs)
#define ACTUATOR_EXTEND_US 1850  // extend pulse width (µs)

// -----------------------------------------------------------------------------
// Actuator State Machine
// -----------------------------------------------------------------------------
enum ActuatorState
{
    UNKNOWN, // initial state before begin() is called
    OPENING, // actuator extending; transition in progress
    OPEN,    // actuator fully extended; chamber open
    CLOSING, // actuator retracting; transition in progress
    CLOSED   // actuator fully retracted; chamber sealed
};

#if USE_ACTUATOR
class ChamberActuator
{
public:
    // Attach the servo to ACTUATOR_PIN, issue the retract pulse to establish a
    // known starting position, and enter CLOSING state. Returns immediately —
    // the transition completes after CHAMBER_TRANSITION_MS via update().
    void begin();

    // Advance the state machine. Must be called on every loop() iteration.
    // Checks elapsed time in the current state and triggers the next transition
    // when the timeout fires. Issues actuator pulses at the start of OPENING and
    // CLOSING transitions.
    void update();

    // Returns true while the actuator is moving (OPENING or CLOSING).
    // The main loop() uses this to skip sensor reads during transitions.
    bool isTransitioning() const { return _state == OPENING || _state == CLOSING; }

    // Returns the current state for diagnostics.
    ActuatorState getState() const { return _state; }

    // Force the chamber open immediately, regardless of current state.
    // Issues the extend pulse and enters OPENING. Intended for XBee manual override.
    void forceOpen();

    // Force the chamber closed immediately, regardless of current state.
    // Issues the retract pulse and enters CLOSING. Intended for XBee manual override.
    void forceClose();

private:
    ActuatorState _state = UNKNOWN;
    unsigned long _stateEnteredMs = 0;

    // Record the new state, timestamp it, and log the transition.
    void _setState(ActuatorState s);
};

void ChamberActuator::begin()
{
    DEBUG_PRINT(F("DEBUG - Linear actuator enabled and in UNKNOWN state"));
    actuator.attach(ACTUATOR_PIN);
    actuator.writeMicroseconds(ACTUATOR_RETRACT_US); // retract to known starting position
    _setState(CLOSING);
}

void ChamberActuator::_setState(ActuatorState s)
{
    _state = s;
    _stateEnteredMs = millis();

    // Log the state change as a standalone line, not as part of a CSV sensor row
    switch (s)
    {
    case OPENING:
        LOG_STREAM.print(F("Opening Chamber"));
        break;
    case OPEN:
        LOG_STREAM.println(F("Chamber is OPEN"));
        break;
    case CLOSING:
        LOG_STREAM.print(F("Closing Chamber"));
        break;
    case CLOSED:
        LOG_STREAM.println(F("Chamber is CLOSED"));
        break;
    default:
        break;
    }

#if USE_DATALOGGER
    switch (s)
    {
    case OPEN:
        logfile.println(F("Chamber is OPEN"));
        break;
    case CLOSED:
        logfile.println(F("Chamber is CLOSED"));
        break;
    default:
        break;
    }
#endif
}

void ChamberActuator::update()
{
    unsigned long elapsed = millis() - _stateEnteredMs;

    switch (_state)
    {
    case CLOSING:
        // Wait for the actuator to finish retracting
        if (elapsed >= CHAMBER_TRANSITION_MS)
            _setState(CLOSED);
        break;

    case CLOSED:
        // Hold sealed for the measurement period, then start opening
        if (elapsed >= CHAMBER_CLOSED_MS)
        {
            actuator.writeMicroseconds(ACTUATOR_EXTEND_US); // extend
            _setState(OPENING);
        }
        break;

    case OPENING:
        // Wait for the actuator to finish extending
        if (elapsed >= CHAMBER_TRANSITION_MS)
            _setState(OPEN);
        break;

    case OPEN:
        // Hold open for equilibration, then start closing
        if (elapsed >= CHAMBER_OPEN_MS)
        {
            actuator.writeMicroseconds(ACTUATOR_RETRACT_US); // retract
            _setState(CLOSING);
        }
        break;

    default:
        break;
    }
}
void ChamberActuator::forceOpen()
{
    actuator.writeMicroseconds(ACTUATOR_EXTEND_US); // extend
    _setState(OPENING);
}

void ChamberActuator::forceClose()
{
    actuator.writeMicroseconds(ACTUATOR_RETRACT_US); // retract
    _setState(CLOSING);
}
#endif // USE_ACTUATOR
