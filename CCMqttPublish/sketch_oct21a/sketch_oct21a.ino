#include <Wire.h>
#include <Adafruit_INA219.h>
#include <WiFiS3.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <PubSubClient.h>




// Wi-Fi credentials
const char* ssid = "PiHotspot";
const char* password = "raspberry123";

// MQTT broker (Raspberry Pi)
const char* mqtt_server = "bharatpi.local"; // or IP

// Time sync
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000); // IST offset

// MQTT client
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// INA219 sensor
Adafruit_INA219 ina219;

void setup() {
  Serial.begin(115200);

  // Connect to hotspot
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("WiFi connected");

  // Start NTP and INA219
  timeClient.begin();
  timeClient.update();
  ina219.begin();

  // Connect to MQTT broker
  mqttClient.setServer(mqtt_server, 1883);
}

void loop() {
  timeClient.update();

  // Read sensor values
  float current_mA = ina219.getCurrent_mA();
  float voltage_V = ina219.getBusVoltage_V();
  float power_mW = ina219.getPower_mW();

  // Timestamp with milliseconds
  String timestamp = timeClient.getFormattedTime() + "." + String(millis() % 1000);

  // Format JSON payload
  String payload = "{";
  payload += "\"timestamp\":\"" + timestamp + "\",";
  payload += "\"current_mA\":" + String(current_mA, 2) + ",";
  payload += "\"voltage_V\":" + String(voltage_V, 2) + ",";
  payload += "\"power_mW\":" + String(power_mW, 2);
  payload += "}";

  // Ensure MQTT connection
  if (!mqttClient.connected()) {
    while (!mqttClient.connect("ArduinoClient")) {
      delay(5000);
    }
  }

  // Publish to topic
  mqttClient.publish("sensors/arduino_r4/current", payload.c_str());
  Serial.println(payload);

  delay(1000); // 1000ms sampling
}
