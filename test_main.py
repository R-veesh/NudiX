# pip install fastapi uvicorn
# uvicorn main:app --reload
# Uvicorn running on http://127.0.0.1:8000



from fastapi import FastAPI
from pydantic import BaseModel

app = FastAPI(
    title="Noodle Vending Machine AI Agent",
    description="Agentic AI backend for suggesting noodle flavors",
    version="1.0"
)

class ChatRequest(BaseModel):
    user_message: str

class ChatResponse(BaseModel):
    reply: str

@app.post("/chat", response_model=ChatResponse)
def chat_agent(request: ChatRequest):
    msg = request.user_message.lower()

    if "cold" in msg:
        return {"reply": "It's cold today ❄️ I recommend Hot Spicy Ramen 🍜"}
    elif "exhausted" in msg:
        return {"reply": "You seem exhausted 😴 Cheese Noodles will help 🧀"}
    elif "starving" in msg:
        return {"reply": "You're starving! Chicken Noodles are available 🍗"}
    else:
        return {"reply": "I recommend our Veg Clear Soup 🌿 Light and healthy!"}
