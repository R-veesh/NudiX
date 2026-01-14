# Serial Communication Debug Guide

## Summary of Fixes Applied

### 1. **Arduino Code Fixes**
- ✅ **Added missing `L298N_ENA` pin definition** (Pin 11)
  - This pin was being referenced in `pinMode()` but never defined
  
- ✅ **Removed undefined `dropSensorCount` variable**
  - Was initialized but never used
  
- ✅ **Corrected RX/TX pin comments** to be clearer about data direction
  - RX Pin 8: Arduino receives FROM ESP32 (ESP32 TX → Arduino RX)
  - TX Pin 9: Arduino sends TO ESP32 (Arduino TX → ESP32 RX)

- ✅ **Enhanced serial communication debugging**
  - Added timestamp logging for all messages
  - Added pin configuration printout on startup
  - Added "test_rx" command for manual testing

### 2. **ESP32 Code Improvements**
- ✅ **Enhanced Serial2 initialization logging**
  - Shows baud rate and pin configuration on startup
  
- ✅ **Added detailed debug timestamps**
  - All sent/received messages now show `[millis()]` timestamp
  - Makes it easier to track timing issues

- ✅ **Improved ARDUINO_READY detection**
  - Now recognizes when Arduino sends initial ready message

## Hardware Connection Verification

### Verify These Connections:

**ESP32 ↔ Arduino (Serial2 Communication)**
```
ESP32 TX (GPIO 21)  → Arduino RX (Pin 9)
ESP32 RX (GPIO 22)  ← Arduino TX (Pin 8)
ESP32 GND           → Arduino GND
```

### Baud Rate Configuration
- Both devices: **9600 baud**
- Data: 8 bits
- Parity: None (N)
- Stop bits: 1

## Testing Procedure

### Step 1: Test Arduino Alone
1. Upload `arduino_main.ino` to Arduino
2. Open Serial Monitor (9600 baud)
3. Look for output similar to:
```
Arduino Noodle Vending Ready
Waiting for ESP32...
ARDUINO_READY
=== Arduino Configuration ===
RX Pin: 8
TX Pin: 9
Relay Pin: 7
Ultrasonic Trig: 2
Ultrasonic Echo: 6
L298N Enable: 11
L298N IN1: 3
L298N IN2: 4
Buzzer Pin: 10
Baud Rate: 9600
```

### Step 2: Test ESP32 Alone
1. Upload `esp32_main.ino` to ESP32
2. Open Serial Monitor (115200 baud)
3. Look for output showing WiFi and MQTT connections
4. Should see: `Serial2 initialized for Arduino communication`

### Step 3: Test Serial Communication
1. Start Arduino first (opens serial port)
2. Start ESP32
3. In ESP32 Serial Monitor (115200 baud), send: `test_dispensing`
4. Watch both monitors for message exchanges

### Step 4: Manual Arduino Test (No WiFi)
1. In Arduino Serial Monitor, type: `test_rx`
2. Arduino should send a "TEST" message to ESP32
3. ESP32 Serial Monitor should show: `[xxxx] Received from Arduino: "TEST"`

## Expected Communication Flow

### Normal Dispensing Sequence:
```
ESP32: [12345] Sent to Arduino: "DISPENSING"
Arduino: [12350] Received from ESP32: "DISPENSING"
Arduino: [12400] Starting drop detection...
(Arduino detects noodle drop)
Arduino: [15000] Sent to ESP32: "DROP_DETECTED"
ESP32: [15005] Received from Arduino: "DROP_DETECTED"
ESP32: [15010] Sent to Arduino: "START_HEATING"
Arduino: [15015] Received from ESP32: "START_HEATING"
Arduino: [15020] Starting heating/water pump...
(Heating for ~10 minutes)
Arduino: [615020] Sent to ESP32: "HEATING_COMPLETE"
ESP32: [615025] Received from Arduino: "HEATING_COMPLETE"
```

## Troubleshooting

### Issue: Arduino shows no data from ESP32
**Possible causes:**
1. TX/RX pins are swapped
2. Baud rate mismatch
3. Loose connections
4. Arduino RX pin (8) is not configured correctly

**Solution:**
- Check wiring: ESP32 pin 21 → Arduino pin 9 (TX direction)
- Verify both devices use 9600 baud
- Test with a simple test sketch sending "TEST" message

### Issue: ESP32 doesn't see Arduino messages
**Possible causes:**
1. Serial2 not initialized properly
2. Arduino TX pin (9) not working
3. Wire quality issues

**Solution:**
- In Arduino, upload working test code provided initially
- Monitor Serial (9600) on Arduino and Serial2 output on ESP32

### Issue: Garbled characters in serial output
**Likely cause:** Baud rate mismatch

**Solution:**
- Arduino must use 9600 baud for espSerial
- ESP32 must use 9600 baud for Serial2
- Check serial monitor baud rate matches (115200 for ESP32 debug, 9600 for Arduino debug)

### Issue: Commands sent but no response
**Check:**
1. Are `Serial2.println()` and `espSerial.println()` being called?
2. Is the message being sent before Arduino setup completes?
3. Try adding 1-2 second delay after serial initialization

## Serial Debug Commands

### Arduino Serial Monitor (9600 baud):
```
test_rx     - Test communication by sending "TEST" to ESP32
stop        - Trigger emergency stop
```

### Look for in logs:
- `[millis()]` timestamps show when messages sent/received
- `"ARDUINO_READY"` indicates Arduino initialized
- `"DISPENSING"` command sent from ESP32
- `"DROP_DETECTED"` message from Arduino
- `"START_HEATING"` command to start heating
- `"HEATING_COMPLETE"` when done

## Important Notes

1. **Initialization Order**: Arduino should be powered up before ESP32 for cleanest startup
2. **Power Supply**: Ensure stable 5V for Arduino and 3.3V for ESP32
3. **Ground Connection**: MUST have common ground between devices
4. **Serial Port**: Arduino SoftwareSerial uses GPIO 8 & 9, not hardware serial

## Working Test Code Reference

The working test code provided at startup demonstrates:
- Proper Serial2 initialization on ESP32 (baud 9600, pins 21/22)
- Proper SoftwareSerial initialization on Arduino (pins 8/9, baud 9600)
- Simple command/response protocol
- No MQTT/WiFi complexity

If communication fails with main code, revert to the simple test code to isolate the issue.
