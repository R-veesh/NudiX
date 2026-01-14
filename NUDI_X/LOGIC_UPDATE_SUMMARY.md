# Noodle Vending Machine Logic Update

## Changes Made

### Arduino Code (`arduino_main.ino`)
- **Fixed L298N Motor Duration**: Changed from 20 minutes (1200000ms) to **20 seconds (20000ms)**
- **Relay Duration**: Confirmed to remain at **10 minutes (600000ms)**
- **Sequence of Operations**:
  1. ESP32 sends "DISPENSING" command → Arduino starts drop detection
  2. Ultrasonic sensor continuously monitors for noodle drop
  3. When drop detected → Arduino sends "DROP_DETECTED" to ESP32
  4. Arduino immediately activates:
     - **RELAY (Heater)**: ON for 10 minutes
     - **L298N Motor (Pump)**: ON for 20 seconds
  5. After 20 seconds: L298N Motor turns OFF
  6. After 10 minutes: Relay turns OFF
  7. Arduino sends "HEATING_COMPLETE" to ESP32

### ESP32 Code (`esp32_main.ino`)
- **Updated comments**: Clarified that L298N motor runs for 20 seconds (not 20 minutes)
- **Heating process flow**: When DROP_DETECTED is received:
  - ESP32 sends "START_HEATING" to Arduino
  - Waits up to 11 minutes for "HEATING_COMPLETE" signal
  - Publishes MQTT status updates every 30 seconds

## Serial Monitor Output Timeline

### Phase 1: Drop Detection (0-15 seconds max)
```
[timestamp] Waiting for noodle drop...
[timestamp] Stepper motor spinning continuously...
[timestamp] Distance: XX cm
...
[timestamp] Distance: XX cm
[timestamp] ✅ Noodle drop detected!
[timestamp] DROP_DETECTED
```

### Phase 2: Heating Process Activated (Immediately after drop)
```
[timestamp] ✓ RELAY (Heater) turned ON for 10 minutes
[timestamp] ✓ L298N Motor Driver turned ON for 20 seconds
```

### Phase 3: Motor Running (0-20 seconds)
```
[timestamp] RELAY Status: ON | Remaining: 599 seconds
[timestamp] L298N Motor Status: ON | Remaining: 19 seconds
```

### Phase 4: L298N Motor Stops (At 20 seconds)
```
[timestamp] ✓ L298N Motor turned OFF after 20 seconds
[timestamp] L298N Motor Status: ON | Remaining: 0 seconds
```

### Phase 5: Relay Continues (20 seconds - 10 minutes)
```
[timestamp] RELAY Status: ON | Remaining: 580 seconds
[timestamp] RELAY Status: ON | Remaining: 550 seconds
...
```

### Phase 6: Relay Stops (At 10 minutes)
```
[timestamp] ✓ RELAY (Heater) turned OFF after 10 minutes
[timestamp] All heating components turned OFF
[timestamp] HEATING_COMPLETE
```

## Component Behavior Summary

| Component | Trigger | Duration | Status Check Interval |
|-----------|---------|----------|----------------------|
| Stepper Motor | DISPENSING command | Until DROP_DETECTED | Spinning |
| Relay (Heater) | DROP_DETECTED | 10 minutes | Every 10 seconds |
| L298N Motor (Pump) | DROP_DETECTED (same time as Relay) | 20 seconds | Every 5 seconds |
| Ultrasonic Sensor | During drop detection | Continuous | Every 200ms |

## Testing Checklist

- [ ] Send "noodle_1" command via MQTT
- [ ] Verify ESP32 sends "DISPENSING" to Arduino
- [ ] Verify stepper motor spins
- [ ] Simulate drop with ultrasonic sensor
- [ ] Verify Arduino detects drop and sends "DROP_DETECTED"
- [ ] Verify Relay turns ON
- [ ] Verify L298N motor turns ON simultaneously
- [ ] Verify L298N motor turns OFF after 20 seconds
- [ ] Verify Relay stays ON for full 10 minutes
- [ ] Verify Arduino sends "HEATING_COMPLETE" after 10 minutes
- [ ] Verify all actions are logged in serial monitor with timestamps

## MQTT Topics Used

- `noodle_vending/status`: Current device status
- `noodle_vending/drop_detected`: Drop detection results
- `noodle_vending/log`: Detailed operation logs
- `noodle_vending/command`: Commands from ESP32 to Arduino via serial

