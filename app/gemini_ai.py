"""
Gemini AI integration for the AI Robot Dashboard.
Uses gemini-3.8-flash (the current available model).
"""
import google.generativeai as genai
import time
from app import config
from app.logger import log

VALID_EXPRESSIONS = [
    "normal", "happy", "sad", "angry", "surprised",
    "wink", "sleepy", "love", "confused", "thinking"
]

# Rate limiting
MIN_INTERVAL_SEC   = 5     # Minimum seconds between requests
MAX_PER_HOUR       = 30    # Max requests per hour
_last_request_time = 0.0
_hourly_timestamps = []

SYSTEM_PROMPT = """You are a helpful robot assistant with emotions. 
You must ALWAYS respond in this EXACT format (no exceptions):
EMOTION:expression|TEXT:your response here

Rules:
- expression must be ONE of: normal, happy, sad, angry, surprised, wink, sleepy, love, confused, thinking
- Choose the emotion that best matches the tone of your response
- Keep your text response brief (1-2 sentences)
- Do NOT include any other formatting

Examples:
User: Tell me a joke
EMOTION:happy|TEXT:Why did the robot go on vacation? Because it needed to recharge its batteries!

User: I'm feeling sad today
EMOTION:sad|TEXT:I'm sorry to hear that. I hope things get better for you soon.

User: What is 2+2?
EMOTION:normal|TEXT:2 plus 2 equals 4."""


def init_gemini():
    if not config.GEMINI_API_KEY:
        log("ERROR: Please set GEMINI_API_KEY in .env file!", "error")
        return False
    try:
        genai.configure(api_key=config.GEMINI_API_KEY)
        config.gemini_model = genai.GenerativeModel("gemini-3.8-flash")
        log("Gemini AI connected! (gemini-3.8-flash)", "success")
        return True
    except Exception as e:
        log(f"Gemini error: {e}", "error")
        return False


def ask_gemini(question: str) -> tuple:
    """Returns (response_text, emotion) tuple. Rate-limited."""
    global _last_request_time, _hourly_timestamps

    now = time.time()

    # Cooldown check
    elapsed = now - _last_request_time
    if elapsed < MIN_INTERVAL_SEC:
        wait = round(MIN_INTERVAL_SEC - elapsed, 1)
        return (f"Please wait {wait}s before asking again.", "sleepy")

    # Hourly limit check
    _hourly_timestamps = [t for t in _hourly_timestamps if now - t < 3600]
    if len(_hourly_timestamps) >= MAX_PER_HOUR:
        return ("Hourly request limit reached (30/hr). Try again later.", "sad")

    if config.gemini_model is None:
        if not init_gemini():
            return ("Gemini not connected.", "confused")
    try:
        _last_request_time = now
        _hourly_timestamps.append(now)
        remaining = MAX_PER_HOUR - len(_hourly_timestamps)
        log(f"Gemini credits: {remaining}/{MAX_PER_HOUR} remaining this hour", "info")

        response = config.gemini_model.generate_content(
            SYSTEM_PROMPT + f"\n\nUser: {question}"
        )
        raw = response.text.strip()
        return parse_emotion_response(raw)
    except Exception as e:
        return (f"Gemini error: {e}", "confused")


def parse_emotion_response(raw: str) -> tuple:
    """Parse 'EMOTION:happy|TEXT:Hello' into ('Hello', 'happy')."""
    try:
        if "EMOTION:" in raw and "|TEXT:" in raw:
            emotion_part = raw.split("|TEXT:")[0].replace("EMOTION:", "").strip().lower()
            text_part    = raw.split("|TEXT:")[1].strip()
            if emotion_part in VALID_EXPRESSIONS:
                return (text_part, emotion_part)
            return (text_part, "normal")
    except:
        pass
    # Fallback: couldn't parse, return raw text with normal expression
    return (raw, "normal")
