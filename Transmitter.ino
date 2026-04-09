/***********************************************************************
 SMART HEALTH MONITORING SYSTEM FOR SUBTERRAIN WORKERS
 ----------------------------------------------------------------------
 Transmitter Node (ESP32 + LoRa)
 Sensors Used:
 1. DHT22      -> Temperature + Humidity
 2. MQ4        -> Methane Gas Approximate PPM
 3. MQ7        -> Carbon Monoxide Approximate PPM
 4. MAX30102   -> Real Heart Rate + SpO2
 5. LoRa SX1278 -> Wireless transmission
 ***********************************************************************/

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <DHT.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

// PIN DEFINITIONS
#define DHTPIN 27
#define DHTTYPE DHT22

#define MQ4_PIN 35
#define MQ7_PIN 34

#define LORA_SS   5
#define LORA_RST  14
#define LORA_DIO0 2

// OBJECTS
DHT dht(DHTPIN, DHTTYPE);
MAX30105 particleSensor;

// MAX30102 BUFFER
#define BUFFER_SIZE 100
uint32_t irBuffer[BUFFER_SIZE];
uint32_t redBuffer[BUFFER_SIZE];

int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

// MQ SENSOR CALIBRATION CONSTANTS
float RL_MQ4 = 10.0;
float RL_MQ7 = 10.0;

float RO_MQ4 = 4.4;
float RO_MQ7 = 5.0;

// CHECKSUM FUNCTION
uint8_t computeChecksum(uint8_t *vals, uint8_t len) {
  uint8_t cs = 0;
  for (uint8_t i = 0; i < len; i++) 
    cs ^= vals[i];
  cs ^= 0xFF;
  return cs;
}

// MQ PPM CALCULATION
float calculatePPM(int adcValue, float Ro, float RL, float a, float b) {
  float voltage = adcValue * (3.3 / 4095.0);
  if (voltage <= 0.01) 
    voltage = 0.01;
  float Rs = ((3.3 - voltage) / voltage) * RL;
  float ratio = Rs / Ro;
  float ppm = a * pow(ratio, b);
  return ppm;
}

// MAX30102 SETUP
void setupMAX30102() {
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 not found!");
    while (1);
  }
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x1F);
  particleSensor.setPulseAmplitudeIR(0x1F);
  particleSensor.setPulseAmplitudeGreen(0);
  Serial.println("MAX30102 Ready");
}

// READ HEART RATE + SpO2
void readHeartAndSpO2(uint8_t &hr, uint8_t &spo2Val) {
  for (byte i = 0; i < BUFFER_SIZE; i++) {
    while (!particleSensor.available())
      particleSensor.check();
    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample();
  }
  maxim_heart_rate_and_oxygen_saturation(irBuffer, BUFFER_SIZE, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  // Finger not detected
  if (irBuffer[BUFFER_SIZE - 1] < 50000) {
    hr = 0;
    spo2Val = 0;
    return;
  }
  if (validHeartRate && heartRate > 30 && heartRate < 220)
    hr = (uint8_t)heartRate;
  else
    hr = 0;
  if (validSPO2 && spo2 >= 70 && spo2 <= 100)
    spo2Val = (uint8_t)spo2;
  else
    spo2Val = 0;
}

// SEND LORA PACKET
void sendSensorValues(uint8_t t, uint8_t h, uint8_t ch4, uint8_t co, uint8_t hr, uint8_t spo2Val) {
  uint8_t payload[6] = {t, h, ch4, co, hr, spo2Val};
  uint8_t cs = computeChecksum(payload, 6);

  LoRa.beginPacket();
  LoRa.write(0xAA);
  LoRa.write(payload, 6);
  LoRa.write(cs);
  LoRa.endPacket();

  Serial.println("LoRa Packet Sent");
}

// SETUP
void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(MQ4_PIN, INPUT);
  pinMode(MQ7_PIN, INPUT);

  // LoRa init
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }
  // MAX30102 init
  setupMAX30102();

  Serial.println("System Ready");
}

// LOOP
void loop() {
  // DHT22
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();
  if (isnan(temp)) temp = 0;
  if (isnan(hum)) hum = 0;
  uint8_t t = constrain((int)temp, 0, 99);
  uint8_t h = constrain((int)hum, 0, 99);

  // MQ4 METHANE
  int mq4ADC = analogRead(MQ4_PIN);
  float methanePPM = calculatePPM(mq4ADC, RO_MQ4, RL_MQ4, 1000.0, -2.95); // 1000.0 and -2.95 = methane curve constants from MQ4 datasheet

  // MQ7 CO
  int mq7ADC = analogRead(MQ7_PIN);
  float coPPM = calculatePPM(mq7ADC, RO_MQ7, RL_MQ7, 99.042, -1.518); // 99.042 and -1.518 = CO curve constants from MQ7 datasheet

  uint8_t ch4 = constrain((int)methanePPM, 0, 255);
  uint8_t co  = constrain((int)coPPM, 0, 255);

  // MAX30102
  uint8_t hr = 0;
  uint8_t spo2Val = 0;
  readHeartAndSpO2(hr, spo2Val);

  // DEBUG OUTPUT
  Serial.println("==================================");
  Serial.print("Temperature: "); Serial.println(t);
  Serial.print("Humidity: "); Serial.println(h);
  Serial.print("Methane PPM: "); Serial.println(ch4);
  Serial.print("CO PPM: "); Serial.println(co);
  Serial.print("Heart Rate: "); Serial.println(hr);
  Serial.print("SpO2: "); Serial.println(spo2Val);

  // SEND VIA LORA
  sendSensorValues(t, h, ch4, co, hr, spo2Val);

  delay(2000);
}
