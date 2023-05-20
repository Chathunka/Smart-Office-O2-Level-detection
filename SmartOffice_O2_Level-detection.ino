#include <esp32ModbusRTU.h>
esp32ModbusRTU modbus(&Serial2, 4);
byte address=2;

#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_task_wdt.h"

const char* ssid = "";
const char* password = "";
const int wifiConnectionTimeout = 20;
const int loopTimeout = 5;
float deviceData = 22.10;

void Init_modbus(){
    Serial2.begin(4800);  // Modbus connection
    Serial.println("Init MODBUS");
    modbus.onData([](uint8_t serverAddress, esp32Modbus::FunctionCode fc, uint16_t address, uint8_t* data, size_t length) {
      digitalWrite(2,HIGH);
      uint16_t combinedValue = (data[0] << 8) | data[1];
      long decimalValue = strtol(String(combinedValue, HEX).c_str(), NULL, 16);
      float floatValue = decimalValue / 10.0;
      Serial.print("O2 vol : ");
      Serial.print(floatValue);
      Serial.print("%");
      sendDatatoServer(floatValue);
    }
  );
  modbus.onError([](esp32Modbus::Error error) {
     Serial.printf("error: 0x%02x\n\n", static_cast<uint8_t>(error));
  });
  modbus.begin();
}

void sendDatatoServer(float deviceData){
    HTTPClient http;
    http.begin("http://serverip/api/v1/device/0xD101");
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode;
    String deviceDataStr = String(deviceData);
    String jsonData = "{\"deviceName\":\"O2-Sensor\",\"deviceStatus\":0,\"type\":\"Gauge\",\"location\":\"Edge\",\"deviceData\":\"" + deviceDataStr + "\"}";
    httpResponseCode = http.PUT(jsonData);
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
    esp_task_wdt_reset();
}

void setup() {
  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    if (millis() > wifiConnectionTimeout * 1000) {
      Serial.println("Wi-Fi connection timeout. Resetting...");
      ESP.restart();
    }
  }
  Serial.println("Connected to WiFi");
  esp_task_wdt_init(loopTimeout, true);
  Init_modbus();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    modbus.readHoldingRegisters(address,0x00,1);
    esp_task_wdt_reset();
  } else {
    Serial.println("Wi-Fi disconnected. Resetting...");
    ESP.restart();
  }
  digitalWrite(2,LOW);
  delay(5000);
}


