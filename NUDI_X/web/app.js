// web/app.js
// http://localhost:5500/index.html


// function addMessage(sender, text) {
//   const chatBox = document.getElementById("chatBox");
//   const div = document.createElement("div");
//   div.className = "msg " + sender;
//   div.innerText = text;
//   chatBox.appendChild(div);
//   chatBox.scrollTop = chatBox.scrollHeight;
// }

// async function sendMessage() {
//   const input = document.getElementById("userInput");
//   const message = input.value.trim();
//   if (!message) return;

//   addMessage("user", "You: " + message);
//   input.value = "";

//   try {
//     const res = await fetch("http://127.0.0.1:8000/chat", {
//       method: "POST",
//       headers: { "Content-Type": "application/json" },
//      body: JSON.stringify({
//         user_message: message,  // <-- updated to match FastAPI
//         session_id: sessionId,
//         pending_order: pendingOrder
//       })
//     });

//     if (!res.ok) throw new Error('Network response was not ok');
//     const data = await res.json();
//     addMessage("ai", "AI: " + (data.reply || "(no reply)"));
//   } catch (err) {
//     // log errors for debugging but do not show network errors in the chat UI
//     console.error('sendMessage error:', err);
//   }
// }


// app.js (UPDATED WITH BETTER ERROR HANDLING)
console.log("🍜 Smart Noodle Interface v2.0");

let deviceStatus = "disconnected";
let mqttConnected = false;
let lastUpdate = 0;

// Initialize
document.addEventListener('DOMContentLoaded', function() {
    console.log("✅ Interface loaded");
    
    // Add enhanced status display
    updateConnectionStatus();
    
    // Attach event listeners
    document.getElementById("sendBtn").addEventListener("click", sendMessage);
    document.getElementById("userInput").addEventListener("keydown", function (e) {
        if (e.key === "Enter") sendMessage();
    });
    
    // Quick select buttons
    for (let i = 1; i <= 4; i++) {
        document.getElementById(`quickBtn${i}`).addEventListener("click", () => quickSelect(i));
    }
    
    // Manual control buttons
    for (let i = 1; i <= 4; i++) {
        document.getElementById(`manualBtn${i}`).addEventListener("click", () => manualDispense(i));
    }
    
    // Start status polling
    checkDeviceStatus();
    setInterval(checkDeviceStatus, 3000);
    setInterval(updateConnectionStatus, 1000);
    
    // Add welcome message
    setTimeout(() => {
        addMessage("ai", "🚀 System initialized! You can now order noodles using natural language or the quick buttons.");
    }, 1000);
});

function updateConnectionStatus() {
    const statusElement = document.getElementById('connectionStatus');
    const timeDiff = Date.now() - lastUpdate;
    
    let statusText = "";
    let statusClass = "";
    
    if (deviceStatus === "ready") {
        statusText = "✅ Connected - Ready to serve!";
        statusClass = "connected";
    } else if (deviceStatus === "disconnected" || timeDiff > 15000) {
        statusText = "❌ Disconnected - Check backend server";
        statusClass = "disconnected";
    } else if (deviceStatus.includes("busy") || deviceStatus.includes("dispensing")) {
        statusText = "⏳ " + deviceStatus.replace("_", " ").toUpperCase();
        statusClass = "busy";
    } else {
        statusText = "🔄 " + deviceStatus;
        statusClass = "connecting";
    }
    
    if (statusElement) {
        statusElement.textContent = statusText;
        statusElement.className = "status " + statusClass;
    }
    
    // Update MQTT status
    const mqttElement = document.getElementById('mqttStatus');
    if (mqttElement) {
        mqttElement.textContent = mqttConnected ? "✅ Connected" : "❌ Disconnected";
        mqttElement.className = mqttConnected ? "status connected" : "status disconnected";
    }
}

async function checkDeviceStatus() {
    try {
        const response = await fetch("http://127.0.0.1:8000/status");
        if (!response.ok) throw new Error("Server error");
        
        const data = await response.json();
        deviceStatus = data.device_status || "disconnected";
        mqttConnected = data.mqtt_connected || false;
        lastUpdate = Date.now();
        
        updateDeviceDisplay();
        
    } catch (error) {
        console.log("Could not fetch device status:", error);
        deviceStatus = "disconnected";
        mqttConnected = false;
        updateDeviceDisplay();
    }
}

function updateDeviceDisplay() {
    const elements = {
        'deviceStatus': deviceStatus,
        'mqttStatus': mqttConnected ? "Connected" : "Disconnected",
        'systemStatus': deviceStatus === "ready" ? "Operational" : "Standby"
    };
    
    for (const [id, value] of Object.entries(elements)) {
        const element = document.getElementById(id);
        if (element) {
            element.textContent = value;
            
            // Add appropriate classes
            if (id === 'deviceStatus') {
                element.className = deviceStatus === "ready" ? "status ready" : 
                                  deviceStatus.includes("busy") ? "status busy" : 
                                  "status error";
            }
        }
    }
    
    // Update noodle status indicators
    updateNoodleStatus();
}

function updateNoodleStatus() {
    for (let i = 1; i <= 4; i++) {
        const element = document.getElementById(`noodle${i}Status`);
        if (element) {
            if (deviceStatus === "ready") {
                element.textContent = "READY";
                element.className = "noodle-status ready";
            } else if (deviceStatus.includes(`_${i}`)) {
                element.textContent = "ACTIVE";
                element.className = "noodle-status active";
            } else {
                element.textContent = "STANDBY";
                element.className = "noodle-status standby";
            }
        }
    }
}

function addMessage(sender, text) {
  const chatBox = document.getElementById("chatBox");
  const div = document.createElement("div");
  div.className = "msg " + sender;
  div.innerText = text;
  chatBox.appendChild(div);
  chatBox.scrollTop = chatBox.scrollHeight;
}

function addMessageWithTyping(sender, text, speed = 30) {
  const chatBox = document.getElementById("chatBox");
  const div = document.createElement("div");
  div.className = "msg " + sender;
  div.innerText = ""; // Start with empty text
  chatBox.appendChild(div);
  
  let index = 0;
  
  return new Promise((resolve) => {
    const typeChar = () => {
      if (index < text.length) {
        // Use textContent to properly handle spaces and special characters
        div.textContent = text.substring(0, index + 1);
        index++;
        chatBox.scrollTop = chatBox.scrollHeight;
        setTimeout(typeChar, speed);
      } else {
        resolve();
      }
    };
    typeChar();
  });
}

function showTypingIndicator() {
  const chatBox = document.getElementById("chatBox");
  const indicator = document.createElement('div');
  indicator.className = 'typing-indicator';
  indicator.id = 'typingIndicator';
  indicator.innerHTML = '<span></span><span></span><span></span>';
  chatBox.appendChild(indicator);
  chatBox.scrollTop = chatBox.scrollHeight;
}

function removeTypingIndicator() {
  const indicator = document.getElementById('typingIndicator');
  if (indicator) {
    indicator.remove();
  }
}

async function sendMessage() {
    const input = document.getElementById("userInput");
    const message = input.value.trim();
    if (!message) return;

    addMessage("user", message);
    input.value = "";
    showTypingIndicator();

    try {
        const response = await fetch("http://127.0.0.1:8000/chat", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ user_message: message })
        });

        if (!response.ok) throw new Error(`Server error: ${response.status}`);

        const data = await response.json();
        removeTypingIndicator();
        
        let aiResponse = data.reply || "I didn't understand that.";
        
        // Add additional info
        if (data.action) {
            aiResponse += `\n\n⚡ ${data.action}`;
            showNotification(data.action, "success");
        }
        if (data.warning) {
            aiResponse += `\n\n⚠️ ${data.warning}`;
            showNotification(data.warning, "warning");
        }
        if (data.error) {
            aiResponse += `\n\n❌ ${data.error}`;
            showNotification(data.error, "error");
        }
        
        await addMessageWithTyping("ai", aiResponse.trim(), 30);
        
        // Update status
        if (data.device_status) {
            deviceStatus = data.device_status;
            updateDeviceDisplay();
        }
        if (data.mqtt_connected !== undefined) {
            mqttConnected = data.mqtt_connected;
        }

    } catch (error) {
        console.error("Send error:", error);
        removeTypingIndicator();
        addMessage("ai", `❌ Connection Error: ${error.message}\n\nPlease ensure:\n1. FastAPI server is running (uvicorn main:app)\n2. Port 8000 is available\n3. No firewall blocking connections`);
    }
}

function quickSelect(noodleNumber) {
    const messages = [
        `I want noodle ${noodleNumber}`,
        `Give me option ${noodleNumber}`,
        `I'd like to try number ${noodleNumber}`,
        `Can I have the ${noodleNumber} option?`
    ];
    
    const randomMessage = messages[Math.floor(Math.random() * messages.length)];
    document.getElementById("userInput").value = randomMessage;
    sendMessage();
}

async function manualDispense(noodleNumber) {
    if (deviceStatus !== "ready") {
        showNotification(`Device is ${deviceStatus}. Please wait.`, "warning");
        return;
    }
    
    try {
        const response = await fetch(`http://127.0.0.1:8000/manual_dispense/${noodleNumber}`, {
            method: "POST"
        });
        
        const data = await response.json();
        
        if (data.success) {
            showNotification(`Dispensing noodle ${noodleNumber}...`, "success");
            addMessage("user", `[MANUAL] Dispense noodle ${noodleNumber}`);
            addMessage("ai", `✅ Manual command sent for noodle ${noodleNumber}. Processing...`);
        } else {
            showNotification(`Failed: ${data.message}`, "error");
        }
    } catch (error) {
        showNotification("Failed to send command", "error");
    }
}

function showNotification(message, type = "info") {
    // Your existing notification code
    console.log(`[${type.toUpperCase()}] ${message}`);
}

// Connection testing
async function testConnection() {
    try {
        const response = await fetch("http://127.0.0.1:8000/");
        const data = await response.json();
        console.log("Backend status:", data);
        return data.status === "running";
    } catch (error) {
        console.error("Backend not reachable");
        return false;
    }
}