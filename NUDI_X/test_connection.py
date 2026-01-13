import requests
import json

# Test the backend
try:
    response = requests.get("http://127.0.0.1:8000/")
    print("✅ Backend response:", response.json())
except Exception as e:
    print("❌ Backend not reachable:", e)

# Test the status endpoint
try:
    response = requests.get("http://127.0.0.1:8000/status")
    print("✅ Status endpoint:", response.json())
except Exception as e:
    print("❌ Status endpoint error:", e)

# Test CORS
try:
    response = requests.get("http://127.0.0.1:8000/test-cors")
    print("✅ CORS test:", response.json())
except Exception as e:
    print("❌ CORS test error:", e)