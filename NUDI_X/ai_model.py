# ai_model.py (UPDATED - lighter model)
import torch
from transformers import AutoTokenizer, AutoModelForCausalLM
import logging

logger = logging.getLogger(__name__)

# Use a smaller model for better performance
# Smaller and faster than phi-2
# Alternative: "distilgpt2" or "gpt2" if DialoGPT fails
MODEL_NAME = "microsoft/DialoGPT-small"  


# Cache for responses to improve performance
response_cache = {}

try:
    tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME)
    model = AutoModelForCausalLM.from_pretrained(
        MODEL_NAME,
        torch_dtype=torch.float32,
        device_map="auto",
        low_cpu_mem_usage=True
    )
    model.eval()
    AI_LOADED = True
    logger.info(f"✅ AI model '{MODEL_NAME}' loaded successfully")
    
    # Set padding token if not set
    if tokenizer.pad_token is None:
        tokenizer.pad_token = tokenizer.eos_token
        
except Exception as e:
    logger.warning(f"⚠️ Failed to load AI model: {e}")
    logger.info("Using mock AI responses")
    AI_LOADED = False
    tokenizer = None
    model = None

# Pre-defined responses for common requests
PRE_DEFINED_RESPONSES = {
    # Keywords -> Response
    "spicy": "I'll prepare Hot Spicy Ramen for you! This is our spiciest option with special chili oil.",
    "hot": "I'll prepare Hot Spicy Ramen for you! Perfect for those who love heat.",
    "chicken": "I'll prepare Chicken Noodles for you! Made with tender chicken pieces in a savory broth.",
    "poultry": "I'll prepare Chicken Noodles for you! Our chicken is always fresh and delicious.",
    "meat": "I'll prepare Chicken Noodles for you! A meaty and satisfying choice.",
    "cheese": "I'll prepare Cheese Noodles for you! Creamy, cheesy, and absolutely delicious.",
    "creamy": "I'll prepare Cheese Noodles for you! The creamiest noodles you'll ever taste.",
    "dairy": "I'll prepare Cheese Noodles for you! Loaded with real cheese.",
    "vegetarian": "I'll prepare Veg Clear Soup for you! A light and healthy vegetarian option.",
    "veggie": "I'll prepare Veg Clear Soup for you! Packed with fresh vegetables.",
    "light": "I'll prepare Veg Clear Soup for you! A light and refreshing choice.",
    "soup": "I'll prepare Veg Clear Soup for you! Warm and comforting vegetable soup.",
    "ramen": "I'll prepare Hot Spicy Ramen for you! Authentic ramen experience.",
    "noodle": "I'll prepare Chicken Noodles for you! Classic noodles done right.",
    "hungry": "I'll prepare Hot Spicy Ramen for you! This will satisfy your hunger!",
    "recommend": "I'll prepare Hot Spicy Ramen for you! It's our most popular item!",
    "default": "I'll prepare Hot Spicy Ramen for you! This is our signature dish and customer favorite."
}

def get_ai_reply(user_message: str) -> str:
    """Get AI response for user message"""
    
    # Check cache first
    user_lower = user_message.lower().strip()
    if user_lower in response_cache:
        return response_cache[user_lower]
    
    # First, try to match with pre-defined responses
    for keyword, response in PRE_DEFINED_RESPONSES.items():
        if keyword in user_lower:
            response_cache[user_lower] = response
            return response
    
    # If AI model is loaded, use it
    if AI_LOADED and tokenizer and model:
        try:
            prompt = f"""User: {user_message}
Assistant: I'll prepare """
            
            inputs = tokenizer(prompt, return_tensors="pt", truncation=True, max_length=100)
            
            with torch.no_grad():
                outputs = model.generate(
                    **inputs,
                    max_new_tokens=50,
                    temperature=0.7,
                    do_sample=True,
                    top_p=0.9,
                    pad_token_id=tokenizer.pad_token_id,
                    eos_token_id=tokenizer.eos_token_id
                )
            
            response = tokenizer.decode(outputs[0], skip_special_tokens=True)
            
            # Extract only the assistant's response
            if "Assistant:" in response:
                response = response.split("Assistant:")[-1].strip()
            
            # Ensure it starts with "I'll prepare"
            if not response.startswith("I'll prepare"):
                # Find which noodle is mentioned
                noodles = ["Hot Spicy Ramen", "Chicken Noodles", "Cheese Noodles", "Veg Clear Soup"]
                mentioned_noodle = None
                for noodle in noodles:
                    if noodle.lower() in user_lower:
                        mentioned_noodle = noodle
                        break
                
                if mentioned_noodle:
                    response = f"I'll prepare {mentioned_noodle} for you! {response}"
                else:
                    response = PRE_DEFINED_RESPONSES["default"]
            
            # Cache the response
            response_cache[user_lower] = response
            return response
            
        except Exception as e:
            logger.error(f"AI model error: {e}")
            # Fall back to pre-defined response
            return PRE_DEFINED_RESPONSES["default"]
    
    # Fallback if AI model is not available
    return PRE_DEFINED_RESPONSES["default"]