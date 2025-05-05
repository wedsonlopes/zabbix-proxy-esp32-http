#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>  // Ensure this is the correct library (e.g., ArduinoJson)

// Wi-Fi credentials
const char* ssid = "TESTE-2G"; // Your Network SSID (Name)
const char* password = "Teste@Teste"; // Your WIFI network password

// PHP server details
const char* serverUrl = "http://192.168.11.12:8000/zabbix_trap.php"; //zabbix proxy address
const char* hostName = "ESP32-SENSOR";  // Must match Zabbix host
const char* itemKey = "temperature";

void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
}

void sendTemperature(float temp) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    connectToWiFi();
  }

  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  // Create JSON payload
  JSONVar payload;
  payload["host"] = hostName;
  payload["key"] = itemKey;
  payload["value"] = temp;  // Send as number, not string
  String requestBody = JSON.stringify(payload);

  // Send POST request
  int httpCode = http.POST(requestBody);
  String response = http.getString();

  // Check response
  if (httpCode == 200) {
    Serial.println("Sent temperature: " + String(temp));
    Serial.println("Response: " + response);
  } else {
    Serial.println("Failed to send data. HTTP code: " + String(httpCode));
    Serial.println("Response: " + response);
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  connectToWiFi();
}

void loop() {
  float temp = random(180, 300) / 10.0;  // Generates random temp: 18.0 to 30.0
  sendTemperature(temp);
  delay(1000);  // Send every second
}
