#main.py
# pip install fastapi uvicorn
# uvicorn main:app --reload
# Uvicorn running on http://127.0.0.1:8000
#pip install -r requirements.txt

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from ai_model import get_ai_reply

app = FastAPI(title="Noodle AI Agent")

# Allow requests from your frontend
origins = [
    "http://localhost",
    "http://localhost:5500",  
    "http://127.0.0.1:5500",
]

app.add_middleware(
    CORSMiddleware,
    allow_origins=origins,
    allow_credentials=True,
    allow_methods=["*"],  # allow POST, OPTIONS, etc.
    allow_headers=["*"],
)

class ChatRequest(BaseModel):
    user_message: str

@app.post("/chat")
def chat_agent(req: ChatRequest):
    reply = get_ai_reply(req.user_message)
    return {"reply": reply}
