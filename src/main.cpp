#include <FS.h>
#include <SPIFFS.h>
#include <WiFiManager.h>
#include <Arduino.h>
#include <PubSubClient.h>
#include <Rfid.h>
#include <ArduinoJson.h>
#include <stdlib.h>     /* strtol */

char mqtt_server[40];
char mqtt_port[6] = "1883";
char mqtt_topic[34] = "sonos-controller/sonos";

WiFiClient espClient;
PubSubClient client(espClient);
#define MSG_BUFFER_SIZE (200)
char msg[MSG_BUFFER_SIZE];
boolean shouldSaveConfig = false;

Rfid rfid;

// callback notifying us of the need to save config
void saveConfigCallback()
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setupWifi()
{
  Serial.println("Setup Wifi");

  // restore config
  if (SPIFFS.begin(true))
  {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json"))
    {
      // file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile)
      {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if (!deserializeError)
        {
          Serial.println("\nparsed json");
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_topic, json["mqtt_topic"]);
        }
        else
        {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  }
  else
  {
    Serial.println("failed to mount FS");
  }

  WiFiManagerParameter custom_mqtt_server("server", "MQTT Server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_topic("topic", "MQTT Topic", mqtt_topic, 32);
  WiFiManager wifiManager;

  //wifiManager.resetSettings();

  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_topic);

  wifiManager.autoConnect("Sonos Controller");

  Serial.println("Wifi setup successful");

  // read updated values
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_topic, custom_mqtt_topic.getValue());

  // save config
  if (shouldSaveConfig)
  {
    DynamicJsonDocument json(1024);
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_topic"] = mqtt_topic;
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
      Serial.println("failed to open config file for writing");
    }
    serializeJson(json, Serial);
    serializeJson(json, configFile);
    configFile.close();
  }

}

void rfidCallback(char *action, char *data)
{
  if (action == "play")
  {
    String command = "{\"command\": \"play\", \"payload\": \"";
    command.concat(data);
    command.concat("\"}");
    Serial.print("Send play command:");
    Serial.println(command);
    snprintf(msg, MSG_BUFFER_SIZE, command.c_str());
    client.publish(mqtt_topic, msg);
  }

  if (action == "stop")
  {
    String command = "{\"command\": \"stop\"}";
    Serial.println("Send stop command");
    snprintf(msg, MSG_BUFFER_SIZE, command.c_str());
    client.publish(mqtt_topic, msg);
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
}

void mqttConnect()
{
  Serial.print("MQTT Server: ");
  Serial.println(mqtt_server);
  Serial.print("MQTT Port: ");
  Serial.println(mqtt_port);
  Serial.print("MQTT Topic: ");
  Serial.println(mqtt_topic);

  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(9600);
  setupWifi();

  //
  rfid = Rfid();
  rfid.setup(rfidCallback);

  // mqtt
  uint16_t mqtt_port_uint16 = (uint16_t)strtol(mqtt_port, NULL, 10);
  client.setServer(mqtt_server, mqtt_port_uint16);
  client.setCallback(mqttCallback);
}

void loop()
{
  if (!client.connected())
  {
    mqttConnect();
  }
  client.loop();

  rfid.loop();
}
