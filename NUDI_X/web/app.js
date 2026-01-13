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

console.log("app.js loaded"); // DEBUG LINE

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

  // Add user message instantly (no typing effect for user)
  addMessage("user", message);
  input.value = "";

  // Show typing indicator while waiting for response
  showTypingIndicator();

  try {
    const res = await fetch("http://127.0.0.1:8000/chat", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ user_message: message })
    });

    if (!res.ok) throw new Error("Server error");

    const data = await res.json();
    
    // Remove typing indicator
    removeTypingIndicator();
    
    // Clean and normalize the response text
    let aiResponse = data.reply || "";
    
    // Ensure proper spacing and formatting
    aiResponse = aiResponse.trim();
    
    // Add AI response with smooth typing animation (30ms per character)
    await addMessageWithTyping("ai", aiResponse, 30);

  } catch (err) {
    console.error(err);
    
    // Remove typing indicator even on error
    removeTypingIndicator();
    
    // Error messages appear instantly
    addMessage("ai", "Server not responding...");
  }
}

/* 🔥 Attach button click AFTER page loads */
document.getElementById("sendBtn").addEventListener("click", sendMessage);

/* 🔥 Press Enter key support */
document.getElementById("userInput").addEventListener("keydown", function (e) {
  if (e.key === "Enter") sendMessage();
});