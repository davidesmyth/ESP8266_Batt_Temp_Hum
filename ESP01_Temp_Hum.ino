/*
  Heavily used code from github.com/thehookup
*/

#include <Wire.h>
#include "SHT21.h"  // from github.com/markbeee/SHT21

#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
ADC_MODE(ADC_VCC);

//USER CONFIGURED SECTION START//
const char* ssid = "YOUR SSID HERE";  // set WIFI network name
const char* password = "YOUR WIFI PSWD HERE";  // set WIFI password
const char* mqtt_server = "192.168.X.X";  // set MQTT server address
const int mqtt_port = 1883;  // set MQTT port
const char *mqtt_user = "USER";  // set MQTT user
const char *mqtt_pass = "PSWD";  // set MQTT user password
const char *mqtt_client_name = "temp_hum"; // Client connections cant have the same connection name
const char *mqtt_topic = "/Temp_Hum/ambient";  // set MQTT topic
IPAddress ip(192, 168, X, XXX);  // set a static IP for shorter connection times and longer battery life
IPAddress gateway(192, 168, X, X);  // set a static gateway (router IP) for shorter connection times...
IPAddress subnet(255, 255, 255, 0);  // ditto
//USER CONFIGURED SECTION END//

WiFiClient espClient;
PubSubClient client(espClient);

// Variables
bool boot = true;
char batteryVoltageMQTT[50];

float ftemp;
float humidity;

char message_buff[100];

const int BUFFER_SIZE = 300;

#define MQTT_MAX_PACKET_SIZE 512

SHT21 SHTsens;

//Functions

void sendState();

void setup_wifi() 
{
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
    {
      delay(50);
    }
}

void reconnect() 
{
  while (!client.connected()) 
  {
      delay(500);
      humidity = SHTsens.getHumidity();
      Serial.println(humidity);
      ftemp = SHTsens.getTemperature();
      Serial.println(ftemp);
            
      int battery_Voltage = ESP.getVcc() + 600;
      String temp_str = String(battery_Voltage);
      String mqttString = temp_str + "mV Replace Battery";
      mqttString.toCharArray(batteryVoltageMQTT, mqttString.length() + 1);
      if (battery_Voltage <= 2900)
      {
        if (client.connect(mqtt_client_name, mqtt_user, mqtt_pass, mqtt_topic, 0, 1, batteryVoltageMQTT)) 
        {
        
          if(boot == true)
          {
            sendState();
            boot = false;
          }
        } 
        else 
        {
          ESP.restart();
        }
      }
      if (battery_Voltage > 2900)
      {
        if (client.connect(mqtt_client_name, mqtt_user, mqtt_pass, mqtt_topic, 0, 1, "OFF")) 
        {
          if(boot == true)
          {
            sendState();
            boot = false;
          }
        } 
        else 
        {
          ESP.restart();
        }
      }
  }
}

void setup() 
{
  Wire.begin(0, 2);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() 
{
  
  if (boot == true) 
  {
    reconnect();
  }
  else
  {
    yield();
  ESP.deepSleep(30*60*1000*1000);
  }
}

void sendState() {
  
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  root["temperature"] = (String)ftemp;
  root["humidity"] = (String)humidity;

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  //Serial.println(buffer);
  client.publish(mqtt_topic, buffer, true);
}
