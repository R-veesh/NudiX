# 🍜 Smart Noodle Vending Machine – Agentic AI Backend

## 📌 Project Overview

This project implements an Agentic AI backend for a Smart IoT-based Noodle Vending Machine.
The AI agent interacts with users through a mobile or web interface, understands their mood, situation, or intent, and intelligently recommends one of four available noodle flavors.

The system is designed as part of the MyUrbanECD IoT Project, where:

The IoT hardware vending machine is developed separately

This repository focuses on the AI-powered backend and API layer

## 🎯 Key Features

🤖 Agentic AI using Hugging Face LLM (Phi-2 / TinyLlama)

💬 Natural language chat-based interaction

🧠 Context-aware noodle recommendation

⚡ GPU-accelerated inference (RTX 3050 – 6GB VRAM)

🌐 REST API using FastAPI

🧪 Built-in Swagger UI for testing

🔄 Lazy AI model loading (stable & efficient)

🍜 Available Noodle Options

## The AI agent recommends only one of the following:

Hot Spicy Ramen – Cold weather / strong hunger

Chicken Noodles – Starving or high energy need

Cheese Noodles – Exhausted / comfort food

Veg Clear Soup – Light meal / low appetite

## 🧠 System Architecture (High Level)
User (Mobile / Web App)
        ↓
   FastAPI Backend
        ↓
  Agentic AI (LLM)
        ↓
Noodle Recommendation
        ↓
 IoT Vending Machine (Future Integration)

## 🛠️ Technology Stack
Backend

Python 3.10

FastAPI

Uvicorn

AI & ML

PyTorch (CUDA-enabled)

Hugging Face Transformers

Accelerate

SentencePiece

Hardware

NVIDIA GPU (Tested on RTX 3050 – 6GB VRAM)

## 📦 Project Structure
``IOT_Project/
│
├── main.py            # FastAPI application
├── ai_model.py        # AI model loading & inference
├── requirements.txt   # Python dependencies
├── README.md          # Project documentation ``

## ⚙️ Installation & Setup
### 1️⃣ Clone the Repository
``git clone <your-repo-url>
cd IOT_Project``

### 2️⃣ Create Virtual Environment (Recommended)
python -m venv venv
venv\Scripts\activate

### 3️⃣ Install CUDA-Enabled PyTorch (IMPORTANT)

⚠️ PyTorch with CUDA must be installed separately.

``pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu118``

### 4️⃣ Install Remaining Dependencies
``pip install -r requirements.txt``

### 5️⃣ Verify GPU Availability
`` python
import torch
print(torch.cuda.is_available())
print(torch.cuda.get_device_name(0)) ``


Expected output:

True
NVIDIA GeForce RTX 3050(your)

## 🚀 Running the Application
Start FastAPI Server
uvicorn main:app


Server will run at:

``http://127.0.0.1:8000``

## 🧪 API Testing (Swagger UI)

### Open your browser:

``http://127.0.0.1:8000/docs``

### Test Endpoint: /chat

Request Body
``
{
  "user_message": "I'm exhausted and it's cold today"
}
``

Sample Response
``
{
  "reply": "You seem tired and cold. A hot spicy ramen would be perfect for you 🍜"
}
``
##🤖 AI Design Approach

Uses a lightweight LLM suitable for limited VRAM

Implements lazy model loading to prevent startup crashes

Combines agent logic + natural language understanding

Designed for future IoT availability checks

## 🔮 Future Enhancements

🔗 Real-time IoT inventory & availability check

🧠 Conversation memory

📱 Flutter mobile chat interface

🌍 Multi-language support

🐳 Docker + GPU deployment

📄 Academic Note

## This project demonstrates:

Practical use of Agentic AI

GPU-accelerated local LLM deployment

Clean API design with FastAPI

Industry-standard dependency management

## 👨‍💻 Author

Raveesha
Software Engineering Student
MyUrbanECD IoT Project
