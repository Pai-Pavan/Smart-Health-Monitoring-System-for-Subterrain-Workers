/*
  ==========================================================
  SMART HEALTH MONITORING SYSTEM FOR SUBTERRAIN WORKERS
  ----------------------------------------------------------
  RECEIVER NODE CODE
  ----------------------------------------------------------
  This ESP32 receives LoRa packets from transmitter node,
  verifies checksum, displays values on LCD,
  and activates buzzer if dangerous conditions occur.
  ==========================================================
*/

#include <SPI.h>
#include <LoRa.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// LCD object: I2C address 0x27, 16 columns x 2 rows
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Buzzer pin
#define buzzer 13

/*
  ==========================================================
  FUNCTION: printValues()
  ----------------------------------------------------------
  Extracts received sensor values and:
  1. Prints to Serial Monitor
  2. Displays on LCD
  3. Triggers buzzer if abnormal condition detected
  ==========================================================
*/
void printValues(uint8_t *values) {

  // Extract transmitted sensor values
  int t    = values[0];   // Temperature
  int h    = values[1];   // Humidity
  int ch4  = values[2];   // Methane gas ppm
  int co   = values[3];   // Carbon monoxide ppm
  int hr   = values[4];   // Heart rate
  int spo2 = values[5];   // Oxygen saturation

  // Print all received values to serial monitor
  Serial.print("Temp: "); Serial.println(t);
  Serial.print("Humidity: "); Serial.println(h);
  Serial.print("CH4: "); Serial.println(ch4);
  Serial.print("CO: "); Serial.println(co);
  Serial.print("HR: "); Serial.println(hr);
  Serial.print("SpO2: "); Serial.println(spo2);
  Serial.println("-----------------------------");

  // Clear LCD before writing fresh values
  lcd.clear();

  // First LCD row
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(t);
  lcd.print(" H:");
  lcd.print(h);
  lcd.print(" M:");
  lcd.print(ch4);

  // Second LCD row
  lcd.setCursor(0, 1);
  lcd.print("C:");
  lcd.print(co);
  lcd.print(" HR:");
  lcd.print(hr);
  lcd.print(" O2:");
  lcd.print(spo2);

  /*
    Activate buzzer if:
    - Temperature too high
    - Humidity too high
    - Methane dangerous
    - CO dangerous
    - Heart rate abnormal
    - SpO2 dangerously low
  */
  if (t > 40 || h > 90 || ch4 > 50 || co > 50 || hr > 140 || (spo2 < 80 && spo2 > 0))
    digitalWrite(buzzer, HIGH);
  else
    digitalWrite(buzzer, LOW);
}


/*
  ==========================================================
  FUNCTION: setup()
  ----------------------------------------------------------
  Runs once during startup:
  - Initializes Serial
  - Initializes LCD
  - Initializes LoRa
  - Configures buzzer pin
  ==========================================================
*/
void setup() {
  // Start serial communication
  Serial.begin(115200);
  // Start I2C communication
  Wire.begin(21, 22);

  // Initialize LCD
  lcd.init();
  lcd.backlight();

  // Configure buzzer pin
  pinMode(buzzer, OUTPUT);

  // Set LoRa pins: NSS, RESET, DIO0
  LoRa.setPins(5, 14, 2);
  // Start LoRa at 433 MHz
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed");
    while (1);
  }

  Serial.println("LoRa receiver ready");
}


/*
  ==========================================================
  FUNCTION: computeChecksum()
  ----------------------------------------------------------
  Recomputes checksum on received data
  Used to verify packet integrity
  ==========================================================
*/
uint8_t computeChecksum(uint8_t *values, uint8_t len) {
  uint8_t cs = 0;
  for (uint8_t i = 0; i < len; i++)
    cs ^= values[i];
  cs ^= 0xFF;
  return cs;
}

/*
  ==========================================================
  FUNCTION: loop()
  ----------------------------------------------------------
  Continuously checks for incoming LoRa packets:
  1. Reads packet
  2. Validates format
  3. Verifies checksum
  4. Displays sensor data
  ==========================================================
*/
void loop() {
  // Check if LoRa packet has arrived
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Print packet metadata
    Serial.print("PktSize: ");
    Serial.print(packetSize);
    Serial.print("  SNR: ");
    Serial.println(LoRa.packetSnr());

    // Temporary buffer for incoming packet bytes
    uint8_t buf[32];
    int idx = 0;

    // Read all bytes from LoRa packet
    while (LoRa.available() && idx < sizeof(buf)) {
      buf[idx++] = (uint8_t)LoRa.read();
    }

    /*
      Expected packet format:
      Byte 0 = Start marker (0xAA)
      Byte 1-6 = Sensor values
      Byte 7 = Checksum
    */
    if (idx >= 8 && buf[0] == 0xAA) {
      uint8_t values[6];
      // Extract 6 sensor values
      for (int i = 0; i < 6; i++)
        values[i] = buf[1 + i];
      // Extract checksum from received packet
      uint8_t receivedCS = buf[7];
      // Compute checksum locally
      uint8_t calculatedCS = computeChecksum(values, 6);
      // Compare checksums
      if (receivedCS == calculatedCS) {
        Serial.println("Packet OK:");
        // Process and display values
        printValues(values);
      } 
      else {
        Serial.println("Checksum FAIL!");
      }
    }
    else {
      Serial.println("Bad packet / wrong format");
    }
  }
}
