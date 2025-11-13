#include <Wire.h>
#include <Adafruit_INA219.h>
#include <WiFiS3.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include "core_cm4.h"  // Needed for NVIC_SystemReset()


//includes for e-ink display
//#include <GxEPD2_BW.h>
//#include <Fonts/FreeMonoBold9pt7b.h>

//GxEPD2_BW<GxEPD2_290, GxEPD2_290::HEIGHT> display(GxEPD2_290(/*CS=*/ 10, /*DC=*/ 9, /*RST=*/ 8, /*BUSY=*/ 7));
//void myeinkrefresh();



// Wi-Fi credentials


// MQTT broker (Raspberry Pi)
const char* mqtt_server = "bharatpi.local"; // or IP
int lcdBrightness = 1;
// Time sync
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000); // IST offset

// MQTT client
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// INA219 sensor
Adafruit_INA219 ina219;

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


  //display.init();
  
  // Full refresh example
  //display.setFullWindow();
  //display.firstPage();
  /*do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(10, 50);
    display.println("Hello E-Ink!");
  } while (display.nextPage());
  Serial.println("Display init done");
*/
  // Connect to hotspot
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("WiFi connected");

  Serial.print("You're connected to the network");
  printCurrentNet();
  printWifiData();

  // Start NTP and INA219
  timeClient.begin();
  timeClient.update();
  ina219.begin();

  // Connect to MQTT broker
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(callback);
}

void loop() {
//#if 1
  static bool mqttSub = false;
  timeClient.update();

  // Read sensor values
  float current_mA = /*0;//*/ ina219.getCurrent_mA();
  float voltage_V = /*0;//*/ina219.getBusVoltage_V();
  float power_mW = /*0;//*/ ina219.getPower_mW();
#if 1
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
    mqttSub = false;
    while (!mqttClient.connect("ArduinoClient")) {
      delay(5000);
    }
    
    //mqttClient.subscribe("sensorsreset");
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

  // Publish to topic
  mqttClient.publish("sensors/arduino_r4/current", payload.c_str());
 
  Serial.println(payload);
 #endif
 mqttClient.loop();
  //myeinkrefresh();
  static int counter =0;
  counter++;
  Serial.println("Iteration Done");
  Serial.println(counter);
  if(counter >= 100)
  {
    counter =0;
  }
  delay(1000); // 1000ms sampling
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



#if 0
void myeinkrefresh()
{
  display.setPartialWindow(0, 0, display.width(), 40);
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(10, 30);
    display.print("Count: ");
    display.print(millis() / 1000);
  } while (display.nextPage());
  
  delay(1000);
  
  // Do a full refresh every 50 updates to clear ghosting
  static int count = 0;
  if (++count >= 50) {
    display.setFullWindow();
    display.refresh();
    count = 0;
  }
}
#endif
