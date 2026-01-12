#ai_model.py
# pip install fastapi uvicorn
# uvicorn main:app --reload
# Uvicorn running on http://127.0.0.1:8000

import torch
from transformers import AutoTokenizer, AutoModelForCausalLM

MODEL_NAME = "microsoft/phi-2"

tokenizer = None
model = None
device = "cuda" if torch.cuda.is_available() else "cpu"

def load_model():
    global tokenizer, model
    if model is None:
        print("Loading AI model on", device)
        tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME)
        model = AutoModelForCausalLM.from_pretrained(
            MODEL_NAME,
            torch_dtype=torch.float16 if device == "cuda" else torch.float32,
            device_map="auto"
        )
        model.eval()

def get_ai_reply(user_message: str) -> str:
    load_model()

    prompt = f"""
SYSTEM ROLE:
You are an autonomous decision-making AI agent embedded in a smart noodle vending machine.

MISSION:
Analyze the user's request and select exactly ONE noodle that best matches their intent.
- Hot Spicy Ramen
- Chicken Noodles
- Cheese Noodles
- Veg Clear Soup

User message: {user_message}
AI response:
"""

    inputs = tokenizer(prompt, return_tensors="pt").to(model.device)

    with torch.no_grad():
        output = model.generate(
            **inputs,
            max_new_tokens=60,
            temperature=0.7,
            do_sample=True
        )

    response = tokenizer.decode(output[0], skip_special_tokens=True)
    return response.split("AI response:")[-1].strip()
