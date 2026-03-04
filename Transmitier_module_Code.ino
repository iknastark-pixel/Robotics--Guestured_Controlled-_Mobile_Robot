#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

uint8_t receiverMAC[] = {0xD8, 0x13, 0x2A, 0x32, 0x6B, 0x38};

typedef struct struct_message {
  float pitch;
  float roll;
} struct_message;

struct_message data;

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Wire.begin(21, 22);
  Wire.setClock(100000);

  mpu.initialize();
  mpu.setSleepEnabled(false);
  delay(500);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Failed");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  Serial.println("Transmitter Ready");
}

void loop() {

  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);

  // Convert raw acceleration to tilt scale
  data.pitch = ax / 8000.0;   // more responsive
  data.roll  = ay / 8000.0;

  esp_now_send(receiverMAC, (uint8_t *)&data, sizeof(data));

  Serial.print("Pitch: ");
  Serial.print(data.pitch);
  Serial.print("  Roll: ");
  Serial.println(data.roll);

  delay(80);
}