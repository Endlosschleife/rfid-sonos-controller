#include "Rfid.h"


String Rfid::readDataFromCard()
{ /* function readRFID */
    ////Read RFID card
    for (byte i = 0; i < 6; i++)
    {
        key.keyByte[i] = 0xFF;
    }

    // Look for new 1 cards
    // if (!rfid.PICC_IsNewCardPresent()) {
    //   return;
    // }
    // Verify if the NUID has been readed
    // if (!rfid.PICC_ReadCardSerial()) {
    //   Serial.println("Not read card");
    //   return;
    // }
    // Store NUID into nuidPICC array
    for (byte i = 0; i < 4; i++)
    {
        nuidPICC[i] = rfid.uid.uidByte[i];
    }

    // Read data from the block
    Serial.print(F("Reading data from block "));
    Serial.println(F(" ..."));
    const int blockSize = 4;
    const int blocks = 8;
    String tagValue = "";

    for (int block = 0; block < blocks; block++)
    {
        byte blockAddr = 7 + blockSize * block;
        byte buffer[128];
        byte size = sizeof(buffer);
        MFRC522::StatusCode status = (MFRC522::StatusCode)rfid.MIFARE_Read(blockAddr, buffer, &size);
        // if (status != MFRC522::STATUS_OK)
        // {
        //   Serial.print(F("MIFARE_Read() failed: "));
        //   Serial.println(rfid.GetStatusCodeName(status));

        //   if (status == 7) // status code 7 means the crc_a didn't match
        //   {
        //     rfid.PCD_Reset();
        //     rfid.PCD_Init();
        //   }
        //   return;
        // }

        uint8_t i = 0;
        while (i < 16 && buffer[i] < 127 && buffer[i] > 0) // stop when special character begins or is null
        {
            Serial.print(buffer[i]);
            Serial.print(" ");
            tagValue += (char)buffer[i];
            i++;
        }
    }

    tagValue.trim();
    tagValue = tagValue.substring(2);

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    return tagValue;
}

/**
 * Returns true if a PICC responds to PICC_CMD_WUPA.
 * All cards in state IDLE or HALT are invited.
 *
 * @return bool
 */
bool Rfid::PICC_IsAnyCardPresent()
{
    byte bufferATQA[2];
    byte bufferSize = sizeof(bufferATQA);

    // Reset baud rates
    rfid.PCD_WriteRegister(rfid.TxModeReg, 0x00);
    rfid.PCD_WriteRegister(rfid.RxModeReg, 0x00);
    // Reset ModWidthReg
    rfid.PCD_WriteRegister(rfid.ModWidthReg, 0x26);

    MFRC522::StatusCode result = rfid.PICC_WakeupA(bufferATQA, &bufferSize);
    return (result == MFRC522::STATUS_OK || result == MFRC522::STATUS_COLLISION);
}

void Rfid::readCard()
{
    // Wake up all cards present within the sensor/reader range.
    bool cardPresent = PICC_IsAnyCardPresent();

    // Reset the loop if no card was locked an no card is present.
    // This saves the select process when no card is found.
    if (!locked && !cardPresent)
        return;

    // When a card is present (locked) the rest ahead is intensive (constantly checking if still present).
    // Consider including code for checking only at time intervals.

    // Ask for the locked card (if rfid.uid.size > 0) or for any card if none was locked.
    // (Even if there was some error in the wake up procedure, attempt to contact the locked card.
    // This serves as a double-check to confirm removals.)
    // If a card was locked and now is removed, other cards will not be selected until next loop,
    // after rfid.uid.size has been set to 0.
    MFRC522::StatusCode result = rfid.PICC_Select(&rfid.uid, 8 * rfid.uid.size);

    if (!locked && result == MFRC522::STATUS_OK)
    {
        locked = true;
        // Action on card detection.
        Serial.println(F("Card found."));
        String data = readDataFromCard();
        callback("play", strcpy(new char[data.length() + 1], data.c_str()));
    }
    else if (locked && result != MFRC522::STATUS_OK)
    {
        locked = false;
        rfid.uid.size = 0;
        // Action on card removal.
        Serial.println(F("Card has been removed."));
        callback("stop", "");
    }
    else if (!locked && result != MFRC522::STATUS_OK)
    {
        // Clear locked card data just in case some data was retrieved in the select procedure
        // but an error prevented locking.
        rfid.uid.size = 0;
    }

    rfid.PICC_HaltA();
}

void Rfid::setup(RFID_CALLBACK_SIGNATURE)
{
    this->callback = callback;
    SPI.begin();
    rfid.PCD_Reset();
    rfid.PCD_Init();
}

void Rfid::loop()
{
    readCard();
}