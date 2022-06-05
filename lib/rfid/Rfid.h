#include <SPI.h>
#include <MFRC522.h>
#include <functional>

#define SS_PIN 5
#define RST_PIN 0
#define RFID_CALLBACK_SIGNATURE std::function<void(byte, char *)> callback

class Rfid
{
private:
    MFRC522::MIFARE_Key key;
    MFRC522 rfid = MFRC522(SS_PIN, RST_PIN);
    byte nuidPICC[4] = {0, 0, 0, 0};
    MFRC522::StatusCode previousResult;
    bool locked = false;
    RFID_CALLBACK_SIGNATURE;
    String readDataFromCard();
    bool PICC_IsAnyCardPresent();
    void readCard();

public:
    enum Action : byte
    {
        PLAY,
        STOP
    };
    void setup(RFID_CALLBACK_SIGNATURE);
    void loop();
};