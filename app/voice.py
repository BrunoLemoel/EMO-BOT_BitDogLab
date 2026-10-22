"""
Voice recognition (STT) module.
"""
import speech_recognition as sr
from app.logger import log

recognizer = sr.Recognizer()


def listen_once() -> str:
    """Block and listen for one phrase. Returns recognized text or empty string."""
    try:
        with sr.Microphone() as source:
            log("Adjusting for ambient noise...", "info")
            recognizer.adjust_for_ambient_noise(source, duration=1)
            log("Listening... Speak now!", "info")
            audio = recognizer.listen(source, timeout=8, phrase_time_limit=10)
            try:
                text = recognizer.recognize_google(audio, language="pt-BR")
            except Exception:
                text = recognizer.recognize_google(audio)
            log(f"You said: {text}", "user")
            return text
    except sr.WaitTimeoutError:
        log("No speech detected.", "warning")
    except sr.UnknownValueError:
        log("Could not understand audio.", "warning")
    except Exception as e:
        log(f"Mic error: {e}", "error")
    return ""
