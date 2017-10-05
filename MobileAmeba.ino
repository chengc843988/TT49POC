#include <cmsis_os.h>
#include <PinNames.h>
#include <NfcTag.h>
#include <Arduino.h>
#include <Wire.h>

#define TASK_MAX7912 0
#define TASK_MQTTCLIENT 0
#define TASK_RESTCLIENT 0
#define TASK_PN532NFC 1
#define TASK_BLUETOOTH 0
#define TASK_AMEBANFC 1
#define TASK_SERIAL_GATEWAY 1

#define DELAY_RS232_WAIT 10
/////////////////////
// multi task
uint32_t now;

extern uint8_t blueBuffer[];


uint32_t tidAmebaNFC = 0;
uint32_t tidPN532NFC = 0;
uint32_t tidBlue = 0;
uint32_t tidSerialGateway = 0;

uint32_t delayPN532NFC = 3000;
uint32_t delayAmebaNFC = 30000;
uint32_t delayBlue = 30000;
uint32_t delaySerialGateway = 100;

#define EVT_WAIT_ALLEVENT 0x0
#define EVT_TIMER_NOW 0x01
#define EVT_REST_CHANGED 0x02
#define EVT_MQTT_CHANGED 0x04
#define EVT_PN532_CHANGED 0x08
#define EVT_BLUE_CHANGED 0x10
#define EVT_AMEBANFC_CHANGED 0x20
#define EVT_AMEBANFC_TAGWRITE 0x40
#define EVT_KEYPAD_CHANGED 0x80
#define EVT_INFO_CHANGED 0x100

// ############################################################################
// uint8_t httpRequest[1024];
// uint8_t bitmapBuffer[40];
uint8_t amebaNfcHexBuffer[1024];
uint8_t pn532HexBuffer[1024];
uint8_t blueHexBuffer[1024];
// uint8_t keypadHexBuffer[1024];


/////////////////////
// utilities...
const char HEX_CHARS[] = "0123456789ABCDEF";
char *byteToHex(uint8_t b, uint8_t *p)
{
  static unsigned char buf[3];
  *p++ = buf[0] = HEX_CHARS[b >> 0x04];
  *p++ = buf[1] = HEX_CHARS[b &  0x0f];
  *p++ = buf[2] = 0;
  // buf[0] = b >> 0x4; buf[0] += (buf[0] <= '9') ? '0' : ('A' - 10);
  // buf[1] = b & 0x0f; buf[1] += (buf[1] <= '9') ? '0' : ('A' - 10);
  return (char *)buf;
}

uint8_t hexToByte(uint8_t hi, uint8_t lo)
{
  if (hi <= '9') hi -= '0';
  else if (hi <= 'Z') hi -= ('A' - 10);
  else hi -= ('a' - 10);

  if (lo <= '9') lo -= '0';
  else if (lo <= 'Z') lo -= ('A' - 10);
  else lo -= ('a' - 10);

  // Serial.print(hi); Serial.print(lo);
  uint8_t v = (hi << 4) + lo;
  return v;
}

byte Bit_Reverse( byte x )
{
  //          01010101  |         10101010
  x = ((x >> 1) & 0x55) | ((x << 1) & 0xaa);
  //          00110011  |         11001100
  x = ((x >> 2) & 0x33) | ((x << 2) & 0xcc);
  //          00001111  |         11110000
  x = ((x >> 4) & 0x0f) | ((x << 4) & 0xf0);
  return x;
}


/////////// AMEBA NFC

char *nfcApps[] = {"com.tradevan.tt49.mobileoaapp"};
char nfcUri[128]; // = "http://192.168.11.20:8080/BEACON/index.html";
char *nfcApp;
uint32_t last_update_time;
const struct NDEF *pNdefMsg;

void initAmebaNFC()
{
  nfcApp = nfcApps[0];
  strcpy(nfcUri, nfcApp);
  // NfcTag.appendRtdUri(nfcUri);
  // NfcTag.appendAndroidPlayApp(nfcApp);
  // sprintf(nfcUri, ADPAGE_FMT, wifiServ[wifiIndex], 0);
  // NfcTag.appendRtdUri(nfcUri);
  NfcTag.appendAndroidPlayApp(nfcApp);
  NfcTag.convertNdefToRaw();
  NfcTag.updateRawToCache();
  NfcTag.begin();

  last_update_time = NfcTag.getLastUpdateTimestamp();
}

void taskAmebaNFC(const void *arg)
{
  static os_event_t evt;
  int c = 0;
  int len;
  do {
    // NfcTag.clearNdefMessage();
    // NfcTag.appendRtdUri("http://192.168.11.20/BEACON/index.html");
    // NfcTag.convertNdefToRaw();
    // NfcTag.updateRawToCache();

    if (tidAmebaNFC != 0 && delayAmebaNFC != 0)
      evt = os_signal_wait(0, delayAmebaNFC);
    now = millis();
    Serial.print("Ameba NFC test="); Serial.print(NfcTag.getLastUpdateTimestamp() );
    Serial.print("/"); Serial.println(last_update_time);

    if (NfcTag.getLastUpdateTimestamp() <= last_update_time) {
      // sprintf(nfcUri, ADPAGE_FMT, wifiServ[wifiIndex], now);
      Serial.print("Update URL message:");  Serial.println(nfcUri);
      NfcTag.clearNdefMessage();
      // NfcTag.appendAndroidPlayApp(nfcApp);
      // NfcTag.appendRtdText(nfcUri);

      NfcTag.appendAndroidPlayApp(nfcApp);
      NfcTag.convertNdefToRaw();
      NfcTag.updateRawToCache();
      delay(100);
      last_update_time = NfcTag.getLastUpdateTimestamp() ;
      continue;
    }

    if (NfcTag.getLastUpdateTimestamp() > last_update_time)  {
      last_update_time = NfcTag.getLastUpdateTimestamp() ;
      pNdefMsg = NfcTag.getNdefData();
      len = pNdefMsg[0].payload_len;

      Serial.print("Get net nfc message: ");
      Serial.print("; size = "); Serial.print(NfcTag.getNdefSize());
      Serial.print("; length = "); Serial.println(len);
      {
        amebaNfcHexBuffer[0] = 0;
        for (int x = 0; x < len; x++) {
          byteToHex(pNdefMsg[0].payload[x], amebaNfcHexBuffer + (x * 2));
          // strcat((char *)amebaNfcHexBuffer, byteToHex(pNdefMsg[0].payload[x]));
        }
        amebaNfcHexBuffer[len * 2] = 0;
        Serial.println((char *)amebaNfcHexBuffer);
      }
      dumpTagContent();

      if (strncmp("android", (char *)pNdefMsg[0].payload_type, 7) == 0) {
        strcpy((char *)nfcUri, (char *)pNdefMsg[0].payload);
        nfcApp = nfcUri;
      }
#if defined(TASK_MAX7912) && (TASK_MAX7912!=0)
      if (tidMax7912) {
        loadFont16x16(amebaNfcHexBuffer[6], amebaNfcHexBuffer[7], amebaNfcHexBuffer[8], amebaNfcHexBuffer[9]);
        os_signal_set(tidMax7912, EVT_AMEBANFC_TAGWRITE );
      }
#endif

#if defined(TASK_RESTCLIENT)  && (TASK_RESTCLIENT!=0)
      if (tidRestClient) {
        os_signal_set(tidRestClient, EVT_AMEBANFC_TAGWRITE );
      }
#endif

#if defined(TASK_MQTTCLIENT)  && (TASK_MQTTCLIENT!=0)
      // if (tidMqttClient != 0) {
      //   os_signal_set(tidMqttClient, EVT_AMEBANFC_TAGWRITE);
      // }
#endif
    }
  } while (tidAmebaNFC != 0);
}

void dumpTagContent() {
  pNdefMsg = NfcTag.getNdefData();
  for (int i = 0; i < NfcTag.getNdefSize(); i++)
  {
    // print the ndef data
    Serial.print("NDEF message ");
    Serial.println(i);

    Serial.print("\tMB: ");
    Serial.print( (pNdefMsg[i].TNF_flag & TNF_MESSAGE_BEGIN) ? 1 : 0);
    Serial.print("\tME: ");
    Serial.print( (pNdefMsg[i].TNF_flag & TNF_MESSAGE_END) ? 1 : 0);
    Serial.print("\tCF: ");
    Serial.print( (pNdefMsg[i].TNF_flag & TNF_MESSAGE_CHUNK_FLAG) ? 1 : 0);
    Serial.print("\tSR: ");
    Serial.print( (pNdefMsg[i].TNF_flag & TNF_MESSAGE_SHORT_RECORD) ? 1 : 0);
    Serial.print("\tIL: ");
    Serial.print( (pNdefMsg[i].TNF_flag & TNF_MESSAGE_ID_LENGTH_IS_PRESENT) ? 1 : 0);
    Serial.println();

    Serial.print("\tTNF Type: ");
    switch ( pNdefMsg[i].TNF_flag & 0x07 ) {
      case TNF_EMPTY:         Serial.println("EMPTY");         break;
      case TNF_WELL_KNOWN:    Serial.println("WELL_KNOWN");    break;
      case TNF_MIME_MEDIA:    Serial.println("MIME_MEDIA");    break;
      case TNF_ABSOLUTE_URI:  Serial.println("ABSOLUTE_URI");  break;
      case TNF_EXTERNAL_TYPE: Serial.println("EXTERNAL_TYPE"); break;
      case TNF_UNCHANGED:     Serial.println("UNCHANGED");     break;
      case TNF_RESERVED:      Serial.println("RESERVED");      break;
      default:                Serial.println();                break;
    }

    Serial.print("\ttype length: ");
    Serial.println(pNdefMsg[i].type_len);
    Serial.print("\ttype: ");
    Serial.println((char *)(pNdefMsg[i].payload_type));

    Serial.print("\tpayload length: ");
    Serial.println(pNdefMsg[i].payload_len);
    Serial.print("\tpayload: ");
    Serial.println((char *)(pNdefMsg[i].payload));
  }
}

// modulized
#if defined(TASK_PN532NFC) && (TASK_PN532NFC!=0)
#include "PN532Util.h"
#endif

#if defined(TASK_BLUETOOTH) && (TASK_BLUETOOTH!=0)
#include "BluetoothUtil.h"
#endif
//////////////////////////////

// extern void proxyToBluetooth(Stream &port);
// extern void proxyToBluetooth(HardwareSerial &port) ;


#if defined(TASK_SERIAL_GATEWAY) && (TASK_SERIAL_GATEWAY != 0) && defined(TASK_BLUETOOTH) && (TASK_BLUETOOTH!=0)

void initSerialGateway() {}

void taskSerialGateway(const void *arg)
{
  static os_event_t evt;
  do {
    if (tidSerialGateway != 0 && delaySerialGateway != 0)
      evt = os_signal_wait(0, delaySerialGateway);
    now = millis();
    // Serial.print("check serial=");Serial.println(Serial.available());
    proxyToBluetooth(Serial);
  } while (tidSerialGateway != 0);
}

#endif
/////////////////////////

void setup() {

  Serial.begin(115200);
  Serial.println("-------Peer to Peer--------");



#if defined(TASK_BLUETOOTH) && (TASK_BLUETOOTH!=0)
  Serial.println("init Bluetuuth...");
  initBlue();

  //Serial.println("run Bluetuuth task..");
  tidBlue = os_thread_create(taskBlue, &tidBlue, OS_PRIORITY_LOW, 8192);
#endif


#if defined(TASK_PN532NFC) && (TASK_PN532NFC!=0)
  Serial.println("init PN532 NFC");
  initPN532NFC();
  Serial.println("run PN532 NFC...");
  tidPN532NFC = os_thread_create(taskPN532NFC, &tidPN532NFC, OS_PRIORITY_HIGH, 16384);
#endif


  Serial.println("init ameba NFC");
  initAmebaNFC();
  Serial.println("run ameba NFC...");
  tidAmebaNFC = os_thread_create(taskAmebaNFC, &tidAmebaNFC, OS_PRIORITY_NORMAL, 4096);


#if defined(TASK_SERIAL_GATEWAY) && (TASK_SERIAL_GATEWAY!=0) && defined(TASK_BLUETOOTH) && (TASK_BLUETOOTH!=0)
  Serial.println("init serial gateway...");
  initSerialGateway();

  //Serial.println("run Bluetuuth task..");
  tidSerialGateway = os_thread_create(taskSerialGateway, &tidSerialGateway, OS_PRIORITY_LOW, 4096);
#endif


}

void loop() {
  // put your main code here, to run repeatedly:

  now = millis();
  Serial.print("\nRun main thread...., then sleep: "); Serial.println(now);
  delay(3000);



#if defined(TASK_PN532NFC) && (TASK_PN532NFC!=0)
  if (tidPN532NFC == 0) {
    Serial.println("run PN532 NFC...");
    taskPN532NFC(NULL);
  }
#endif

#if defined(TASK_AMEBANFC) && (TASK_AMEBANFC != 0)
  if (tidAmebaNFC == 0) {
    Serial.println("run Ameba NFC...");
    taskAmebaNFC(NULL);
  }
#endif

#if defined(TASK_BLUETOOTH) && (TASK_BLUETOOTH!=0)
  if (tidBlue == 0) {
    Serial.println("run Bluetooth BLE4.0...");
    taskBlue(NULL);
  }
#endif

#if defined(TASK_SERIAL_GATEWAY) && (TASK_SERIAL_GATEWAY != 0) && defined(TASK_BLUETOOTH) && (TASK_BLUETOOTH!=0)
  if (tidSerialGateway == 0) {
    Serial.println("run tidSerialGateway...");
    taskSerialGateway(NULL);
  }
#endif

}




