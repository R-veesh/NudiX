# Pre-Upload Verification Checklist

## ✅ Code Compilation Check

Before uploading, verify these fixes are in place:

### Arduino Fixes
- [ ] Line 12: `#define L298N_ENA 11` exists
- [ ] Line 34: `pinMode(L298N_ENA, OUTPUT);` can now reference L298N_ENA
- [ ] `startDropDetection()` no longer initializes undefined `dropSensorCount`
- [ ] Serial comments show correct direction (RX ← from ESP32, TX → to ESP32)
- [ ] `test_rx` command available in loop for testing

### ESP32 Fixes
- [ ] Line 76: "Serial2 initialized for Arduino communication" message in setup
- [ ] Timestamp logging present in all Serial2 commands
- [ ] `dispenseNoodle()` function includes timestamp debug output
- [ ] `emergencyStop()` includes "Sent to Arduino" debug message

## 📡 Hardware Pre-Check

Before connecting devices:

**Verify Wiring:**
- [ ] ESP32 GPIO 21 → Arduino Pin 9
- [ ] ESP32 GPIO 22 ← Arduino Pin 8  
- [ ] Common GND connection present
- [ ] No loose or damaged wires
- [ ] Connections not reversed

**Power Configuration:**
- [ ] Arduino: 5V power supply
- [ ] ESP32: 3.3V power supply
- [ ] Both have stable power
- [ ] Ground connection is secure

## 🔧 Upload Procedure

### For Arduino:
1. [ ] Select correct board (Arduino Uno/Nano/etc)
2. [ ] Select correct COM port
3. [ ] Baud rate: 9600 (shown in sketch)
4. [ ] Upload `arduino_main.ino`
5. [ ] Wait for "Upload complete" message
6. [ ] Close Serial Monitor (if open)

### For ESP32:
1. [ ] Select board: ESP32 Dev Module
2. [ ] Select correct COM port (different from Arduino)
3. [ ] Baud rate: 115200 (shown in sketch)
4. [ ] Upload `esp32_main.ino`
5. [ ] Wait for "Upload complete" message

## 📊 Post-Upload Testing

### Step 1: Arduino Startup (9600 baud)
```
Expected output:
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

### Step 2: ESP32 Startup (115200 baud)
```
Expected output:
=== ESP32 Noodle Vending Machine Starting ===
Serial2 initialized for Arduino communication
Serial2 Baud Rate: 9600 | RX Pin: 22 | TX Pin: 21

(Then WiFi and MQTT connection messages)
```

### Step 3: Verify Communication
Open both Serial Monitors simultaneously:
1. [ ] Both show startup messages
2. [ ] Messages appear without garbled characters
3. [ ] Timestamps are visible in debug messages
4. [ ] No compilation errors in Arduino IDE

## 🧪 Manual Test (Optional)

In Arduino Serial Monitor (9600 baud), type:
```
test_rx
```

Expected result:
- Arduino sends: `[xxxx] Sent to Arduino: "TEST"`
- ESP32 shows: `[yyyy] Received from Arduino: "TEST"`

## ⚠️ Common Issues & Quick Fixes

| Issue | Check |
|-------|-------|
| Arduino won't compile | Verify L298N_ENA is defined on line 12 |
| ESP32 won't upload | Check board selection is "ESP32 Dev Module" |
| Garbled text in serial | Verify baud rates (Arduino: 9600, ESP32: 115200) |
| No "ARDUINO_READY" on ESP32 | Check wiring, especially GPIO 22 (RX) |
| Arduino shows no messages from ESP32 | Check ESP32 GPIO 21 connects to Arduino Pin 9 |
| Both show no debug messages | Verify Serial Monitor baud rate matches |

## 📝 Documentation Files

Read these after fixing to understand the system:

- [ ] `FIXES_APPLIED.md` - Summary of all changes made
- [ ] `SERIAL_COMMUNICATION_DEBUG.md` - Complete troubleshooting guide
- [ ] `TESTING_GUIDE.md` - Full testing procedures

---

**Status**: Ready for upload ✅

All code has been fixed and enhanced with proper debugging.
Proceed with uploading to your devices!
