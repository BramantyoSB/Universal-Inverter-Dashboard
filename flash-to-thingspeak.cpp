#define TS_ENABLE_SSL

#include "ThingSpeak.h"
#include "secrets.h"
#include <ModbusMaster.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#define WIFI_TIMEOUT_MS 20000
#define WIFI_RETRY_DELAY_MS 5000

WiFiClientSecure client;
ModbusMaster node;

// CONFIGURE DIBAWAH
//
// inverter Modbus slave ID
const uint8_t inverterID = 1;

// Terserah isinya apa tergantung register address,
// yang penting jgn lebih dari 8
uint16_t registersToRead[] = {
    /*
    0x0001, // AC Voltage
    0x0002, // AC Current
    0x0003, // AC Power
    0x0004, // DC Voltage
    0x0005, // DC Current
    0x0006, // DC Power
    0x0007, // Status Code
    0x0008  // Error Code
    */
};
const uint8_t numRegisters =
    sizeof(registersToRead) / sizeof(registersToRead[0]);
//
// STOP CONFIGURING

void connectToWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to WiFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - startAttemptTime < WIFI_TIMEOUT_MS) {
      Serial.print(".");
      delay(150);
    }

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println(" Failed. Retrying...");
      delay(WIFI_RETRY_DELAY_MS);
    } else {
      Serial.print(" Connected! IP: ");
      Serial.println(WiFi.localIP());
    }
  }
}

void setupModbus() {
  node.begin(inverterID, Serial2);

  // CONFIGURE DIBAWAH
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // nomor pin
  // STOP CONFIGURE
}

uint16_t readRegister(uint16_t reg) {
  uint8_t result = node.readHoldingRegisters(reg, 1);
  if (result == node.ku8MBSuccess) {
    return node.getResponseBuffer(0);
  } else {
    Serial.print("Error reading reg ");
    Serial.println(reg, HEX);
    return 0;
  }
}

void setup() {
  Serial.begin(9600); // baud rate kl bisa samain sama rekomendasi vendor
  connectToWiFi();

  client.setInsecure(); // simpen untuk testing
  ThingSpeak.begin(client);

  setupModbus();
}

void loop() {
  for (uint8_t i = 0; i < numRegisters; i++) {
    uint16_t value = readRegister(registersToRead[i]);

    if ((i == 6 || i == 7) && value == 0) {
      continue;
    }

    ThingSpeak.setField(i + 1, value);
    Serial.printf("Reg 0x%04X = %u\n", registersToRead[i], value);
  }

  ThingSpeak.writeFields(atoi(SECRET_CH_ID), SECRET_WRITE_APIKEY);

  delay(30000); // Kirim setiap 30 dtk
}
