# 🧪 Testing & Debugging Guide

## Part 1: Serial Monitor Setup

### ESP32 Serial Connection
```
Baud Rate: 115200
Data Bits: 8
Stop Bits: 1
Parity: None
Flow Control: None
```

### Expected Serial Output When Dispensing

```
====== DISPENSING SEQUENCE START ======
Noodle Number: 1
Start Time: 12345
🍜 Dispensing Noodle 1
[DEBUG] Stepper 1 activated - 1019 steps
Sent: DISPENSING to Arduino
📏 Ultrasonic Distance: 50.23 cm
📏 Ultrasonic Distance: 42.15 cm
📏 Ultrasonic Distance: 35.88 cm
📏 Ultrasonic Distance: 28.42 cm
📏 Ultrasonic Distance: 19.05 cm
✅ DROP DETECTED!
✅ Noodle drop confirmed by ultrasonic sensor!
Sent: START_HEATING to Arduino
🔥 Heating in progress...
[00:30] Heating... 540s remaining
[01:00] Heating... 480s remaining
Arduino: HEATING_COMPLETE
✅ Heating process completed!
[DEVICE] Noodle 1 ready!
```

---

## Part 2: Web Browser Debugging

### Open Developer Console
Press **F12** or **Ctrl+Shift+I** and go to **Console** tab

### Test Manual Dispense Button
1. Click "Dispense 1" button
2. Look for console messages:

```javascript
// SUCCESS - You should see:
[MANUAL_DISPENSE] Starting dispense for noodle 1
[MANUAL_DISPENSE] Sending request to /manual_dispense/1
[MANUAL_DISPENSE] Response status: 200
[MANUAL_DISPENSE] Response data: {success: true, message: "Command sent: noodle_1", ...}
[MANUAL_DISPENSE] ✅ Command sent successfully: noodle_1

// FAILURE - Would show:
[MANUAL_DISPENSE] ❌ Fetch error: TypeError: Failed to fetch
// Check if backend is running!
```

### Test Chat Message Button
1. Type: "I want hot spicy ramen"
2. Click "Send"
3. Look for console messages:

```javascript
[SEND_MESSAGE] User message: I want hot spicy ramen
[SEND_MESSAGE] Sending request to /chat...
[SEND_MESSAGE] Response status: 200
[SEND_MESSAGE] Response data: {reply: "I'll prepare Hot Spicy Ramen...", ...}
[SEND_MESSAGE] Action: Dispensing Hot Spicy Ramen
```

---

## Part 3: Backend Testing

### Check if FastAPI is Running
```bash
curl http://127.0.0.1:8000/
```

Expected response:
```json
{
  "message": "Noodle Vending Machine API",
  "status": "running",
  "mqtt_connected": true,
  "device_status": "ready",
  "ai_enabled": true,
  "timestamp": "2026-01-14T12:34:56.789123"
}
```

### View Serial Logs
```bash
curl http://127.0.0.1:8000/serial-logs | python -m json.tool
```

Expected response:
```json
{
  "serial_logs": [
    "[12:34:56] ✅ WiFi connected",
    "[12:34:57] 🔗 Attempting MQTT connection...",
    "[12:34:58] ====== DISPENSING SEQUENCE START ======",
    "[12:34:59] 📏 Ultrasonic Distance: 45.32 cm",
    "[12:35:00] ✅ DROP DETECTED!"
  ],
  "count": 5,
  "total_available": 50,
  "timestamp": "2026-01-14T12:34:59.123456"
}
```

### View All System Logs
```bash
curl http://127.0.0.1:8000/logs | python -m json.tool
```

### Check Device Status
```bash
curl http://127.0.0.1:8000/status | python -m json.tool
```

---

## Part 4: MQTT Testing

### Check MQTT Connection
Use an MQTT client to monitor topics:

**Topic**: `noodle_vending/status`
```
Expected messages:
- "ready"
- "dispensing_noodle_1"
- "dispensing_noodle_2"
- etc.
```

**Topic**: `noodle_vending/command`
```
Expected messages:
- "noodle_1"
- "noodle_2"
- "test_motor_1"
- "status"
- "emergency_stop"
```

**Topic**: `noodle_vending/log`
```
Example messages:
- "Dispensing noodle 1"
- "Drop detected for noodle 1"
- "MQTT connected successfully"
```

**Topic**: `noodle_vending/drop_detected`
```
Expected messages:
- "success_noodle_1"
- "timeout_noodle_2"
```

---

## Part 5: Ultrasonic Sensor Calibration

### Distance Reading Test
The sensor should read:
- **No object**: 150+ cm
- **Hand at arm's length**: 60-80 cm
- **Hand at distance**: 30-40 cm
- **Noodle container**: 25-30 cm (varies by placement)
- **After noodle drops**: < 23 cm ✅ **DETECTION**

### Adjust Threshold if Needed
In [esp32_main.ino](esp32_main.ino), find and modify:
```cpp
#define DISTANCE_THRESHOLD 23  // Change this value
```

Common values:
- `20`: Very sensitive, detects small objects
- `23`: Default, works for noodle boxes
- `25`: Less sensitive, ignores small debris
- `30`: Very loose, any object nearby

---

## Part 6: Troubleshooting

### Problem: "Device is disconnected" on web interface

**Check**:
1. Is MQTT broker reachable?
   ```bash
   ping broker.hivemq.com
   ```
2. Is ESP32 connected to WiFi?
   - Check Serial Monitor for: `✅ WiFi connected`
3. Is MQTT connection established?
   - Look for: `✅ connected` in serial output

### Problem: Web buttons don't work

**Check Console (F12)**:
1. Any red errors?
2. Look for `[MANUAL_DISPENSE]` or `[SEND_MESSAGE]` logs
3. Check response status (should be 200)

**Check Backend**:
1. Is FastAPI running?
   ```bash
   uvicorn main:app --reload
   ```
2. Check for errors in terminal
3. Try curl command:
   ```bash
   curl -X POST http://127.0.0.1:8000/manual_dispense/1
   ```

### Problem: No drop detection

**Check Serial Monitor**:
1. Distance readings appearing?
   - If not: Check sensor wiring
2. Distance numbers reasonable?
   - Too high (150+): No sensor connection
   - Unstable: Poor wiring

**Check Threshold**:
1. Is 23cm correct for your setup?
2. Try placing object and watch distance change
3. Adjust `DISTANCE_THRESHOLD` if needed

### Problem: MQTT not publishing

**Check**:
1. Backend logs for MQTT errors:
   ```bash
   curl http://127.0.0.1:8000/status
   ```
2. Check if `mqtt_connected` is `true`
3. Try manual publish from ESP32 code

---

## Part 7: System Health Check

Run this complete health check:

```bash
# 1. Check backend
echo "=== Backend Status ==="
curl http://127.0.0.1:8000/

# 2. Check MQTT connection
echo "=== Device Status ==="
curl http://127.0.0.1:8000/status

# 3. Check recent logs
echo "=== System Logs ==="
curl http://127.0.0.1:8000/logs

# 4. Check serial logs
echo "=== Serial Logs ==="
curl http://127.0.0.1:8000/serial-logs

# 5. Test MQTT
echo "=== MQTT Test ==="
curl -X POST http://127.0.0.1:8000/test_motor/1
```

---

## Part 8: Performance Monitoring

### Check Serial Log Growth
```bash
# Get count before action
COUNT1=$(curl -s http://127.0.0.1:8000/serial-logs | python -c "import sys, json; print(json.load(sys.stdin)['count'])")

# Do an action (click button)
sleep 2

# Get count after action
COUNT2=$(curl -s http://127.0.0.1:8000/serial-logs | python -c "import sys, json; print(json.load(sys.stdin)['count'])")

# Should increase by 3-5
echo "Logs before: $COUNT1, after: $COUNT2, diff: $((COUNT2 - COUNT1))"
```

---

## Quick Reference

| Command | Purpose |
|---------|---------|
| `F12` | Open Developer Console |
| `F12 → Console` | View JavaScript logs |
| `F12 → Network` | Monitor API calls |
| `curl http://127.0.0.1:8000/serial-logs` | View ESP32 serial output |
| `curl http://127.0.0.1:8000/status` | Check device status |
| `Ctrl+Shift+M` | Toggle mobile view (responsive test) |

---

**Happy Testing! 🍜**
