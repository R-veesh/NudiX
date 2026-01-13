# main.py (UPDATED with better MQTT and error handling)
from fastapi import FastAPI, HTTPException, BackgroundTasks
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
import paho.mqtt.client as mqtt
import json
import asyncio
import threading
import time
from datetime import datetime
import logging

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = FastAPI(title="Noodle AI Agent - IoT Vending Machine")

# MQTT Configuration - USE SAME BROKER AS ESP32
MQTT_BROKER = "broker.hivemq.com"  # Public broker, works better than EMQX
MQTT_PORT = 1883
MQTT_TOPIC_COMMAND = "noodle_vending/command"
MQTT_TOPIC_STATUS = "noodle_vending/status"
MQTT_TOPIC_LOG = "noodle_vending/log"

# Global state
mqtt_client = None
mqtt_connected = False
device_status = "disconnected"
last_status_update = None
system_logs = []
active_connections = []

# MQTT Callbacks
def on_connect(client, userdata, flags, rc):
    global mqtt_connected
    if rc == 0:
        logger.info("✅ Connected to MQTT Broker!")
        mqtt_connected = True
        client.subscribe(MQTT_TOPIC_STATUS)
        client.subscribe(MQTT_TOPIC_LOG)
        # Publish initial status request
        client.publish(MQTT_TOPIC_COMMAND, "status")
    else:
        logger.error(f"❌ Failed to connect to MQTT, return code {rc}")
        mqtt_connected = False

def on_disconnect(client, userdata, rc):
    global mqtt_connected
    logger.warning(f"MQTT disconnected, rc={rc}")
    mqtt_connected = False

def on_message(client, userdata, msg):
    global device_status, last_status_update, system_logs
    
    try:
        topic = msg.topic
        payload = msg.payload.decode()
        timestamp = datetime.now().strftime("%H:%M:%S")
        
        logger.info(f"📨 MQTT: [{topic}] {payload}")
        
        if topic == MQTT_TOPIC_STATUS:
            device_status = payload
            last_status_update = time.time()
            
            # Log status changes
            if "dispensing" in payload or "ready" in payload or "busy" in payload:
                add_log("DEVICE", payload)
                
        elif topic == MQTT_TOPIC_LOG:
            add_log("SYSTEM", payload)
            
    except Exception as e:
        logger.error(f"Error processing MQTT message: {e}")

def add_log(log_type, message):
    """Add log entry and broadcast to connected clients"""
    timestamp = datetime.now().strftime("%H:%M:%S")
    log_entry = f"[{timestamp}] [{log_type}] {message}"
    system_logs.append(log_entry)
    
    # Keep only last 50 logs
    if len(system_logs) > 50:
        system_logs.pop(0)
    
    # Broadcast to WebSocket connections (if implemented)
    for connection in active_connections:
        try:
            connection.put_nowait(log_entry)
        except:
            pass

def connect_mqtt():
    """Connect to MQTT broker in background thread"""
    global mqtt_client
    
    mqtt_client = mqtt.Client(client_id=f"noodle-server-{int(time.time())}")
    mqtt_client.on_connect = on_connect
    mqtt_client.on_disconnect = on_disconnect
    mqtt_client.on_message = on_message
    
    # Set last will testament
    mqtt_client.will_set(MQTT_TOPIC_STATUS, "server_disconnected", qos=1, retain=True)
    
    try:
        mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
        mqtt_client.loop_start()
        logger.info("MQTT connection started")
        return True
    except Exception as e:
        logger.error(f"Cannot connect to MQTT: {e}")
        return False

def mqtt_publish(topic, message, qos=1):
    """Publish message to MQTT with error handling"""
    if mqtt_connected and mqtt_client:
        try:
            result = mqtt_client.publish(topic, message, qos=qos, retain=False)
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                logger.info(f"📤 Published to {topic}: {message}")
                return True
            else:
                logger.error(f"Failed to publish to {topic}, rc={result.rc}")
                return False
        except Exception as e:
            logger.error(f"Exception publishing to MQTT: {e}")
            return False
    else:
        logger.warning("MQTT not connected, cannot publish")
        return False

# Start MQTT connection on startup
@app.on_event("startup")
async def startup_event():
    logger.info("🚀 Starting Noodle Vending Machine Server...")
    
    # Connect to MQTT in background
    mqtt_thread = threading.Thread(target=connect_mqtt, daemon=True)
    mqtt_thread.start()
    
    # Wait for MQTT connection
    for i in range(10):
        if mqtt_connected:
            break
        logger.info(f"Waiting for MQTT connection... ({i+1}/10)")
        await asyncio.sleep(1)
    
    if mqtt_connected:
        logger.info("✅ Server startup complete!")
    else:
        logger.warning("⚠️ Server started but MQTT not connected")

# CORS middleware
origins = [
    "http://localhost",
    "http://localhost:5500",
    "http://127.0.0.1:5500",
    "http://localhost:8000",
    "http://127.0.0.1:8000",
    "http://localhost:3000",
    "http://127.0.0.1:3000",
    "*"  # For development only
]

app.add_middleware(
    CORSMiddleware, 
    allow_origins=origins,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Pydantic models
class ChatRequest(BaseModel):
    user_message: str

class ManualDispenseRequest(BaseModel):
    noodle_number: int

# Noodle mapping
NOODLE_MAP = {
    "Hot Spicy Ramen": "noodle_1",
    "Chicken Noodles": "noodle_2",
    "Cheese Noodles": "noodle_3",
    "Veg Clear Soup": "noodle_4"
}

NOODLE_REVERSE_MAP = {v: k for k, v in NOODLE_MAP.items()}

# AI Model Import (with fallback)
try:
    from ai_model import get_ai_reply
    AI_ENABLED = True
    logger.info("✅ AI model loaded successfully")
except ImportError as e:
    logger.warning(f"⚠️ AI model not available: {e}")
    AI_ENABLED = False
    
    # Mock AI function
    def get_ai_reply(user_message: str) -> str:
        user_lower = user_message.lower()
        
        if "spicy" in user_lower or "hot" in user_lower:
            return "I'll prepare Hot Spicy Ramen for you! This is our spiciest option."
        elif "chicken" in user_lower:
            return "I'll prepare Chicken Noodles for you! This has tender chicken pieces."
        elif "cheese" in user_lower or "creamy" in user_lower:
            return "I'll prepare Cheese Noodles for you! This is our creamiest option."
        elif "vegetarian" in user_lower or "veg" in user_lower or "light" in user_lower:
            return "I'll prepare Veg Clear Soup for you! This is a light vegetarian soup."
        elif "test" in user_lower:
            return "I'll prepare Hot Spicy Ramen for you! This is our test option."
        else:
            return "I'll prepare Hot Spicy Ramen for you! This is our most popular option."

# API Routes
@app.get("/")
def read_root():
    return {
        "message": "Noodle Vending Machine API",
        "status": "running",
        "mqtt_connected": mqtt_connected,
        "device_status": device_status,
        "ai_enabled": AI_ENABLED,
        "timestamp": datetime.now().isoformat()
    }

@app.get("/status")
def get_status():
    return {
        "device_status": device_status,
        "mqtt_connected": mqtt_connected,
        "last_update": last_status_update,
        "timestamp": time.time()
    }

@app.get("/logs")
def get_logs():
    return {
        "logs": system_logs[-20:],  # Return last 20 logs
        "count": len(system_logs)
    }

@app.post("/chat")
async def chat_agent(req: ChatRequest):
    try:
        # Get AI response
        ai_reply = get_ai_reply(req.user_message)
        
        # Extract noodle selection from AI response
        selected_noodle = None
        for noodle_name, noodle_code in NOODLE_MAP.items():
            if noodle_name.lower() in ai_reply.lower():
                selected_noodle = (noodle_name, noodle_code)
                break
        
        response_data = {
            "reply": ai_reply,
            "device_status": device_status,
            "timestamp": datetime.now().isoformat()
        }
        
        # If a noodle is selected and device is ready, send command
        if selected_noodle and device_status == "ready":
            noodle_name, noodle_code = selected_noodle
            
            # Add log
            add_log("ORDER", f"User ordered: {noodle_name}")
            
            if mqtt_publish(MQTT_TOPIC_COMMAND, noodle_code):
                response_data["action"] = f"Dispensing {noodle_name}"
                response_data["command_sent"] = noodle_code
                response_data["success"] = True
            else:
                response_data["error"] = "Failed to send command to device"
                response_data["success"] = False
        elif selected_noodle:
            response_data["warning"] = f"Device is busy ({device_status}), please wait"
            response_data["success"] = False
        else:
            response_data["info"] = "No noodle selected from your request"
            response_data["success"] = False
        
        return response_data
        
    except Exception as e:
        logger.error(f"Error in chat endpoint: {e}")
        return {
            "error": str(e),
            "reply": "Sorry, I encountered an error processing your request.",
            "success": False
        }

@app.post("/manual_dispense")
async def manual_dispense(req: ManualDispenseRequest):
    try:
        noodle_number = req.noodle_number
        
        if 1 <= noodle_number <= 4:
            command = f"noodle_{noodle_number}"
            noodle_name = NOODLE_REVERSE_MAP.get(command, f"Noodle {noodle_number}")
            
            # Add log
            add_log("MANUAL", f"Manual dispense: {noodle_name}")
            
            if mqtt_publish(MQTT_TOPIC_COMMAND, command):
                return {
                    "success": True,
                    "message": f"Command sent: {command}",
                    "noodle_name": noodle_name,
                    "timestamp": datetime.now().isoformat()
                }
            else:
                return {
                    "success": False,
                    "message": "Failed to send command to device",
                    "timestamp": datetime.now().isoformat()
                }
        else:
            return {
                "success": False,
                "message": "Invalid noodle number (must be 1-4)",
                "timestamp": datetime.now().isoformat()
            }
            
    except Exception as e:
        logger.error(f"Error in manual dispense: {e}")
        return {
            "success": False,
            "message": f"Error: {str(e)}"
        }

@app.post("/emergency_stop")
async def emergency_stop():
    """Emergency stop all operations"""
    try:
        if mqtt_publish(MQTT_TOPIC_COMMAND, "emergency_stop"):
            add_log("EMERGENCY", "Emergency stop activated")
            return {
                "success": True,
                "message": "Emergency stop command sent",
                "timestamp": datetime.now().isoformat()
            }
        else:
            return {
                "success": False,
                "message": "Failed to send emergency stop"
            }
    except Exception as e:
        logger.error(f"Error in emergency stop: {e}")
        return {
            "success": False,
            "message": f"Error: {str(e)}"
        }

@app.post("/test_motor/{motor_number}")
async def test_motor(motor_number: int):
    """Test a specific motor"""
    try:
        if 1 <= motor_number <= 4:
            command = f"test_motor_{motor_number}"
            if mqtt_publish(MQTT_TOPIC_COMMAND, command):
                add_log("TEST", f"Testing motor {motor_number}")
                return {
                    "success": True,
                    "message": f"Test command sent for motor {motor_number}"
                }
        return {
            "success": False,
            "message": "Invalid motor number (1-4)"
        }
    except Exception as e:
        return {
            "success": False,
            "message": f"Error: {str(e)}"
        }

@app.get("/system_info")
async def system_info():
    """Get complete system information"""
    return {
        "server": {
            "status": "running",
            "uptime": "N/A",  # Could implement with startup time
            "ai_enabled": AI_ENABLED,
            "timestamp": datetime.now().isoformat()
        },
        "mqtt": {
            "connected": mqtt_connected,
            "broker": MQTT_BROKER,
            "port": MQTT_PORT
        },
        "device": {
            "status": device_status,
            "last_update": last_status_update
        },
        "noodles": [
            {"id": 1, "name": "Hot Spicy Ramen", "code": "noodle_1"},
            {"id": 2, "name": "Chicken Noodles", "code": "noodle_2"},
            {"id": 3, "name": "Cheese Noodles", "code": "noodle_3"},
            {"id": 4, "name": "Veg Clear Soup", "code": "noodle_4"}
        ]
    }

# Background task to periodically check device status
async def status_checker():
    """Periodically check device status"""
    while True:
        if mqtt_connected:
            mqtt_publish(MQTT_TOPIC_COMMAND, "status")
        await asyncio.sleep(10)  # Check every 10 seconds

# Start background task
@app.on_event("startup")
async def start_background_tasks():
    asyncio.create_task(status_checker())