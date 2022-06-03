#include <Arduino.h>
#include <WiFiManager.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5
#define RST_PIN 0

char mqtt_server[40];
char mqtt_port[6] = "8080";
char mqtt_topic[34] = "topic";

MFRC522::MIFARE_Key key;
MFRC522 rfid = MFRC522(SS_PIN, RST_PIN);
byte nuidPICC[4] = {0, 0, 0, 0};

void setupWifi()
{
  Serial.println("Setup Wifi");
  WiFiManagerParameter custom_mqtt_server("server", "MQTT Server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_topic("topic", "MQTT Topic", mqtt_topic, 32);
  WiFiManager wifiManager;

  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_topic);

  wifiManager.autoConnect("Sonos Controller");

  Serial.println("Wifi setup successful");

  // read updated values
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_topic, custom_mqtt_topic.getValue());

  Serial.print("MQTT Broker: ");
  Serial.println(mqtt_server);
}

/**
   Helper routine to dump a byte array as hex values to Serial.
*/
void printHex(byte *buffer, byte bufferSize)
{
  for (byte i = 0; i < bufferSize; i++)
  {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}
/**
   Helper routine to dump a byte array as dec values to Serial.
*/
void printDec(byte *buffer, byte bufferSize)
{
  for (byte i = 0; i < bufferSize; i++)
  {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}

void sendPlayCommand(String value) {
  Serial.println(value);
}

void readRFID()
{ /* function readRFID */
  ////Read RFID card
  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }
  // Look for new 1 cards
  if (!rfid.PICC_IsNewCardPresent())
    return;
  // Verify if the NUID has been readed
  if (!rfid.PICC_ReadCardSerial())
    return;
  // Store NUID into nuidPICC array
  for (byte i = 0; i < 4; i++)
  {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }
  Serial.print(F("RFID In dec: "));
  printDec(rfid.uid.uidByte, rfid.uid.size);
  Serial.println();
  Serial.println();

  // Read data from the block
  Serial.print(F("Reading data from block "));
  Serial.println(F(" ..."));
  const int blockSize = 4;
  const int blocks = 8;
  String tagValue = "";

  for (int block = 0; block < blocks; block++)
  {
    byte blockAddr = 7 + blockSize * block;
    // Serial.print("Read Block:");
    // Serial.println(blockAddr);

    byte buffer[128];
    byte size = sizeof(buffer);
    MFRC522::StatusCode status = (MFRC522::StatusCode)rfid.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK)
    {
      Serial.print(F("MIFARE_Read() failed: "));
      Serial.println(rfid.GetStatusCodeName(status));

      if (status == 7) // status code 7 means the crc_a didn't match
      {
        rfid.PCD_Reset();
        rfid.PCD_Init();
      }
      return;
    }

    for (uint8_t i = 0; i < 16; i++)
    {
      tagValue += (char)buffer[i];
    }
  }

  tagValue.trim();

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  sendPlayCommand(tagValue);
}

void setup()
{
  Serial.begin(9600);
  setupWifi();

  SPI.begin();
  rfid.PCD_Reset();
  rfid.PCD_Init();
  Serial.print(F("Reader :"));
  rfid.PCD_DumpVersionToSerial();
}

void loop()
{
  readRFID();
}
