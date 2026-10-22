"""
Communication layer: WiFi, USB Serial, Bluetooth, Audio streaming.
All operations are non-blocking where possible.
"""
import time
import socket
import subprocess
import re
import threading
import requests
import serial
import serial.tools.list_ports
import pyttsx3
import numpy as np
import wave
import tempfile
import os

from app import config
from app.logger import log

# =============================================
#  TTS ENGINE
# =============================================
tts_engine = pyttsx3.init()
tts_engine.setProperty('rate', 150)
tts_engine.setProperty('volume', 1.0)
_tts_lock = threading.Lock()


def speak_pc(text: str):
    """Speak text on the computer speakers via local TTS."""
    def _worker():
        try:
            with _tts_lock:
                engine = pyttsx3.init()
                engine.setProperty('rate', 155)
                engine.setProperty('volume', 1.0)
                engine.say(text)
                engine.runAndWait()
        except Exception as e:
            log(f"PC TTS error: {e}", "warning")
    threading.Thread(target=_worker, daemon=True).start()


# =============================================
#  DISPATCH
# =============================================
def send_command(cmd: str, conn_mode: str) -> str:
    """Send command to robot via the given connection mode."""
    try:
        # Se WiFi foi selecionado mas não há IP, e a USB Serial está conectada, roteia para USB
        if conn_mode == "WiFi" and not config.esp32_ip and config.serial_conn and config.serial_conn.is_open:
            return send_serial(cmd)

        if conn_mode == "WiFi":
            return send_wifi(cmd)
        elif conn_mode == "USB Serial":
            return send_serial(cmd)
        elif conn_mode == "Bluetooth":
            return send_bluetooth(cmd)
    except Exception as e:
        return f"ERROR: {e}"
    return "ERROR: Unknown mode"


# =============================================
#  WIFI
# =============================================
def send_wifi(cmd: str) -> str:
    ip = config.esp32_ip
    if not ip:
        return "ERROR: No IP entered"

    try:
        if cmd.startswith("MSG:"):
            r = requests.get(f"http://{ip}/message",
                             params={"text": cmd[4:]}, timeout=5)
            return r.text
        elif cmd.startswith("SERVO:"):
            r = requests.get(f"http://{ip}/servo",
                             params={"angle": cmd[6:]}, timeout=5)
            return r.text
        elif cmd.startswith("EXPR:"):
            r = requests.get(f"http://{ip}/expr",
                             params={"expr": cmd[5:]}, timeout=5)
            return r.text
        elif cmd.startswith("ALL:"):
            parts = cmd[4:].split(":", 1)
            expr = parts[0]
            text = parts[1] if len(parts) > 1 else ""
            r = requests.get(f"http://{ip}/all",
                             params={"expr": expr, "text": text}, timeout=5)
            return r.text
        elif cmd == "BLINK":
            r = requests.get(f"http://{ip}/expr",
                             params={"expr": "blink"}, timeout=5)
            return r.text
        elif cmd.startswith("BOTH:"):
            parts = cmd[5:].split(":", 1)
            angle = parts[0]
            text  = parts[1] if len(parts) > 1 else ""
            r = requests.get(f"http://{ip}/both",
                             params={"angle": angle, "text": text}, timeout=5)
            return r.text
        elif cmd == "PING":
            r = requests.get(f"http://{ip}/ping", timeout=5)
            return r.text
        elif cmd == "STATUS":
            r = requests.get(f"http://{ip}/status", timeout=5)
            return r.text
    except requests.exceptions.Timeout:
        return "ERROR: WiFi timeout"
    except requests.exceptions.ConnectionError:
        return "ERROR: Cannot reach ESP32"
    except Exception as e:
        return f"ERROR: {e}"

    return "Unknown command"


def send_audio_wifi(text: str) -> str:
    ip = config.esp32_ip
    if not ip:
        return "ERROR: No IP"

    log("Generating TTS Audio...", "info")

    temp_wav = os.path.join(tempfile.gettempdir(), "temp_tts.wav")
    with _tts_lock:
        tts_engine.save_to_file(text, temp_wav)
        tts_engine.runAndWait()

    try:
        with wave.open(temp_wav, 'rb') as wf:
            nchannels = wf.getnchannels()
            sampwidth = wf.getsampwidth()
            frames = wf.readframes(wf.getnframes())

            dtype = np.int16 if sampwidth == 2 else np.uint8
            audio_data = np.frombuffer(frames, dtype=dtype)

            if nchannels == 2:
                audio_data = audio_data[::2] // 2 + audio_data[1::2] // 2

            raw_bytes = audio_data.astype(np.int16).tobytes()

            log(f"Streaming {len(raw_bytes)} bytes to ESP32...", "info")
            r = requests.post(f"http://{ip}/audio",
                              data=raw_bytes,
                              headers={'Content-Type': 'application/octet-stream'},
                              timeout=15)
            return r.text
    except Exception as e:
        return f"Audio Error: {e}"
    finally:
        if os.path.exists(temp_wav):
            os.remove(temp_wav)


# =============================================
#  USB SERIAL
# =============================================
def send_serial(cmd: str) -> str:
    if config.serial_conn is None or not config.serial_conn.is_open:
        return "ERROR: Serial not connected. Click 'Connect USB' first."

    try:
        config.serial_conn.reset_input_buffer()
        config.serial_conn.write((cmd + "\n").encode())
        time.sleep(0.15)
        response = ""
        while config.serial_conn.in_waiting:
            line = config.serial_conn.readline().decode(errors="ignore")
            response += line
            if "\n" in line:
                break
        return response.strip() or "OK"
    except Exception as e:
        disconnect_serial()
        return f"ERROR: {e}"


def connect_serial(port: str, baud: int):
    """Connect to a serial port. Returns (success, message)."""
    disconnect_serial()  # Close any existing connection first
    try:
        config.serial_conn = serial.Serial(port, baud, timeout=2)
        config.serial_conn.dtr = True
        config.serial_conn.rts = True
        time.sleep(1)
        return True, f"USB Serial connected: {port} @ {baud}"
    except Exception as e:
        config.serial_conn = None
        return False, f"Serial error: {e}"


def disconnect_serial():
    if config.serial_conn:
        try:
            config.serial_conn.close()
        except:
            pass
        config.serial_conn = None


def list_serial_ports():
    return [p.device for p in serial.tools.list_ports.comports()]


# =============================================
#  BLUETOOTH
# =============================================
def send_bluetooth(cmd: str) -> str:
    if not config.cached_bt_mac:
        auto_detect_bt_mac()

    if not config.cached_bt_mac:
        return "ERROR: AI_ROBOT_ESP32 not found. Pair it in Windows first."

    if config.bt_conn is None:
        config.bt_conn = socket.socket(socket.AF_BLUETOOTH,
                                       socket.SOCK_STREAM,
                                       socket.BTPROTO_RFCOMM)
        try:
            config.bt_conn.connect((config.cached_bt_mac, 1))
        except Exception as e:
            config.bt_conn = None
            return f"ERROR: BT Connect failed ({e})"

    try:
        config.bt_conn.send((cmd + "\n").encode())
        time.sleep(0.3)
        response = config.bt_conn.recv(1024).decode(errors="ignore")
        return response.strip()
    except:
        config.bt_conn = None
        return "Sent"


def auto_detect_bt_mac() -> str:
    try:
        cmd = 'powershell -NoProfile -Command "Get-PnpDevice -Class Bluetooth | Where-Object { $_.FriendlyName -like \'*AI_ROBOT*\' -or $_.FriendlyName -like \'*ESP32*\' } | Select-Object -ExpandProperty InstanceId"'
        result = subprocess.run(cmd, capture_output=True, text=True, shell=True,
                                creationflags=0x08000000)
        match = re.search(r'DEV_([0-9A-F]{12})', result.stdout, re.IGNORECASE)
        if match:
            mac_hex = match.group(1)
            config.cached_bt_mac = ':'.join(mac_hex[i:i+2] for i in range(0, 12, 2))
            return config.cached_bt_mac
    except:
        pass
    return ""
