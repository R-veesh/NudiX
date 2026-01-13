# setup.py - Run this first
import subprocess
import sys
import os

def install_requirements():
    print("Installing requirements...")
    subprocess.check_call([sys.executable, "-m", "pip", "install", "-r", "requirements.txt"])
    print("Requirements installed successfully!")

def create_env_file():
    env_content = """# Noodle Vending Machine Configuration
MQTT_BROKER=broker.hivemq.com
MQTT_PORT=1883
WIFI_SSID=YOUR_WIFI_SSID
WIFI_PASSWORD=YOUR_WIFI_PASSWORD
ESP32_PORT=/dev/ttyUSB0  # Linux/Mac
# ESP32_PORT=COM3  # Windows
"""
    
    if not os.path.exists(".env"):
        with open(".env", "w") as f:
            f.write(env_content)
        print(".env file created. Please update it with your settings.")
    else:
        print(".env file already exists.")

def main():
    print("=== Noodle Vending Machine Setup ===")
    print("1. Installing Python dependencies...")
    install_requirements()
    
    print("\n2. Creating environment file...")
    create_env_file()
    
    print("\n3. Setup complete!")
    print("\nNext steps:")
    print("  1. Update the .env file with your WiFi credentials")
    print("  2. Upload ESP32 and Arduino code to your devices")
    print("  3. Run the server: uvicorn main:app --reload --host 0.0.0.0 --port 8000")
    print("  4. Open index.html in your browser")
    print("\nFor testing without hardware:")
    print("  - Use the test buttons in the web interface")
    print("  - The system will work in simulation mode")

if __name__ == "__main__":
    main()