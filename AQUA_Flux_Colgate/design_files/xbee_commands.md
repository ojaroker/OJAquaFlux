# XBee Commands

Single-character commands sent over the XBee radio link to control and configure a running AQUA-Flux unit. Commands are case-insensitive (`O` and `o` are equivalent). When `USE_XBEE=0`, the same commands are available over USB Serial instead.

Commands are processed:
- **During the log interval wait** — polled once per second via `interruptibleWait()`
- **During actuator transitions** — polled on every `loop()` tick while the chamber is opening or closing
- **While suspended** — the only thing processed until `R` resumes operation

Unknown commands echo the received byte and print the command list.

---

## Chamber Control

### `O` — Force Chamber Open
Immediately issues the extend pulse to the linear actuator and enters the `OPENING` state, regardless of the current state. The chamber will travel for 24 s and then enter `OPEN` (10 min equilibration), after which the normal cycle resumes.

Use this to manually open the chamber for inspection or maintenance.

> Requires `USE_ACTUATOR=1`. If the actuator is disabled, prints an error and does nothing.

### `C` — Force Chamber Closed
Immediately issues the retract pulse to the linear actuator and enters the `CLOSING` state, regardless of the current state. The chamber will travel for 24 s and then enter `CLOSED` (50 min measurement period), after which the normal cycle resumes.

Use this to manually close the chamber or to abort an open cycle early.

> Requires `USE_ACTUATOR=1`. If the actuator is disabled, prints an error and does nothing.

---

## Logging Control

### `S` — Suspend
Flushes and closes the SD card log file, sets the `xbeeSuspended` flag, and notifies the operator that the unit can be safely powered off. While suspended, all sensor reads and timing delays are skipped; only XBee commands are processed.

```
Logging suspended. Unit can be safely powered off.
Send 'R' to resume logging.
```

Use this before disconnecting power to avoid a partial or corrupt final log row.

> Has no effect if the unit is already suspended.

### `R` — Resume
Clears the `xbeeSuspended` flag and opens a new log file on the SD card. Sensor reads and the measurement cycle resume immediately on the next `loop()` iteration.

```
Logging resumed.
```

> Has no effect if the unit is already running.

---

## Configuration

### `I` — Set Unit ID
Prompts for a new AQUA-Flux unit ID (integer, 1–255). The ID is appended as the last column of every CSV log row and is used to identify the unit when multiple nodes are deployed.

**Input:** up to 3 digits, terminated by CR or LF. Times out after 30 s with no change.

```
Current ID: 1
Enter new AQUA-Flux ID (1-255):
```

### `L` — Set Log Interval
Prompts for a new sensor log interval in seconds. Measurements are taken once per interval.

- **Minimum:** 3 s (limited by K30 measurement period)
- **Maximum:** 300 s (1/10 of the 50-minute chamber closed period)

**Input:** up to 3 digits, terminated by CR or LF. Times out after 30 s with no change.

```
Current log interval: 10 s (3-300 s allowed):
```

### `D` — Set RTC Date and Time
Interactive multi-step prompt to set the real-time clock. All times are **UTC**.

> Requires `USE_DATALOGGER=1`. If the data logger is disabled, prints an error and does nothing.

**Step 1 — Date**
```
Enter date (YYYYMMDD):
```
Enter exactly 8 digits, e.g. `20260315` for March 15, 2026. Up to 3 invalid attempts are allowed; the 60 s per-field timeout applies.

**Step 2 — Time**
```
Enter time (HHMMSS):
```
Enter exactly 6 digits in 24-hour format, e.g. `143022` for 14:30:22. Same attempt and timeout limits apply.

**Step 3 — Confirm**
```
Set RTC to: 2026-03-15 14:30:22 UTC
Confirm? Enter Y to accept, any other key to cancel:
```
Send `Y` or `y` to write. Any other character cancels without changing the RTC.

After writing, the firmware waits 2 s for the crystal to stabilize, reads back the RTC, and verifies the difference is within 60 s. A larger difference indicates a hardware fault (dead battery, bad I2C connection) and triggers an error halt.

### `P` — Print Configuration
Prints the current build configuration and pin assignments to the log stream. Equivalent to the output shown at startup.

---

## Quick Reference

| Command | Action | Notes |
|---------|--------|-------|
| `O` | Force chamber open | Requires `USE_ACTUATOR=1` |
| `C` | Force chamber closed | Requires `USE_ACTUATOR=1` |
| `S` | Suspend logging | Safe power-off; send `R` to resume |
| `R` | Resume logging | Reopens log file |
| `I` | Set unit ID | 1–255; 30 s input timeout |
| `L` | Set log interval | 3–300 s; 30 s input timeout |
| `D` | Set RTC date/time | UTC; YYYYMMDD + HHMMSS; requires `USE_DATALOGGER=1` |
| `P` | Print configuration | — |
