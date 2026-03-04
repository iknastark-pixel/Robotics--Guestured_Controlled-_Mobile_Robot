#include <esp_now.h>
#include <WiFi.h>

#define IN1 16
#define IN2 17
#define IN3 18
#define IN4 19

// ─── TUNING ZONE ───────────────────────────────────────────
#define DEAD_ZONE     0.25   // below this on BOTH axes = stop
#define MOVE_THRESH   0.55   // medium sensitivity — needs real tilt
#define STOP_DELAY    100    // ms safety cutoff if signal drops
// ───────────────────────────────────────────────────────────

typedef struct struct_message {
  float pitch;
  float roll;
} struct_message;

struct_message incomingData;
unsigned long lastRecvTime = 0;

// ── Motor Functions ─────────────────────────────────────────
void stopMotors() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW);
}

// Pure directions
void forward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}
void backward() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
}
void turnLeft() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}
void turnRight() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
}

// Diagonal combinations
void forwardLeft() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW);   // left motor off
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);   // right motor forward
}
void forwardRight() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);   // left motor forward
  digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW);   // right motor off
}
void backwardLeft() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW);   // left motor off
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);  // right motor backward
}
void backwardRight() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);  // left motor backward
  digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW);   // right motor off
}

// ── Gesture Decision Logic ───────────────────────────────────
void handleGesture(float pitch, float roll) {

  bool goForward  = pitch >  MOVE_THRESH;
  bool goBackward = pitch < -MOVE_THRESH;
  bool goRight    = roll  >  MOVE_THRESH;
  bool goLeft     = roll  < -MOVE_THRESH;

  // ── Diagonal combos first ──
  if (goForward  && goRight)  { Serial.println(">> FWD-RIGHT");  forwardRight();  return; }
  if (goForward  && goLeft)   { Serial.println(">> FWD-LEFT");   forwardLeft();   return; }
  if (goBackward && goRight)  { Serial.println(">> BACK-RIGHT"); backwardRight(); return; }
  if (goBackward && goLeft)   { Serial.println(">> BACK-LEFT");  backwardLeft();  return; }

  // ── Pure directions ──
  if (goForward)  { Serial.println(">> FORWARD");  forward();   return; }
  if (goBackward) { Serial.println(">> BACKWARD"); backward();  return; }
  if (goRight)    { Serial.println(">> RIGHT");    turnRight(); return; }
  if (goLeft)     { Serial.println(">> LEFT");     turnLeft();  return; }

  stopMotors();
}

// ── ESP-NOW Callback ─────────────────────────────────────────
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataRaw, int len) {

  memcpy(&incomingData, incomingDataRaw, sizeof(incomingData));
  lastRecvTime = millis();

  float pitch = incomingData.pitch;
  float roll  = incomingData.roll;

  Serial.print("P: "); Serial.print(pitch, 2);
  Serial.print("  R: "); Serial.println(roll, 2);

  // Both axes inside dead zone = flat hand = stop
  if (abs(pitch) < DEAD_ZONE && abs(roll) < DEAD_ZONE) {
    Serial.println("-- STOP (flat)");
    stopMotors();
    return;
  }

  handleGesture(pitch, roll);
}

// ── Setup ────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  stopMotors();

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Failed");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("Robot Ready");
}

// ── Safety Stop if signal drops ───────────────────────────────
void loop() {
  if (millis() - lastRecvTime > STOP_DELAY && lastRecvTime != 0) {
    stopMotors();
  }
}