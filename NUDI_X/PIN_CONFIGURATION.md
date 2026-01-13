# 🔌 Pin Configuration Reference

## ESP32 GPIO Pin Assignments

```
┌─────────────────────────────────────────────────────────┐
│          ESP32 NOODLE VENDING MACHINE                   │
│             Pin Configuration (UPDATED)                 │
└─────────────────────────────────────────────────────────┘

═══════════════════════════════════════════════════════════
│ ULTRASONIC SENSOR (NEW - Replaces IR Sensors)           │
═══════════════════════════════════════════════════════════
  GPIO 34  →  HC-SR04 Trigger Pin  (TRIG)
  GPIO 35  →  HC-SR04 Echo Pin     (ECHO)
  GND      →  HC-SR04 Ground
  5V/3.3V  →  HC-SR04 VCC

═══════════════════════════════════════════════════════════
│ STEPPER MOTOR 1 (Noodle 1)                              │
═══════════════════════════════════════════════════════════
  GPIO 19  →  IN1 (ULN2003 A)
  GPIO 18  →  IN2 (ULN2003 B)
  GPIO 5   →  IN3 (ULN2003 C)
  GPIO 17  →  IN4 (ULN2003 D)

═══════════════════════════════════════════════════════════
│ STEPPER MOTOR 2 (Noodle 2)                              │
═══════════════════════════════════════════════════════════
  GPIO 16  →  IN1 (ULN2003 A)
  GPIO 4   →  IN2 (ULN2003 B)
  GPIO 2   →  IN3 (ULN2003 C)
  GPIO 15  →  IN4 (ULN2003 D)

═══════════════════════════════════════════════════════════
│ STEPPER MOTOR 3 (Noodle 3)                              │
═══════════════════════════════════════════════════════════
  GPIO 13  →  IN1 (ULN2003 A)
  GPIO 12  →  IN2 (ULN2003 B)
  GPIO 14  →  IN3 (ULN2003 C)
  GPIO 27  →  IN4 (ULN2003 D)

═══════════════════════════════════════════════════════════
│ STEPPER MOTOR 4 (Noodle 4)                              │
═══════════════════════════════════════════════════════════
  GPIO 26  →  IN1 (ULN2003 A)
  GPIO 25  →  IN2 (ULN2003 B)
  GPIO 33  →  IN3 (ULN2003 C)
  GPIO 32  →  IN4 (ULN2003 D)

═══════════════════════════════════════════════════════════
│ SERIAL COMMUNICATION (Arduino)                          │
═══════════════════════════════════════════════════════════
  GPIO 26  →  RX (Receive from Arduino TX)
  GPIO 25  →  TX (Send to Arduino RX)
  Baud Rate: 9600 bps

═══════════════════════════════════════════════════════════
│ STATUS LED                                              │
═══════════════════════════════════════════════════════════
  GPIO 2   →  LED_BUILTIN (Status indicator)
  High = Connected, Low = Disconnected

═══════════════════════════════════════════════════════════
│ SPECIAL PINS (Don't use)                                │
═══════════════════════════════════════════════════════════
  GPIO 0   →  Boot mode selection
  GPIO 6-11→  SPI Flash (Reserved)
  GPIO 34-39→ Input only (No output)
```

---

## Pin Change Summary

| Function | Old Pins | New Pins | Note |
|----------|----------|----------|------|
| IR Sensor 1 | GPIO 34 | Removed | → Ultrasonic TRIG |
| IR Sensor 2 | GPIO 35 | Removed | → Ultrasonic ECHO |
| Ultrasonic TRIG | N/A | GPIO 34 | New |
| Ultrasonic ECHO | N/A | GPIO 35 | New |
| Arduino RX | GPIO 34 | GPIO 26 | Relocated |
| Arduino TX | GPIO 35 | GPIO 25 | Relocated |

---

## Hardware Connection Diagram

```
ESP32 DevKit
┌──────────────────────────────────────┐
│  3.3V  GND  TX2  RX2  GPIO26 GPIO25  │ (Top)
│   │     │              │       │
│   │     │              └──┬────┘
│   │     └────────────┐    │
│   │                  │    └──→ Arduino (UART)
│   │                  │
│  GND (shared with all)
│
│  GPIO34(TRIG)  GPIO35(ECHO)         │
│   │             │
│   └─HC-SR04─────┘
│   [Ultrasonic Sensor]
│
│  GPIO19-17,16-15,13-27,26-32        │
│   │
│   ├─ ULN2003 Driver (Motor 1)
│   ├─ ULN2003 Driver (Motor 2)
│   ├─ ULN2003 Driver (Motor 3)
│   └─ ULN2003 Driver (Motor 4)
│
└──────────────────────────────────────┘
         ↓
      28BYJ-48 Stepper Motors
```

---

## Ultrasonic Sensor Specifications

**HC-SR04 Module**:
- **Voltage**: 5V or 3.3V (use 3.3V for ESP32)
- **Frequency**: 40 kHz
- **Range**: 2cm - 400cm
- **Accuracy**: ±3mm
- **Measurement Angle**: 30°
- **Trigger Pulse Width**: 10μs
- **Echo Output**: HIGH for (distance × 58μs)

**Threshold Setting**:
```cpp
#define DISTANCE_THRESHOLD 23  // cm - adjust based on test
```

**Expected Readings**:
- No obstacle: 150+ cm
- 1 meter away: ~100 cm
- 50 cm away: ~50 cm
- 30 cm away: ~30 cm
- **Noodle drops**: < 23 cm ✅

---

## Power Distribution

```
5V Supply
│
├─→ ULN2003 (Motors) - Stepper coil supply
├─→ HC-SR04 VCC (if using 5V version)
└─→ ESP32 (via USB or regulated 3.3V)

3.3V Supply (ESP32 internal)
│
├─→ HC-SR04 VCC (if using 3.3V version)
├─→ GPIO Logic (34, 35, etc.)
└─→ LED_BUILTIN

GND (Common)
│
├─→ All devices must share common ground
├─→ ULN2003 GND
├─→ HC-SR04 GND
├─→ Arduino GND
└─→ Motor GND
```

---

## Testing the Pins

### Verify Ultrasonic Sensor
```cpp
// Add this to setup() for testing:
void testUltrasonic() {
  for (int i = 0; i < 10; i++) {
    float distance = getUltrasonicDistance();
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
    delay(500);
  }
}
```

### Verify Stepper Motors
```cpp
// Add this to test motor 1:
void testStepper() {
  Serial.println("Motor 1 forward:");
  stepper1.step(500);
  delay(1000);
  
  Serial.println("Motor 1 backward:");
  stepper1.step(-500);
  delay(1000);
}
```

### Verify Serial Communication
```cpp
// Add this to loop():
if (Serial2.available()) {
  String msg = Serial2.readStringUntil('\n');
  Serial.println("From Arduino: " + msg);
}
```

---

## Recommended Sensor Placement

```
┌─────────────────────────────┐
│  Noodle Dispenser Box       │
│                             │
│  [Noodle Container]         │
│           ↓                 │
│   ┌───────────────┐         │
│   │    Chute      │  ← 23cm │
│   │               │         │
│   │   ← SENSOR    │         │
│   │    (23cm away)│         │
│   │               │         │
│   │  [Catch Tray] │         │
│   │               │         │
│   └───────────────┘         │
└─────────────────────────────┘

Sensor Position: 23cm from noodle path
When noodle drops: distance drops below 23cm
Detection: ✅ DROP DETECTED!
```

---

## Common Issues & Solutions

| Issue | Cause | Solution |
|-------|-------|----------|
| No distance readings | Sensor not connected | Check GPIO 34, 35 connections |
| Always > 150cm | Sensor not working | Check 3.3V power to sensor |
| Erratic readings | Poor wiring | Use shielded cable, short traces |
| No drop detection | Threshold wrong | Adjust `DISTANCE_THRESHOLD` value |
| Serial comm fails | Wrong pins | Verify GPIO 26 (RX), 25 (TX) |
| Motors don't move | Pin config wrong | Check stepper motor pin order |

---

**Last Updated**: January 14, 2026
**Version**: 2.1 (With Ultrasonic Sensor)
