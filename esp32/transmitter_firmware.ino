/* ═══════════════════════════════════════════════════════════════════════════
   FIREFIGHTER MONITORING SYSTEM — ESP32 Transmitter (Sensor Node)
   ═══════════════════════════════════════════════════════════════════════════
   Reads all onboard sensors and transmits raw data via ESP-NOW to the
   receiver node for TinyML processing.

   Hardware:
     - ESP32 DevKit V1
     - DHT22  → GPIO 4  (temperature + humidity)
     - MQ2    → GPIO 34 (smoke)
     - MQ9    → GPIO 35 (CO)
     - MQ135  → GPIO 32 (air quality)
     - MPU6050 → I2C (SDA=21, SCL=22)
     - NEO-6M → Serial2 (RX=16, TX=17)

   Upload:
     Board  → ESP32 Dev Module
     Speed  → 115200
     ═══════════════════════════════════════════════════════════════════════════ */

#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <DHT.h>
#include <TinyGPSPlus.h>

// ── Configuration ─────────────────────────────────────────────────────────
#define NODE_ID             101      // Unique ID for this firefighter node

// Pin Definitions
#define DHT_PIN             4
#define DHT_TYPE            DHT22
#define MQ2_PIN             34
#define MQ9_PIN             35
#define MQ135_PIN           32
#define MPU6050_ADDR        0x68
#define GPS_RX_PIN          16
#define GPS_TX_PIN          17
#define LED_PIN             2

// Timing
#define SEND_INTERVAL_MS    200      // Send data every 200ms (5Hz for IMU)
#define GPS_BAUD            9600
#define SERIAL_BAUD         115200

// ── Receiver MAC Address ──────────────────────────────────────────────────
// IMPORTANT: Replace with the actual MAC address of your receiver ESP32.
// You can find it by running WiFi.macAddress() on the receiver and reading
// the serial output.
uint8_t receiverMAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // broadcast

// ── ESP-NOW Packet Structure ──────────────────────────────────────────────
// Must match the receiver's struct exactly
typedef struct __attribute__((packed)) {
  uint8_t   node_id;
  float     temperature;
  float     humidity;
  uint16_t  mq2;
  uint16_t  mq9;
  uint16_t  mq135;
  float     accel_x;
  float     accel_y;
  float     accel_z;
  float     gyro_x;
  float     gyro_y;
  float     gyro_z;
  float     latitude;
  float     longitude;
  uint32_t  timestamp_ms;
} SensorPacket;

// ── Global Objects ────────────────────────────────────────────────────────
DHT dht(DHT_PIN, DHT_TYPE);
TinyGPSPlus gps;
HardwareSerial GPSSerial(2);
SensorPacket packet;

// ── ESP-NOW Send Callback ─────────────────────────────────────────────────
esp_now_peer_info_t peerInfo;
bool lastSendSuccess = false;

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  lastSendSuccess = (status == ESP_NOW_SEND_SUCCESS);
}

// ── MPU6050 Setup ─────────────────────────────────────────────────────────
void setupMPU6050() {
  Wire.begin();
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0x00);  // Wake up MPU6050
  Wire.endTransmission(true);

  // Configure accelerometer range: ±16g
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x1C);  // ACCEL_CONFIG register
  Wire.write(0x18);  // ±16g
  Wire.endTransmission(true);

  // Configure gyroscope range: ±2000°/s
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x1B);  // GYRO_CONFIG register
  Wire.write(0x18);  // ±2000°/s
  Wire.endTransmission(true);

  Serial.println("  ✓ MPU6050 initialized (±16g, ±2000°/s)");
}

// ── Read MPU6050 ──────────────────────────────────────────────────────────
void readMPU6050(float &ax, float &ay, float &az,
                 float &gx, float &gy, float &gz) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x3B);  // Starting register for accel data
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)MPU6050_ADDR, (uint8_t)14, (uint8_t)true);

  int16_t raw_ax = Wire.read() << 8 | Wire.read();
  int16_t raw_ay = Wire.read() << 8 | Wire.read();
  int16_t raw_az = Wire.read() << 8 | Wire.read();
  Wire.read(); Wire.read(); // Skip temperature
  int16_t raw_gx = Wire.read() << 8 | Wire.read();
  int16_t raw_gy = Wire.read() << 8 | Wire.read();
  int16_t raw_gz = Wire.read() << 8 | Wire.read();

  // Convert to physical units
  // ±16g range → sensitivity = 2048 LSB/g
  ax = (float)raw_ax / 2048.0f;
  ay = (float)raw_ay / 2048.0f;
  az = (float)raw_az / 2048.0f;

  // ±2000°/s range → sensitivity = 16.4 LSB/(°/s)
  gx = (float)raw_gx / 16.4f;
  gy = (float)raw_gy / 16.4f;
  gz = (float)raw_gz / 16.4f;
}

// ── Read Gas Sensors ──────────────────────────────────────────────────────
void readGasSensors(uint16_t &mq2, uint16_t &mq9, uint16_t &mq135) {
  // Read raw 12-bit ADC values (0–4095)
  // In production, apply calibration curves to convert to PPM
  mq2   = analogRead(MQ2_PIN);
  mq9   = analogRead(MQ9_PIN);
  mq135 = analogRead(MQ135_PIN);
}

// ── Read GPS ──────────────────────────────────────────────────────────────
void readGPS(float &lat, float &lon) {
  while (GPSSerial.available() > 0) {
    gps.encode(GPSSerial.read());
  }

  if (gps.location.isValid()) {
    lat = (float)gps.location.lat();
    lon = (float)gps.location.lng();
  }
  // If invalid, lat/lon retain their previous values
}

// ══════════════════════════════════════════════════════════════════════════
//  SETUP
// ══════════════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(500);

  Serial.println();
  Serial.println("╔══════════════════════════════════════════════════╗");
  Serial.println("║  FIREFIGHTER MONITORING — TRANSMITTER NODE      ║");
  Serial.printf( "║  Node ID: %d                                   ║\n", NODE_ID);
  Serial.println("╚══════════════════════════════════════════════════╝");
  Serial.println();

  // LED indicator
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialize sensors
  Serial.println("Initializing sensors...");
  dht.begin();
  Serial.println("  ✓ DHT22 ready");

  setupMPU6050();

  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("  ✓ GPS (NEO-6M) UART started");

  // Configure analog resolution (ESP32 default is 12-bit)
  analogReadResolution(12);
  Serial.println("  ✓ Gas sensors (MQ2/MQ9/MQ135) ready");

  // Initialize WiFi in station mode for ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.printf("  ✓ WiFi STA mode — MAC: %s\n", WiFi.macAddress().c_str());

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("  ✗ ESP-NOW init FAILED — rebooting...");
    delay(1000);
    ESP.restart();
  }
  Serial.println("  ✓ ESP-NOW initialized");

  esp_now_register_send_cb(onDataSent);

  // Register receiver peer
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;   // Use current WiFi channel
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("  ✗ Failed to add peer");
  } else {
    Serial.println("  ✓ Receiver peer registered");
  }

  // Initialize packet with defaults
  memset(&packet, 0, sizeof(packet));
  packet.node_id = NODE_ID;
  packet.latitude  = 12.9692f;   // Default VIT Vellore coordinates
  packet.longitude = 79.1559f;

  Serial.println();
  Serial.printf("Transmitting every %dms...\n\n", SEND_INTERVAL_MS);
}

// ══════════════════════════════════════════════════════════════════════════
//  LOOP
// ══════════════════════════════════════════════════════════════════════════
unsigned long lastSendTime = 0;
unsigned long packetCount = 0;

void loop() {
  unsigned long now = millis();

  if (now - lastSendTime < SEND_INTERVAL_MS) return;
  lastSendTime = now;

  // ── Collect sensor data ──────────────────────────────────────────────
  // DHT22 (temperature + humidity) — slow sensor, ~2s response time
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();
  if (!isnan(temp)) packet.temperature = temp;
  if (!isnan(hum))  packet.humidity    = hum;

  // Gas sensors
  readGasSensors(packet.mq2, packet.mq9, packet.mq135);

  // IMU (accelerometer + gyroscope)
  readMPU6050(packet.accel_x, packet.accel_y, packet.accel_z,
              packet.gyro_x,  packet.gyro_y,  packet.gyro_z);

  // GPS
  readGPS(packet.latitude, packet.longitude);

  // Timestamp
  packet.timestamp_ms = now;

  // ── Transmit via ESP-NOW ─────────────────────────────────────────────
  esp_err_t result = esp_now_send(receiverMAC, (uint8_t *)&packet, sizeof(packet));

  // LED feedback
  if (result == ESP_OK) {
    digitalWrite(LED_PIN, HIGH);
    delayMicroseconds(50);
    digitalWrite(LED_PIN, LOW);
  }

  // ── Debug output (every 10th packet) ─────────────────────────────────
  packetCount++;
  if (packetCount % 10 == 0) {
    Serial.printf("[%lu] T:%.1f°C H:%.0f%% MQ2:%u MQ9:%u MQ135:%u "
                  "Ax:%.2f Ay:%.2f Az:%.2f GPS:%.4f,%.4f %s\n",
                  packetCount,
                  packet.temperature, packet.humidity,
                  packet.mq2, packet.mq9, packet.mq135,
                  packet.accel_x, packet.accel_y, packet.accel_z,
                  packet.latitude, packet.longitude,
                  lastSendSuccess ? "✓" : "✗");
  }
}
