# pip install fastapi uvicorn
# uvicorn main:app --reload
# Uvicorn running on http://127.0.0.1:8000
#pip install -r requirements.txt

from fastapi import FastAPI
from pydantic import BaseModel
from ai_model import get_ai_reply

app = FastAPI(title="Noodle AI Agent")

class ChatRequest(BaseModel):
    user_message: str

@app.post("/chat")
def chat_agent(req: ChatRequest):
    reply = get_ai_reply(req.user_message)
    return {"reply": reply}
