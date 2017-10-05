#ifndef NfcTagPn532_h
#define NfcTagPn532_h

#include <inttypes.h>
#include <Arduino.h>
#include "NdefMessage.h"

class NfcTagPn532
{
    public:
        NfcTagPn532();
        NfcTagPn532(byte *uid, unsigned int uidLength);
        NfcTagPn532(byte *uid, unsigned int uidLength, String tagType);
        NfcTagPn532(byte *uid, unsigned int uidLength, String tagType, NdefMessage& ndefMessage);
        NfcTagPn532(byte *uid, unsigned int uidLength, String tagType, const byte *ndefData, const int ndefDataLength);
        ~NfcTagPn532(void);
        NfcTagPn532& operator=(const NfcTagPn532& rhs);
        uint8_t getUidLength();
        void getUid(byte *uid, unsigned int uidLength);
        String getUidString();
        String getTagType();
        boolean hasNdefMessage();
        NdefMessage getNdefMessage();
        void print();
    private:
        byte *_uid;
        unsigned int _uidLength;
        String _tagType; // Mifare Classic, NFC Forum Type {1,2,3,4}, Unknown
        NdefMessage* _ndefMessage;
        // TODO capacity
        // TODO isFormatted
};

#endif