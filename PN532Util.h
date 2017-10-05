
#ifndef TASK_MAX7912
#define TASK_MAX7912 0
#endif

#ifndef TASK_RESTCLIENT
#define TASK_RESTCLIENT 0
#endif
#ifndef EVT_WAIT_ALLEVENT
#define EVT_WAIT_ALLEVENT 0x0f
#endif

#include <Wire.h>
// #include "Adafruit_PN532.h"

#define PN532_IRQ   (12)
#define PN532_RESET (13)  // Not connected by default on the NFC Shield
#define PN532_SCL   (4)
#define PN532_SDA   (3)


#include "PN532_I2C.h"
#include "PN532.h"
#include "NfcAdapter.h"

PN532_I2C pn532_i2c(Wire1);
// NfcAdapter nfc = NfcAdapter(pn532_i2c);
PN532 nfc = PN532(pn532_i2c);


// Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

#include "NdefMessage.h"

NdefMessage createNdefMessage()
{
  NdefMessage message = NdefMessage();
  message.addUriRecord("http://arduino.cc");
}


void initPN532NFC(void) {

  Serial.println("-------Peer to Peer HCE--------");

 nfc.begin();

    // Reset the PN532
    digitalWrite(PN532_RESET, HIGH);
    digitalWrite(PN532_RESET, LOW);
    delay(400);
    digitalWrite(PN532_RESET, HIGH);
    delay(10);  

                
    uint32_t versiondata = nfc.getFirmwareVersion();
  Serial.print("PN532 Version="); Serial.println(versiondata);
  if (! versiondata) {
    Serial.print("Didn't find PN53x board, halt!!");
    while (1); // halt
  }

  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);

  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  //nfc.setPassiveActivationRetries(0xFF);

  // configure board to read RFID tags
  nfc.SAMConfig();
}


uint8_t message[4096];

uint8_t jobPN532NFC_XXX()
{
  bool success;

  uint8_t responseLength = 32;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  Serial.println("Waiting for an ISO14443A card");

  // set shield to inListPassiveTarget
  // success = nfc.inListPassiveTarget();
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  if (success) {

    Serial.println("Found something!");

    uint8_t selectApdu[] = { 0x00, /* CLA */
                             0xA4, /* INS */
                             0x04, /* P1  */
                             0x00, /* P2  */
                             0x07, /* Length of AID  */
                             0xF0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, /* AID defined on Android App */
                             0x00  /* Le  */
                           };

    uint8_t response[32];

    success = nfc.inDataExchange(selectApdu, sizeof(selectApdu), response, &responseLength);

    if (success) {

      Serial.print("responseLength: "); Serial.println(responseLength);

      nfc.PrintHexChar(response, responseLength);

      do {
        uint8_t apdu[] = "Hello from Arduino";
        uint8_t back[32];
        uint8_t length = 32;

        success = nfc.inDataExchange(apdu, sizeof(apdu), back, &length);

        if (success) {

          Serial.print("responseLength: "); Serial.println(length);

          nfc.PrintHexChar(back, length);
        }
        else {

          Serial.println("Broken connection?");
        }
      }
      while (success);
    }
    else {

      Serial.println("Failed sending SELECT AID");
    }
  }
  else {

    Serial.println("Didn't find anything!");
  }

  delay(1000);
}

void printResponse(uint8_t *response, uint8_t responseLength)
{
  String respBuffer;
  for (int i = 0; i < responseLength; i++) {

    if (response[i] < 0x10)
      respBuffer = respBuffer + "0"; //Adds leading zeros if hex value is smaller than 0x10

    respBuffer = respBuffer + String(response[i], HEX) + " ";
  }

  Serial.print("response: "); Serial.println(respBuffer);
}

uint8_t jobPN532NFC()
{
  uint8_t success;                          // Flag to check if there was an error with the PN532
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  uint8_t currentblock;                     // Counter to keep track of which block we're on
  bool authenticated = false;               // Flag to indicate if the sector is authenticated
  // uint8_t data[16];                         // Array to store block data during reads
  uint8_t *data;
  // Keyb on NDEF and Mifare Classic should be the same
  uint8_t keyuniversal[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  // uint8_t r = 0xff, g = 0xff, b = 0xff;

  Serial.println("PN532   ==> Try to read ISO14443A tag");
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (!success) return 0xff;
  // Display some basic information about the card
  Serial.println("Found an ISO14443A card");
  Serial.print("  UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
  Serial.print("  UID Value : ");
  for (uint8_t i = 0; i < uidLength; i++) {
    Serial.print(uid[i], HEX);  Serial.print(' ');
  }
  Serial.print("\n--------------------------------------------------\n");

  uint8_t responseLength;

  if (uidLength == 4 && uid[0] == 1 && uid[1] == 2 && uid[2] == 3 && uid[3] == 4) {
    uint8_t selectApdu[] = { 0x00, /* CLA */
                             0xA4, /* INS */
                             0x04, /* P1  */
                             0x00, /* P2  */
                             0x07, /* Length of AID  */
                             0xF0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, /* AID defined on Android App */
                             0x00  /* Le  */
                           };
    uint8_t data[] = {0x63, 0x6f, 0x6d, 0x2e, 0x74, 0x72, 0x61, 0x64,
                      0x65, 0x76, 0x61, 0x6e, 0x2e, 0x74, 0x74, 0x34,
                      0x39, 0x2e, 0x77, 0x6f, 0x72, 0x6b, 0x64, 0x65,
                      0x6d, 0x6f
                     };
    uint8_t datanNdef[] = {0xd4, 0x0f, 0x1a, 0x61, 0x6e, 0x64, 0x72, 0x6f,
                           0x69, 0x64, 0x2e, 0x63, 0x6f, 0x6d, 0x3a, 0x70,
                           0x6b, 0x67, 0x63, 0x6f, 0x6d, 0x2e, 0x74, 0x72,
                           0x61, 0x64, 0x65, 0x76, 0x61, 0x6e, 0x2e, 0x74,
                           0x74, 0x34, 0x39, 0x2e, 0x77, 0x6f, 0x72, 0x6b,
                           0x64, 0x65, 0x6d, 0x6f
                          };

    Serial.println("This tag maybe ANDROID");
    responseLength = 200;
    success = nfc.inDataExchange(selectApdu, sizeof(selectApdu), message, &responseLength);
    // responseLength = 200;
    // success = nfc.inDataExchange(datanNdef, sizeof(datanNdef), message, &responseLength);
    Serial.print("Data exchange len="); Serial.print(responseLength); Serial.print(", success= "); Serial.println(success);
    if (success) {
      Serial.print("responseLength: "); Serial.println(responseLength);
      nfc.PrintHexChar(message, responseLength);
      do {
        uint8_t apdu[] = "Hello from Arduino";
        uint8_t back[32];
        uint8_t length = 32;
        success = nfc.inDataExchange(apdu, sizeof(apdu), back, &length);
        if (success) {
          Serial.print("responseLength: "); Serial.println(length);
          nfc.PrintHexChar(back, length);
        }  else {
          Serial.println("Broken connection?");
        }
      } while (success);
    }  else {
      Serial.println("Failed sending SELECT AID");
    }
  }  else if (uidLength == 4)  {
    // We probably have a Mifare Classic card ...
    Serial.println("Seems to be a Mifare Classic card (4 byte UID)");

    //Block 4  00 00 03 0A D1 01 06 54 02 65 6E E8 B0 B7 FE 00  ....炷.T.en靚溼�.
    // Now we try to go through all 16 sectors (each having 4 blocks)
    // authenticating each sector, and then dumping the blocks
    for (currentblock = 4; currentblock < 5; currentblock++)    {
      // Check if this is a new block so that we can reauthenticate
      if (nfc.mifareclassic_IsFirstBlock(currentblock)) authenticated = false;

      // If the sector hasn't been authenticated, do so first
      if (!authenticated)      {
        // Starting of a new sector ... try to to authenticate
        Serial.print("------------------------Sector "); Serial.print(currentblock / 4, DEC); Serial.println("-------------------------");
        if (currentblock == 0)        {
          // This will be 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF for Mifare Classic (non-NDEF!)
          // or 0xA0 0xA1 0xA2 0xA3 0xA4 0xA5 for NDEF formatted cards using key a,
          // but keyb should be the same for both (0xFF 0xFF 0xFF 0xFF 0xFF 0xFF)
          success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keyuniversal);
        }        else        {
          // This will be 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF for Mifare Classic (non-NDEF!)
          // or 0xD3 0xF7 0xD3 0xF7 0xD3 0xF7 for NDEF formatted cards using key a,
          // but keyb should be the same for both (0xFF 0xFF 0xFF 0xFF 0xFF 0xFF)
          success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keyuniversal);
        }
        if (success)        {
          authenticated = true;
        } else {
          Serial.print("Block "); Serial.print(currentblock, DEC); Serial.println(" unable to authenticate");
        }
      }
      // If we're still not authenticated just skip the block
      if (authenticated)      {
        // Authenticated ... we should be able to read the block now
        // Dump the data into the 'data' array
        data = message + (currentblock * 16);
        success = nfc.mifareclassic_ReadDataBlock(currentblock, data);
        if (success)    {
          // Read successful
          Serial.print("Block "); Serial.print(currentblock, DEC);
          if (currentblock < 10)   {
            Serial.print("  ");
          }     else         {
            Serial.print(" ");
          }
          // Dump the raw data
          nfc.PrintHexChar(data, 16);
        } else {
          // Oops ... something happened
          Serial.print("Block "); Serial.print(currentblock, DEC);
          Serial.println(" unable to read this block");

        }
      }
    }

    if (success) {
      *pn532HexBuffer = 0;
      // uid length
      byteToHex(uidLength, pn532HexBuffer);
      for (uint8_t t = 0; t <= uidLength; t++) {
        // uid address
        byteToHex(uid[t], pn532HexBuffer + (t + 1 ) * 2);
      }
      byteToHex(message[0x4b], pn532HexBuffer + (uidLength + 1 ) * 2);
      byteToHex(message[0x4c], pn532HexBuffer + (uidLength + 2 ) * 2);
      byteToHex(message[0x4d], pn532HexBuffer + (uidLength + 3 ) * 2);
      pn532HexBuffer[(uidLength + 4 ) * 2] = 0;
      Serial.print("\nPN532FNC Present:"); Serial.print(uidLength); Serial.print("; ");
      Serial.println((char *)pn532HexBuffer);
#if defined(TASK_MAX7912) && (TASK_MAX7912!=0)
      if (tidMax7912) {
        loadFont16x16(pn532HexBuffer[6], pn532HexBuffer[7], pn532HexBuffer[8], pn532HexBuffer[9]);
        os_signal_set(tidMax7912, EVT_PN532_CHANGED );
      }
#endif
#if defined(TASK_RESTCLIENT) && (TASK_RESTCLIENT!=0)
      if (tidRestClient) {
        os_signal_set(tidRestClient, EVT_PN532_CHANGED );
      }
#endif
    }
  }  else  {
    Serial.println("Ooops ... this doesn't seem to be a Mifare Classic card!");
  }
}

void taskPN532NFC(const void *arg)
{
  static os_event_t evt;

  do {
    if (tidPN532NFC != 0 && delayPN532NFC != 0)
      evt = os_signal_wait(EVT_WAIT_ALLEVENT, delayPN532NFC);

    jobPN532NFC();
  } while (tidPN532NFC != 0);
}




