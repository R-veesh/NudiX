// esp32_main.ino
#include <WiFi.h>
#include <PubSubClient.h>
#include <Stepper.h>

// WiFi Credentials
const char* ssid = "Your_WiFi_SSID";
const char* password = "Your_WiFi_Password";

// MQTT Configuration
const char* mqtt_server = "broker.emqx.io"; // or your local MQTT broker
const char* mqtt_topic = "noodle_vending/command";
const char* mqtt_status = "noodle_vending/status";
const char* mqtt_drop = "noodle_vending/drop_detected";

// Stepper Motor Pins (using ULN2003)
#define IN1_1 19
#define IN2_1 18
#define IN3_1 5
#define IN4_1 17

#define IN1_2 16
#define IN2_2 4
#define IN3_2 2
#define IN4_2 15

#define IN1_3 13
#define IN2_3 12
#define IN3_3 14
#define IN4_3 27

#define IN1_4 26
#define IN2_4 25
#define IN3_4 33
#define IN4_4 32

// Steps per revolution for 28BYJ-48
const int STEPS_PER_REV = 2038;
Stepper stepper1(STEPS_PER_REV, IN1_1, IN3_1, IN2_1, IN4_1);
Stepper stepper2(STEPS_PER_REV, IN1_2, IN3_2, IN2_2, IN4_2);
Stepper stepper3(STEPS_PER_REV, IN1_3, IN3_3, IN2_3, IN4_3);
Stepper stepper4(STEPS_PER_REV, IN1_4, IN3_4, IN2_4, IN4_4);

// Communication with Arduino
#define ARDUINO_RX 34  // ESP32 RX
#define ARDUINO_TX 35  // ESP32 TX

WiFiClient espClient;
PubSubClient mqttClient(espClient);
String currentCommand = "";
bool dispensing = false;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, ARDUINO_RX, ARDUINO_TX);
  
  // Initialize stepper motor speeds
  stepper1.setSpeed(10);
  stepper2.setSpeed(10);
  stepper3.setSpeed(10);
  stepper4.setSpeed(10);
  
  setupWiFi();
  setupMQTT();
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  Serial.println("ESP32 Noodle Vending Machine Ready");
  mqttPublish(mqtt_status, "ready");
}

void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setupMQTT() {
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(mqttCallback);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.println(message);
  
  if (String(topic) == mqtt_topic) {
    handleCommand(message);
  }
}

void handleCommand(String cmd) {
  if (dispensing) {
    mqttPublish(mqtt_status, "busy");
    return;
  }
  
  cmd.trim();
  Serial.println("Processing command: " + cmd);
  
  if (cmd == "noodle_1") {
    dispenseNoodle(1);
  } else if (cmd == "noodle_2") {
    dispenseNoodle(2);
  } else if (cmd == "noodle_3") {
    dispenseNoodle(3);
  } else if (cmd == "noodle_4") {
    dispenseNoodle(4);
  } else if (cmd == "status") {
    mqttPublish(mqtt_status, dispensing ? "busy" : "ready");
  }
}

void dispenseNoodle(int noodleNumber) {
  dispensing = true;
  Serial.println("Dispensing Noodle " + String(noodleNumber));
  mqttPublish(mqtt_status, "dispensing_noodle_" + String(noodleNumber));
  
  // Activate corresponding stepper motor
  switch (noodleNumber) {
    case 1:
      stepper1.step(STEPS_PER_REV / 2); // Half revolution for dispensing
      break;
    case 2:
      stepper2.step(STEPS_PER_REV / 2);
      break;
    case 3:
      stepper3.step(STEPS_PER_REV / 2);
      break;
    case 4:
      stepper4.step(STEPS_PER_REV / 2);
      break;
  }
  
  delay(100);
  
  // Signal Arduino to prepare for drop detection
  Serial2.println("DISPENSING");
  
  // Wait for drop detection (timeout after 10 seconds)
  unsigned long startTime = millis();
  bool dropDetected = false;
  
  while (millis() - startTime < 10000) {
    if (Serial2.available()) {
      String response = Serial2.readStringUntil('\n');
      response.trim();
      if (response == "DROP_DETECTED") {
        dropDetected = true;
        break;
      }
    }
    delay(50);
  }
  
  if (dropDetected) {
    Serial.println("Drop detected successfully!");
    mqttPublish(mqtt_drop, "success_noodle_" + String(noodleNumber));
    
    // Signal Arduino to start heating/water pump
    Serial2.println("START_HEATING");
    
    // Wait for completion signal from Arduino
    startTime = millis();
    while (millis() - startTime < 650000) { // ~11 minutes timeout
      if (Serial2.available()) {
        String response = Serial2.readStringUntil('\n');
        response.trim();
        if (response == "HEATING_COMPLETE") {
          break;
        }
      }
      delay(100);
    }
  } else {
    Serial.println("Drop detection timeout!");
    mqttPublish(mqtt_drop, "timeout_noodle_" + String(noodleNumber));
  }
  
  dispensing = false;
  mqttPublish(mqtt_status, "ready");
}

void mqttPublish(const char* topic, String message) {
  if (mqttClient.connected()) {
    mqttClient.publish(topic, message.c_str());
    Serial.println("Published to " + String(topic) + ": " + message);
  }
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32-NoodleVending-";
    clientId += String(random(0xffff), HEX);
    
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
      mqttClient.subscribe(mqtt_topic);
      mqttPublish(mqtt_status, "reconnected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();
  
  // Check for messages from Arduino
  if (Serial2.available()) {
    String message = Serial2.readStringUntil('\n');
    message.trim();
    Serial.println("From Arduino: " + message);
    
    if (message == "DROP_DETECTED") {
      mqttPublish(mqtt_drop, "detected");
    }
  }
  
  delay(10);
}