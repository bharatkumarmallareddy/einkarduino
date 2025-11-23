#include <Wire.h>
#include <Adafruit_INA219.h>
#include <WiFiS3.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include "core_cm4.h"  // Needed for NVIC_SystemReset()
#include "arduino_secrets.h" 

#define LOOP_DELAY 1000

// Wi-Fi credentials

char ssid[] = SECRET_SSID;        // your network SSID (name)
char password[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)


// MQTT broker (Raspberry Pi)
const char* mqtt_server = "bharatpi.local"; // or IP
int lcdBrightness = 1;
// Time sync
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000); // IST offset

// MQTT client
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

//FileManagement
String fileName = "MES01";

// INA219 sensor
Adafruit_INA219 ina219;

//WiFiMQtt state Mgmt
static bool mqttSub = false;
static bool wifiConnected = true;


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  if(strcmp(topic,"sensorsreset") == 0)
  {
    Serial.println("reset Arduino");
    delay(1000);
    NVIC_SystemReset();
  }
  else if (strcmp(topic, "brightness") == 0) {
    // Convert payload to string
    String message;
    for (unsigned int i = 0; i < length; i++) {
      message += (char)payload[i];
    }

    // Convert string to integer
    int brightness = message.toInt();

    // Clamp brightness to valid range (0–255)
    brightness = constrain(brightness, 0, 255);

    // Set LCD brightness (example: using PWM pin)
    //analogWrite(LCD_BACKLIGHT_PIN, brightness);
    Serial.println("Brightness set to: " + String(brightness));
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);

  int wifiRetryCount = 0;
 
  // Connect to hotspot
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    wifiRetryCount++;
    if(10 == wifiRetryCount)
    {
      wifiConnected = false;
      Serial.println("Wifi Connection Failed");
      break;
    }
  }

  if(wifiConnected)
  {
    Serial.println("WiFi connected");

    Serial.print("You're connected to the network");
    printCurrentNet();
    printWifiData();

  // Start NTP and INA219
    timeClient.begin();
    timeClient.update();

    mqttClient.setServer(mqtt_server, 1883);
    mqttClient.setCallback(callback);
  }

  ina219.begin();

  // Connect to MQTT broker
  
}

void measureAndPublishCurrent()
{

  float current_mA = /*0;//*/ ina219.getCurrent_mA();
  float voltage_V = /*0;//*/ina219.getBusVoltage_V();
  float power_mW = /*0;//*/ ina219.getPower_mW();

  // Timestamp with milliseconds
  String timestamp = timeClient.getFormattedTime() + "." + String(millis() % 1000);

  // Format JSON payload
  String payload = "{";
  payload += "\"file\":\""+ getMeasurementFileName() + "\",";//+ "MES01"+ "\",";
  payload += "\"timestamp\":\"" + timestamp + "\",";
  payload += "\"current_mA\":" + String(current_mA, 2) + ",";
  payload += "\"voltage_V\":" + String(voltage_V, 2) + ",";
  payload += "\"power_mW\":" + String(power_mW, 2);
  payload += "}";

  if(wifiConnected)
  {
    mqttClient.publish("sensors/arduino_r4/current", payload.c_str());
  }

  Serial.println(payload);
}

String getMeasurementFileName()
{
  return fileName;
}



void loop() {
//#if 1
  
  

  if(wifiConnected){
    timeClient.update();
  // Ensure MQTT connection
    if (!mqttClient.connected()) {
      mqttSub = false;
      while (!mqttClient.connect("ArduinoClient")) {
        delay(5000);
        Serial.println(mqttClient.state());
      }
    }
    else
    {
      if(!mqttSub)
      {
        mqttClient.subscribe("sensorsreset");
        mqttClient.subscribe("brightness");
        mqttSub = true;
        Serial.println("TopicSubscribed");
      }
    }
  }

  // Publish to topic
 measureAndPublishCurrent();
 
 if(wifiConnected)
 {
  mqttClient.loop();
 }
 delay(LOOP_DELAY); // 1000ms sampling
}

void printWifiData() {
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  
  Serial.println(ip);

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);
}

void printCurrentNet() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  printMacAddress(bssid);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption, HEX);
  Serial.println();
}

void printMacAddress(byte mac[]) {
  for (int i = 0; i < 6; i++) {
    if (i > 0) {
      Serial.print(":");
    }
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
  }
  Serial.println();
}



