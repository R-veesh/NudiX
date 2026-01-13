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

async function sendMessage() {
  const input = document.getElementById("userInput");
  const message = input.value.trim();
  if (!message) return;

  addMessage("user", "You: " + message);
  input.value = "";

  try {
    const res = await fetch("http://127.0.0.1:8000/chat", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ user_message: message })
    });

    if (!res.ok) throw new Error("Server error");

    const data = await res.json();
    addMessage("ai", "AI: " + data.reply);

  } catch (err) {
    console.error(err);
    addMessage("ai", "AI: Server not responding...");
  }
}

/* 🔥 Attach button click AFTER page loads */
document.getElementById("sendBtn").addEventListener("click", sendMessage);

/* 🔥 Press Enter key support */
document.getElementById("userInput").addEventListener("keydown", function (e) {
  if (e.key === "Enter") sendMessage();
});
