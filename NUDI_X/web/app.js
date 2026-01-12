// app.js

let pendingOrder = null;

// Generate a random session ID for each user
const sessionId = Math.random().toString(36).substring(2, 12);

function addMessage(sender, text) {
  const chatBox = document.getElementById("chatBox");
  const div = document.createElement("div");
  div.className = "msg " + sender;
  div.innerText = text;
  chatBox.appendChild(div);
  chatBox.scrollTop = chatBox.scrollHeight;
}

async function sendMessage() {
  const input = document.getElementById("userInput");
  const message = input.value.trim();
  if (!message) return;

  addMessage("user", "You: " + message);
  input.value = "";

  try {
    const response = await fetch("http://127.0.0.1:8000/chat", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        user_message: message,  // <-- updated to match FastAPI
        session_id: sessionId,
        pending_order: pendingOrder
      })
    });

    const data = await response.json();
    addMessage("ai", "AI: " + data.reply);

    // Handle pending order if backend sends it
    if (data.pending_order) {
      pendingOrder = data.pending_order;
    }

    // Reset pending order if confirmed
    if (data.confirmed) {
      pendingOrder = null;
    }
  } catch (err) {
    console.error(err);
    addMessage("ai", "AI: Error connecting to server.");
  }
}
