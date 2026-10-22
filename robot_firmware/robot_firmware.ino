/*
================================================
  AI ROBOT - COMPLETE INTEGRATED CODE
  
  Components:
  - OLED Display (U8g2) → SCL=22, SDA=21
  - INMP441 Mic (I2S)   → WS=15, SCK=14, SD=32
  - MAX98357A Amp (I2S)  → LRC=25, BCLK=26, DIN=27
  - Servo Motor         → GPIO 13
  - WiFi HTTP Server
  - USB Serial
  - Preferences (NVS)   → For Persistent Memory
================================================
*/

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>
#include <driver/i2s.h>
#include <Preferences.h>

// =============================================
//  WiFi CREDENTIALS — CHANGE THESE
// =============================================
const char* ssid     = "AT-VISITANTE";
const char* password = "ATL%940308!";

// =============================================
//  DISPLAY (U8g2)
// =============================================
U8G2_SSD1306_128X64_NONAME_F_HW_I2C
  u8g2(U8G2_R0, U8X8_PIN_NONE, 22, 21);

// =============================================
//  SERVO
// =============================================
#define SERVO_PIN   13
Servo myServo;
int   currentAngle = 90;

// =============================================
//  INMP441 MIC (RX) & MAX98357A AMP (TX)
// =============================================
// Settings for Mic
#define I2S_WS_RX   15
#define I2S_SCK_RX  14
#define I2S_SD_RX   32
#define I2S_PORT_RX I2S_NUM_0

// Settings for Speaker
#define I2S_WS_TX   25 // LRC
#define I2S_BCLK_TX 26 // BCLK
#define I2S_DOUT_TX 27 // DIN
#define I2S_PORT_TX I2S_NUM_1

#define SAMPLES  256

// =============================================
//  SYSTEM OBJECTS
// =============================================
WebServer       server(80);
Preferences     preferences;

// =============================================
//  GLOBALS
// =============================================
String  currentExpression = "normal";
String  lastMessage       = "";
String  connectionMode    = "USB";
float   micVolume         = 0;
bool    wifiConnected     = false;

unsigned long lastBlink       = 0;
unsigned long lastAutoExpr    = 0;
unsigned long lastMicCheck    = 0;

// Eye positions
#define LEFT_EYE_X   32
#define RIGHT_EYE_X  96
#define EYE_Y        32
#define EYE_R        18
#define PUPIL_R       7

// Volume thresholds
#define TALK_THRESHOLD    0.8
#define LISTEN_THRESHOLD  0.2

// =============================================
//  PERSISTENT STORAGE HELPERS
// =============================================
void loadPersistentState() {
  preferences.begin("robot_db", true); // true = Read-only mode
  currentAngle = preferences.getInt("angle", 90);
  currentExpression = preferences.getString("expr", "normal");
  preferences.end();
  Serial.println("Loaded state from DB -> Angle: " + String(currentAngle) + ", Expr: " + currentExpression);
}

void saveStateToDB(int angle, String expr) {
  preferences.begin("robot_db", false); // false = Read/Write mode
  preferences.putInt("angle", angle);
  preferences.putString("expr", expr);
  preferences.end();
}

// =============================================
//  I2S SETUP (MIC & SPEAKER)
// =============================================
void setupI2S() {
  // --- INMP441 (Mic - RX) ---
  i2s_config_t i2s_rx_config = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate          = 16000,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 8,
    .dma_buf_len          = 64,
    .use_apll             = false,
    .tx_desc_auto_clear   = false,
    .fixed_mclk           = 0
  };
  i2s_pin_config_t pin_rx_config = {
    .bck_io_num   = I2S_SCK_RX,
    .ws_io_num    = I2S_WS_RX,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = I2S_SD_RX
  };
  i2s_driver_install(I2S_PORT_RX, &i2s_rx_config, 0, NULL);
  i2s_set_pin(I2S_PORT_RX, &pin_rx_config);
  i2s_start(I2S_PORT_RX);

  // --- MAX98357A (Speaker - TX) ---
  i2s_config_t i2s_tx_config = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate          = 16000, // Matching python TTS 16kHz
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 8,
    .dma_buf_len          = 64,
    .use_apll             = false,
    .tx_desc_auto_clear   = true,
    .fixed_mclk           = 0
  };
  i2s_pin_config_t pin_tx_config = {
    .bck_io_num   = I2S_BCLK_TX,
    .ws_io_num    = I2S_WS_TX,
    .data_out_num = I2S_DOUT_TX,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };
  i2s_driver_install(I2S_PORT_TX, &i2s_tx_config, 0, NULL);
  i2s_set_pin(I2S_PORT_TX, &pin_tx_config);
  i2s_start(I2S_PORT_TX);
}

float readMicVolume() {
  int32_t samples[SAMPLES];
  size_t  bytesRead = 0;
  i2s_read(I2S_PORT_RX, samples, sizeof(samples),
           &bytesRead, portMAX_DELAY);
  int   samplesRead = bytesRead / sizeof(int32_t);
  float sum = 0;
  for (int i = 0; i < samplesRead; i++) {
    float val = samples[i] / (float)INT32_MAX;
    sum += val * val;
  }
  return sqrt(sum / samplesRead) * 1000.0;
}

// =============================================
//  SERVO
// =============================================
void rotateToFace(int angle) {
  angle = constrain(angle, 0, 180);
  int step = (angle > currentAngle) ? 1 : -1;
  while (currentAngle != angle) {
    currentAngle += step;
    myServo.write(currentAngle);
    delay(8);
  }
  Serial.println("Servo: " + String(currentAngle) + "deg");
  saveStateToDB(currentAngle, currentExpression); // Save to DB
}

// =============================================
//  EYE DRAWING HELPERS
// =============================================
void drawEye(int cx, int cy, int outerR, int pupilR,
             int offX, int offY) {
  u8g2.drawCircle(cx, cy, outerR);
  u8g2.drawDisc(cx + offX, cy + offY, pupilR);
  u8g2.setDrawColor(0);
  u8g2.drawDisc(cx + offX + 2, cy + offY - 2, 2);
  u8g2.setDrawColor(1);
}

// =============================================
//  ALL EYE EXPRESSIONS
// =============================================
void eyeNormal() {
  u8g2.clearBuffer();
  drawEye(LEFT_EYE_X,  EYE_Y, EYE_R, PUPIL_R, 0, 0);
  drawEye(RIGHT_EYE_X, EYE_Y, EYE_R, PUPIL_R, 0, 0);
  u8g2.sendBuffer();
}

void eyeHappy() {
  u8g2.clearBuffer();
  for (int i = 0; i < 2; i++) {
    int cx = (i == 0) ? LEFT_EYE_X : RIGHT_EYE_X;
    for (int a = 0; a <= 180; a += 3) {
      float r = a * PI / 180.0;
      u8g2.drawPixel(cx + (int)(EYE_R * cos(r)),
                     EYE_Y - (int)(EYE_R * sin(r)));
    }
    u8g2.setDrawColor(0);
    u8g2.drawBox(cx - EYE_R - 1, EYE_Y, EYE_R*2+2, EYE_R+2);
    u8g2.setDrawColor(1);
    u8g2.drawDisc(cx + EYE_R - 4, EYE_Y + 8, 3);
  }
  u8g2.sendBuffer();
}

void eyeSad() {
  u8g2.clearBuffer();
  for (int i = 0; i < 2; i++) {
    int cx = (i == 0) ? LEFT_EYE_X : RIGHT_EYE_X;
    u8g2.drawCircle(cx, EYE_Y + 4, EYE_R);
    u8g2.setDrawColor(0);
    u8g2.drawBox(cx - EYE_R - 1, EYE_Y - EYE_R - 1,
                 EYE_R*2+2, EYE_R - 2);
    u8g2.setDrawColor(1);
    if (i == 0)
      u8g2.drawLine(cx-EYE_R, EYE_Y-EYE_R+2,
                    cx+EYE_R/2, EYE_Y-EYE_R-4);
    else
      u8g2.drawLine(cx-EYE_R/2, EYE_Y-EYE_R-4,
                    cx+EYE_R, EYE_Y-EYE_R+2);
    u8g2.drawDisc(cx + EYE_R - 5, EYE_Y + EYE_R + 4, 2);
  }
  u8g2.sendBuffer();
}

void eyeAngry() {
  u8g2.clearBuffer();
  drawEye(LEFT_EYE_X,  EYE_Y, EYE_R, PUPIL_R,  2, 2);
  drawEye(RIGHT_EYE_X, EYE_Y, EYE_R, PUPIL_R, -2, 2);
  u8g2.drawLine(LEFT_EYE_X-EYE_R,  EYE_Y-EYE_R-5,
                LEFT_EYE_X+EYE_R,  EYE_Y-EYE_R-1);
  u8g2.drawLine(LEFT_EYE_X-EYE_R,  EYE_Y-EYE_R-4,
                LEFT_EYE_X+EYE_R,  EYE_Y-EYE_R);
  u8g2.drawLine(RIGHT_EYE_X-EYE_R, EYE_Y-EYE_R-1,
                RIGHT_EYE_X+EYE_R, EYE_Y-EYE_R-5);
  u8g2.drawLine(RIGHT_EYE_X-EYE_R, EYE_Y-EYE_R,
                RIGHT_EYE_X+EYE_R, EYE_Y-EYE_R-4);
  u8g2.sendBuffer();
}

void eyeSurprised() {
  u8g2.clearBuffer();
  u8g2.drawCircle(LEFT_EYE_X,  EYE_Y, EYE_R+4);
  u8g2.drawCircle(RIGHT_EYE_X, EYE_Y, EYE_R+4);
  u8g2.drawDisc(LEFT_EYE_X,   EYE_Y, PUPIL_R+2);
  u8g2.drawDisc(RIGHT_EYE_X,  EYE_Y, PUPIL_R+2);
  u8g2.drawLine(LEFT_EYE_X-EYE_R,  EYE_Y-EYE_R-9,
                LEFT_EYE_X+EYE_R,  EYE_Y-EYE_R-9);
  u8g2.drawLine(RIGHT_EYE_X-EYE_R, EYE_Y-EYE_R-9,
                RIGHT_EYE_X+EYE_R, EYE_Y-EYE_R-9);
  u8g2.sendBuffer();
}

void eyeWink() {
  u8g2.clearBuffer();
  drawEye(LEFT_EYE_X, EYE_Y, EYE_R, PUPIL_R, 0, 0);
  u8g2.drawHLine(RIGHT_EYE_X-EYE_R, EYE_Y-1, EYE_R*2);
  u8g2.drawHLine(RIGHT_EYE_X-EYE_R, EYE_Y,   EYE_R*2);
  u8g2.drawHLine(RIGHT_EYE_X-EYE_R, EYE_Y+1, EYE_R*2);
  u8g2.sendBuffer();
}

void eyeSleepy() {
  u8g2.clearBuffer();
  for (int i = 0; i < 2; i++) {
    int cx = (i == 0) ? LEFT_EYE_X : RIGHT_EYE_X;
    u8g2.drawCircle(cx, EYE_Y, EYE_R);
    u8g2.setDrawColor(0);
    u8g2.drawBox(cx-EYE_R-1, EYE_Y-EYE_R-1,
                 EYE_R*2+2, (int)(EYE_R*1.4));
    u8g2.setDrawColor(1);
    u8g2.drawHLine(cx-EYE_R, EYE_Y-3, EYE_R*2);
    u8g2.drawDisc(cx, EYE_Y+5, 4);
  }
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(50, 62, "z z z");
  u8g2.sendBuffer();
}

void eyeLove() {
  u8g2.clearBuffer();
  for (int i = 0; i < 2; i++) {
    int cx = (i == 0) ? LEFT_EYE_X : RIGHT_EYE_X;
    u8g2.drawDisc(cx-6, EYE_Y-4, 7);
    u8g2.drawDisc(cx+6, EYE_Y-4, 7);
    u8g2.drawTriangle(cx-13, EYE_Y-2,
                      cx+13, EYE_Y-2,
                      cx,    EYE_Y+10);
  }
  u8g2.sendBuffer();
}

void eyeConfused() {
  u8g2.clearBuffer();
  u8g2.drawCircle(LEFT_EYE_X,  EYE_Y, EYE_R-4);
  u8g2.drawDisc(LEFT_EYE_X,   EYE_Y, PUPIL_R-2);
  u8g2.drawCircle(RIGHT_EYE_X, EYE_Y, EYE_R+4);
  u8g2.drawDisc(RIGHT_EYE_X,  EYE_Y, PUPIL_R+2);
  u8g2.setFont(u8g2_font_10x20_tf);
  u8g2.drawStr(57, 62, "?");
  u8g2.sendBuffer();
}

void eyeThinking() {
  u8g2.clearBuffer();
  drawEye(LEFT_EYE_X,  EYE_Y, EYE_R, PUPIL_R, 5, -5);
  drawEye(RIGHT_EYE_X, EYE_Y, EYE_R, PUPIL_R, 5, -5);
  u8g2.drawLine(RIGHT_EYE_X-EYE_R, EYE_Y-EYE_R-8,
                RIGHT_EYE_X+EYE_R, EYE_Y-EYE_R-4);
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(50, 62, ". . .");
  u8g2.sendBuffer();
}

void smoothBlink() {
  for (int h = 0; h <= EYE_R*2; h += 3) {
    u8g2.clearBuffer();
    for (int i = 0; i < 2; i++) {
      int cx = (i==0) ? LEFT_EYE_X : RIGHT_EYE_X;
      u8g2.drawCircle(cx, EYE_Y, EYE_R);
      u8g2.setDrawColor(0);
      u8g2.drawBox(cx-EYE_R, EYE_Y-EYE_R, EYE_R*2+1, h);
      u8g2.setDrawColor(1);
    }
    u8g2.sendBuffer();
    delay(12);
  }
  delay(60);
  for (int h = EYE_R*2; h >= 0; h -= 3) {
    u8g2.clearBuffer();
    for (int i = 0; i < 2; i++) {
      int cx = (i==0) ? LEFT_EYE_X : RIGHT_EYE_X;
      u8g2.drawCircle(cx, EYE_Y, EYE_R);
      u8g2.setDrawColor(0);
      u8g2.drawBox(cx-EYE_R, EYE_Y-EYE_R, EYE_R*2+1, h);
      u8g2.setDrawColor(1);
    }
    u8g2.sendBuffer();
    delay(12);
  }
}

// =============================================
//  SET EXPRESSION BY NAME
// =============================================
void setExpression(String expr) {
  expr.toLowerCase();
  expr.trim();
  currentExpression = expr;

  if      (expr == "normal")    eyeNormal();
  else if (expr == "happy")     eyeHappy();
  else if (expr == "sad")       eyeSad();
  else if (expr == "angry")     eyeAngry();
  else if (expr == "surprised") eyeSurprised();
  else if (expr == "wink")      eyeWink();
  else if (expr == "sleepy")    eyeSleepy();
  else if (expr == "love")      eyeLove();
  else if (expr == "confused")  eyeConfused();
  else if (expr == "thinking")  eyeThinking();
  else if (expr == "blink")     smoothBlink();
  else                          eyeNormal();

  Serial.println("Expression: " + expr);
  saveStateToDB(currentAngle, currentExpression); // Save to DB
}

// =============================================
//  SCROLL TEXT ON OLED
// =============================================
void scrollText(String text) {
  int textW  = text.length() * 6;
  int startX = 128;
  int endX   = -textW;

  u8g2.setFont(u8g2_font_6x10_tf);

  for (int x = startX; x > endX; x -= 3) {
    u8g2.clearBuffer();
    // Header
    u8g2.drawStr(0, 10, "[ AI ROBOT ]");
    u8g2.drawHLine(0, 12, 128);
    // Scrolling text
    u8g2.drawStr(x, 40, text.c_str());
    // Connection mode at bottom
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawStr(0, 63, ("Via: " + connectionMode).c_str());
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.sendBuffer();
    delay(12);
  }

  // After scrolling restore expression
  setExpression(currentExpression);
}

// =============================================
//  PROCESS COMMANDS FROM DASHBOARD
// =============================================
void processCommand(String cmd, String source) {
  cmd.trim();
  connectionMode = source;
  Serial.println("[" + source + "] " + cmd);

  String reply = "OK";

  if (cmd == "PING") {
    reply = "PONG:AI_ROBOT_ONLINE";
    setExpression("surprised");
    delay(500);
    setExpression("normal");

  } else if (cmd == "STATUS") {
    // Modified to use a buffer to avoid heavy String concatenation
    char statusBuf[128];
    snprintf(statusBuf, sizeof(statusBuf), 
             "STATUS:angle=%d,expr=%s,mode=%s", 
             currentAngle, currentExpression.c_str(), connectionMode.c_str());
    reply = String(statusBuf);

  } else if (cmd.startsWith("MSG:")) {
    // MSG:Hello World
    lastMessage = cmd.substring(4);
    scrollText(lastMessage);
    reply = "OK:MSG";

  } else if (cmd.startsWith("EXPR:")) {
    // EXPR:happy
    String expr = cmd.substring(5);
    setExpression(expr);
    reply = "OK:EXPR:" + expr;

  } else if (cmd.startsWith("SERVO:")) {
    // SERVO:90
    int angle = cmd.substring(6).toInt();
    rotateToFace(angle);
    reply = "OK:SERVO:" + String(angle);

  } else if (cmd.startsWith("BOTH:")) {
    // BOTH:90:Hello World
    int colonPos = cmd.indexOf(':', 5);
    if (colonPos > 0) {
      int    angle = cmd.substring(5, colonPos).toInt();
      String text  = cmd.substring(colonPos + 1);
      lastMessage  = text;
      rotateToFace(angle);
      delay(200);
      scrollText(text);
      reply = "OK:BOTH";
    }

  } else if (cmd.startsWith("ALL:")) {
    // ALL:90:happy:Hello World
    String body   = cmd.substring(4);
    int    c1     = body.indexOf(':');
    int    c2     = body.indexOf(':', c1+1);
    if (c1 > 0 && c2 > 0) {
      int    angle = body.substring(0, c1).toInt();
      String expr  = body.substring(c1+1, c2);
      String text  = body.substring(c2+1);
      lastMessage  = text;
      rotateToFace(angle);
      delay(150);
      setExpression(expr);
      delay(500);
      scrollText(text);
      reply = "OK:ALL";
    }

  } else if (cmd.startsWith("BLINK")) {
    smoothBlink();
    reply = "OK:BLINK";

  } else {
    reply = "ERR:Unknown command";
  }

  // Send reply back to source
  if (source == "USB") Serial.println(reply);
}

// =============================================
//  WIFI HANDLERS
// =============================================
void handleMsg() {
  if (server.hasArg("text")) {
    processCommand("MSG:" + server.arg("text"), "WiFi");
    server.send(200, "text/plain", "OK");
  } else server.send(400, "text/plain", "Missing text");
}

void handleExpr() {
  if (server.hasArg("expr")) {
    processCommand("EXPR:" + server.arg("expr"), "WiFi");
    server.send(200, "text/plain", "OK");
  } else server.send(400, "text/plain", "Missing expr");
}

void handleServo() {
  if (server.hasArg("angle")) {
    processCommand("SERVO:" + server.arg("angle"), "WiFi");
    server.send(200, "text/plain", "OK");
  } else server.send(400, "text/plain", "Missing angle");
}

void handleAll() {
  if (server.hasArg("angle") &&
      server.hasArg("expr")  &&
      server.hasArg("text")) {
    String cmd = "ALL:" + server.arg("angle") + ":" +
                 server.arg("expr")  + ":" +
                 server.arg("text");
    processCommand(cmd, "WiFi");
    server.send(200, "text/plain", "OK");
  } else server.send(400, "text/plain", "Missing params");
}

void handleStatus() {
  char jsonBuf[256];
  snprintf(jsonBuf, sizeof(jsonBuf),
           "{\"status\":\"online\",\"angle\":%d,\"expr\":\"%s\",\"mode\":\"%s\",\"volume\":%.3f,\"last_msg\":\"%s\"}",
           currentAngle, currentExpression.c_str(), connectionMode.c_str(), micVolume, lastMessage.c_str());
  server.send(200, "application/json", jsonBuf);
}

void handlePing() {
  processCommand("PING", "WiFi");
  server.send(200, "text/plain", "PONG:AI_ROBOT_ONLINE");
}

void handleAudio() {
  if (server.hasArg("plain") == false) {
    server.send(400, "text/plain", "No audio data");
    return;
  }
  
  String audioData = server.arg("plain");
  size_t len = audioData.length();
  
  Serial.print("Received Audio bytes: ");
  Serial.println(len);

  // Play audio chunk through MAX98357A I2S port
  size_t bytesWritten;
  i2s_write(I2S_PORT_TX, audioData.c_str(), len, &bytesWritten, portMAX_DELAY);
  
  server.send(200, "text/plain", "AUDIO_OK");
}

// Mic streaming: reads ~0.5s of 16kHz audio from INMP441
// and sends raw 16-bit PCM bytes to the dashboard
#define MIC_STREAM_SAMPLES 8000  // 0.5s at 16kHz
void handleMicStream() {
  // Read raw 32-bit I2S data from INMP441
  int32_t raw_samples[512];
  int16_t out_buf[MIC_STREAM_SAMPLES];
  int     out_idx = 0;

  while (out_idx < MIC_STREAM_SAMPLES) {
    size_t bytesRead = 0;
    int toRead = min(512, MIC_STREAM_SAMPLES - out_idx);
    i2s_read(I2S_PORT_RX, raw_samples, toRead * sizeof(int32_t),
             &bytesRead, pdMS_TO_TICKS(200));
    int count = bytesRead / sizeof(int32_t);
    for (int i = 0; i < count && out_idx < MIC_STREAM_SAMPLES; i++) {
      // Convert 32-bit I2S to 16-bit PCM (take upper 16 bits)
      out_buf[out_idx++] = (int16_t)(raw_samples[i] >> 16);
    }
    if (count == 0) break; // timeout
  }

  server.send(200, "application/octet-stream",
              String((const char*)out_buf, out_idx * sizeof(int16_t)));
}

// =============================================
//  SETUP
// =============================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Load persistent data early to set starting variables
  loadPersistentState();

  // ---- Display ----
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.sendBuffer();
  delay(300);

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.clearBuffer();
  u8g2.drawStr(20, 30, "AI ROBOT");
  u8g2.drawStr(15, 45, "Loading DB...");
  u8g2.sendBuffer();

  // ---- Servo ----
  myServo.attach(SERVO_PIN);
  myServo.write(currentAngle); // Restores angle from the database
  delay(300);

  // ---- I2S (Mic + Speaker) ----
  setupI2S();

  // ---- WiFi ----
  u8g2.clearBuffer();
  u8g2.drawStr(5, 35, "Connecting WiFi...");
  u8g2.sendBuffer();

  WiFi.begin(ssid, password);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500); tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    String ip = WiFi.localIP().toString();
    Serial.println("WiFi IP: " + ip);

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(0, 20, "WiFi Connected!");
    u8g2.drawStr(0, 35, ip.c_str());
    u8g2.sendBuffer();
    delay(3000);

    // WiFi routes
    server.on("/message", handleMsg);
    server.on("/expr",    handleExpr);
    server.on("/servo",   handleServo);
    server.on("/all",     handleAll);
    server.on("/status",  handleStatus);
    server.on("/ping",    handlePing);
    server.on("/audio",      HTTP_POST, handleAudio);
    server.on("/mic_stream", handleMicStream);
    server.begin();
  } else {
    Serial.println("WiFi failed — using BT/USB only");
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(0, 25, "WiFi Failed!");
    u8g2.drawStr(0, 40, "Use BT or USB");
    u8g2.sendBuffer();
    delay(2000);
  }

  // Startup eye animation
  eyeSurprised(); delay(600);
  smoothBlink();
  
  // Start up using the expression saved in our local DB
  setExpression(currentExpression); 

  Serial.println("=== AI ROBOT READY ===");
  Serial.println("Commands: PING | STATUS | MSG:text");
  Serial.println("          EXPR:happy | SERVO:90");
  Serial.println("          ALL:90:happy:Hello");
}

// =============================================
//  LOOP
// =============================================

int  autoExprIdx = 0;
String autoExprs[] = {
  "normal","happy","sad","angry","surprised",
  "wink","sleepy","love","confused","thinking"
};
int autoDelays[] = {
  4000,3000,3000,3000,3000,
  3000,4000,3000,3000,3000
};

void loop() {
  // Handle WiFi
  if (wifiConnected) server.handleClient();

  // Handle USB Serial
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    processCommand(cmd, "USB");
  }

  // ---- Periodic mic volume update (for status endpoint) ----
  if (millis() - lastMicCheck > 500) {
    lastMicCheck = millis();
    micVolume    = readMicVolume();
  }

  // ---- Auto blink ----
  if (millis() - lastBlink > random(3000, 6000)) {
    lastBlink = millis();
    smoothBlink();
  }

  // ---- Auto expression cycle when idle ----
  if (millis() - lastAutoExpr > (unsigned long)autoDelays[autoExprIdx]) {
    lastAutoExpr = millis();
    setExpression(autoExprs[autoExprIdx]);
    autoExprIdx = (autoExprIdx + 1) % 10;
  }
}
