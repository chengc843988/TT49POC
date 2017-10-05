

#ifndef DELAY_RS232_WAIT
#define DELAY_RS232_WAIT 10
#endif
#ifndef TASK_MAX7912
#define TASK_MAX7912 0
#endif

#ifndef TASK_RESTCLIENT
#define TASK_RESTCLIENT 0
#endif
#ifndef EVT_WAIT_ALLEVENT
#define EVT_WAIT_ALLEVENT 0x0
#endif

#include <SoftwareSerial.h>

uint8_t blueBuffer[128];


//////////////////////////////////
// SoftwareSerial blueSerial(0, 1); // RX, TX
SoftwareSerial blueSerial(8, 9); // RX, TX
#define PIN_BLUE_ENABLE 11
#define PIN_BLUE_STATE 10
bool blueState = false;

uint8_t blueLADDR[14];
uint8_t blueRADDR[14];
// uint8_t blueCommand[128];

enum BLUE_DEVICE_NAME_TYPE {
  HM10, CC41A, UNKNOWN_DEVICE
} blueDeviceName;

const uint32_t BAUD_RATE[] = {
  9600, 57600, 115200, 19200, 38400, 4800, 2400, 1200, 230400, 0
};


uint32_t blueAutoBaudrate()
{
  digitalWrite(PIN_BLUE_ENABLE, false);
  long cur_baud = 9600;
  byte c1, c2;
  // Serial.setBufferSize(512);

  Serial.print("AutoBaud Detecting:");
  if (blueDeviceName == HM10) Serial.println("HM10");
  else if (blueDeviceName == CC41A) Serial.println("CC41A");
  else Serial.println("UNKNOWN");

  if (blueSerial.isListening()) return 9600;
  blueSerial.flush();

  cur_baud = BAUD_RATE[0];
  for (const uint32_t *p = BAUD_RATE; cur_baud != 0; cur_baud = *++p) {
    Serial.print("Auto Baudrate Checking: ");  Serial.println(cur_baud, DEC);
    if (blueSerial.isListening()) {
      blueSerial.end();
      delay(5);
    }
    blueSerial.begin(cur_baud);
    // blueSerial.flush();
    delay(5);
    while (blueSerial.available() > 0) blueSerial.read();

    blueSerial.print("AT");
    switch (blueDeviceName) {
      case HM10:
        // delay(10);
        // blueSerial.print("AT");
        break;
      case CC41A:
      default:
        blueSerial.println();
        break;
    }

    delay(25);
    if (blueSerial.available() >= 1) {
      c1 = blueSerial.read();
      if (blueSerial.available() == 0) delay(10);
    }
    if (blueSerial.available() >= 1) {
      c2 = blueSerial.read();
    }
    if (c1 == 'O' && c2 == 'K') {
      Serial.print("Detected ");
      Serial.println(cur_baud, DEC);
      return cur_baud;
    } else {
      delay(10);
      cur_baud = 9600;
    }
  }
  if (cur_baud == 0)cur_baud = BAUD_RATE[0];
  if (blueSerial.isListening()) {
    blueSerial.end();
    delay(5);
  }
  blueSerial.begin(cur_baud);
  return 0;
}


void blueCheckDeviceType()
{
  delay(30);
  Serial.println("\nCheck Device Type: CC41A");
  blueDeviceName = CC41A;

  if (blueAutoBaudrate()) return;

  Serial.println("\nCheck Device Type: HM10");
  blueDeviceName = HM10;
  if (blueAutoBaudrate()) return;
  Serial.println("DEVICE is UNKNOWN_DEVICE");
  blueDeviceName = UNKNOWN_DEVICE;
}


void mapRemoteAddr(uint8_t *buf, uint8_t *mac)
{
  uint8_t *addr = mac;
  if (blueDeviceName == HM10)
    buf += 8;
  else
    buf += 9;

  *addr++ = *buf++;    *addr++ = *buf++;
  *addr++ = *buf++;    *addr++ = *buf++;
  *addr++ = *buf++;    *addr++ = *buf++;
  *addr++ = *buf++;    *addr++ = *buf++;
  *addr++ = *buf++;    *addr++ = *buf++;
  *addr++ = *buf++;    *addr++ = *buf++;
  *addr++ = 0;
  Serial.print("Read mac From remote address: "); Serial.println((char *)mac);
}

void mapAddrToMac(uint8_t *buf, uint8_t *mac)
{
  uint8_t *laddr = mac;
  if (blueDeviceName == HM10)
    buf += 8;
  else
    buf += 6;

  for (int8_t i = 0; i < 6; i++) {
    *laddr++ = *buf++; *laddr++ = *buf++;
    if (blueDeviceName != HM10)
      buf++;
  }
  *laddr++ = 0;
  // laddr=mac+6;
  // if (memcmp(("00682A"), mac + 6, 6) == 0) memset(mac + 6, '0', 6);
  Serial.print("Read mac address:"); Serial.println((char *)mac);
}


int readLine(Stream & from, uint8_t *chBuffer, int maxLen, uint32_t millisTimeout = 1000, uint32_t delayTime = DELAY_RS232_WAIT)
{
  int c = 0;
  int ch;
  uint32_t timeout;
  timeout = millis() + millisTimeout + delayTime;
  do {
    delay(delayTime);
    while (from.available() > 0) {
      ch = from.read() & 0xff;
      // Serial.print(ch,HEX);Serial.print(', ');
      // if (ch < 0) continue;
      if (ch >= 0x1f) {
        chBuffer[c++] = ch;
        chBuffer[c] = 0;
      } else {
        if (c > 0) {
          goto bleReturn;
        }
      }
    }
  } while (millis() < timeout);
bleReturn:
  chBuffer[c] = 0;
  if (c > 0) {
    Serial.print("BLE INPUT: ");  Serial.println((char *)chBuffer);
    delay(delayTime);
  }
  return c;
}

static uint8_t blueOK[16];

void blueExecuteReset(Stream & out, char *cmd, uint8_t *resultBuffer, int bufLen)
{
  *resultBuffer = 0;
  out.print(cmd);
  if (blueDeviceName != HM10)  out.println();
}

void blueExecuteCommand(Stream & out, char *cmd1, char *cmd2, uint8_t *resultBuffer, int bufLen)
{
  *resultBuffer = 0;
  Serial.print("Output BLE command:");
  if (cmd1) Serial.print(cmd1);
  if (cmd2) Serial.print(cmd2);
  Serial.println();

  out.flush();

  if (cmd1)
    out.print(cmd1);
  if (cmd2)
    out.print(cmd2);
  delay(50);
  switch (blueDeviceName) {
    case HM10:
      readLine(out, resultBuffer, bufLen);
      break;
    case CC41A:
    default:
      out.println();
      readLine(out, resultBuffer, bufLen);
      readLine(out, blueOK, sizeof(blueOK));
      break;
  }
}


void blueExecuteCommand(Stream & out, char *cmd, uint8_t *resultBuffer, int bufLen) {
  blueExecuteCommand(out, cmd, (char *)NULL, resultBuffer, bufLen);
}

void blueExecuteQuery(Stream & out, char *cmd, uint8_t *resultBuffer, int bufLen)
{
  Serial.print("BLUE ExecuteQuery:");
  if (cmd) Serial.print(cmd);
  Serial.println();

  if (cmd) out.print(cmd);
  *resultBuffer = 0;
  switch (blueDeviceName) {
    case HM10:
      out.write(' ? ');
      readLine(out, resultBuffer, bufLen);
      break;
    case CC41A:
    default:
      out.println();
      readLine(out, resultBuffer, bufLen);
      break;
  }
  // Serial.print("query receive:"); Serial.println((char *)resultBuffer);
}

bool PROXY_SERIAL(Stream & from, Stream & to, uint32_t delayTime = DELAY_RS232_WAIT)
{
  uint16_t c;
again:
  c = 0;
  while (from.available() > 0) {
    c = from.read();
    to.write(c);
    // Serial.write(c);
  }
  if (c > 0) {
    delay(delayTime);
    goto again;
  }
}

void initBlue()
{
  blueSerial.begin(9600);

  pinMode(PIN_BLUE_ENABLE, OUTPUT);
  pinMode(PIN_BLUE_STATE, INPUT);
  {
    digitalWrite(PIN_BLUE_ENABLE, LOW);
    digitalWrite(PIN_BLUE_STATE, LOW);
    // pinMode(BLE_STATE,INPUT);
    // pinMode(BLE_STATE, INPUT_IRQ_RISE);
    //digitalSetIrqHandler(BLE_STATE, bleStatusChange);
    // // attachInterrupt(digitalPinToInterrupt(BLE_STATE),bleStatusChange,RISING);
    // delayMicroseconds(100);
    // digitalWrite(BLE_ENABLE, HIGH);
  }


  // blueDeviceName = HM10;
  Serial.println("init setup BLUE ...");
  blueCheckDeviceType();
  // blueAutoBaudrate();

  Serial.println("Reset device");
  blueExecuteReset(blueSerial, "AT+RENEW", blueBuffer, sizeof(blueBuffer));
  delay(100);
  blueAutoBaudrate();

  blueSerial.flush();
  while (blueSerial.available() > 0) blueSerial.read();

  // 1:1200, 2400, 4800, *9600, 19200,...
  // blueSerial.println("AT+BAUD5");
  // blueExecuteCommand(blueBuffer);
  // blueAutoBaudrate();

  if (1 == 1) {
    Serial.println("Get Local MAC Address:");
    // run twice, check it running
    blueExecuteQuery(blueSerial, "AT+ADDR", blueBuffer, sizeof(blueBuffer));
    mapAddrToMac(blueBuffer, blueLADDR);
  }

  if (1 == 1) {
    // set device as MAC
    Serial.println("Setup DEVICE Name:");
    blueExecuteCommand(blueSerial, "AT+NAMETT49-", (char *)(blueLADDR + 8),  blueBuffer, sizeof(blueBuffer));
    // set pin code to default; this not a BLUEready device , so the PIN is inuse;
    blueExecuteCommand(blueSerial, "AT+PASS", "111111",  blueBuffer, sizeof(blueBuffer));
    delay(200);
    blueExecuteQuery(blueSerial, "AT", blueBuffer, sizeof(blueBuffer));
    delay(10);
    blueExecuteQuery(blueSerial, "AT", blueBuffer, sizeof(blueBuffer));
  }

  if (1 == 1) {
    Serial.println("Setup Major & minor address:");
    blueExecuteCommand(blueSerial, "AT+MARJ0x", "0AAA",  blueBuffer, sizeof(blueBuffer));

    // set this minor number
    if (memcmp(("00682A"), blueLADDR + 6, 6) == 0) {
    }
    blueExecuteCommand(blueSerial, "AT+MINO0x", (char *)blueLADDR + 8,  blueBuffer, sizeof(blueBuffer));

  }

  if (1 == 1) {
    // MD5(TRADEVAN)=e64e672aed8625a7e21781dff4851bd9
    // Serial.println("Start config uuid");
    // iBeacon UUID is: 74278BDA-B644-4520-8F0C-720EAF059935
    // 0000b81d-0000-1000-8000-00805f9b34fb
    blueExecuteCommand(blueSerial, "AT+IBE00000B81D",  blueBuffer, sizeof(blueBuffer));
    blueExecuteCommand(blueSerial, "AT+IBE100001000",  blueBuffer, sizeof(blueBuffer));
    blueExecuteCommand(blueSerial, "AT+IBE280000080",  blueBuffer, sizeof(blueBuffer));
    blueExecuteCommand(blueSerial, "AT+IBE30080",  blueBuffer, sizeof(blueBuffer));
  }
  {
    blueExecuteQuery(blueSerial, "AT+IBE0",  blueBuffer, sizeof(blueBuffer));
    blueExecuteQuery(blueSerial, "AT+IBE1",  blueBuffer, sizeof(blueBuffer));
    blueExecuteQuery(blueSerial, "AT+IBE3",  blueBuffer, sizeof(blueBuffer));
  }

  if (1 == 1) {
    // 0:-23dbm, -6, *2:0, 6
    blueExecuteCommand(blueSerial, "AT+POWE", "2",  blueBuffer, sizeof(blueBuffer));
    // 0:sleep mode, *1: non_sleep mode
    blueExecuteCommand(blueSerial, "AT+PWRM", "1",  blueBuffer, sizeof(blueBuffer));

    // default ADVI0, *0:100ms, 1525,211,318,17,546,768,852,1022,9:1285ms,A:2000,..7000
    blueExecuteCommand(blueSerial, "AT+ADVI", "8",  blueBuffer, sizeof(blueBuffer));

    //0:no need pin, 1:auth not need pin, 2:with pin, 3:and bond
    blueExecuteCommand(blueSerial, "AT+TYPE", "0",  blueBuffer, sizeof(blueBuffer));
  }

  if (1 == 1) {
    Serial.println("Setup Notify type:");
    blueExecuteCommand(blueSerial, "AT+NOTI", "1",  blueBuffer, sizeof(blueBuffer));
    blueExecuteCommand(blueSerial, "AT+NOTP", "1",  blueBuffer, sizeof(blueBuffer));
  }

  if (1 == 1 && blueDeviceName == HM10)  {
    blueExecuteCommand(blueSerial, "AT+IBEA", "1",  blueBuffer, sizeof(blueBuffer));
  }

  if (1 == 1) {
    blueExecuteQuery(blueSerial, "AT+NAME", blueBuffer, sizeof(blueBuffer));
    blueExecuteQuery(blueSerial, "AT+MARJ", blueBuffer, sizeof(blueBuffer));
    blueExecuteQuery(blueSerial, "AT+MINO", blueBuffer, sizeof(blueBuffer));
    // blueSerial.print("AT+IMME0");
    // blueExecuteExecute(blueBuffer);
    // blueExecuteQuery(blueSerial,"AT+IMME0", blueBuffer, sizeof(blueBuffer));

    blueExecuteReset(blueSerial, "AT+RESET", blueBuffer, sizeof(blueBuffer));
    delay(100);
    blueAutoBaudrate();
    delay(100);
    blueSerial.flush();
    while (blueSerial.available() > 0) blueSerial.read();
  }
  digitalWrite(PIN_BLUE_ENABLE, HIGH);
  Serial.println("bluetooth init done...");
}

void processBluetoothInput()
{
  Serial.print("processBluetoothInput:" ); Serial.println(blueSerial.available() );
  static char ch;
  static char plusCount = 0;
  int len;
  *blueBuffer = 0;
  len = 0;
  while (blueSerial.available() > 0) {
    len += readLine(blueSerial, blueBuffer + len, sizeof(blueBuffer) - len);
  }

  if (blueBuffer[0] == '+' && blueBuffer[1] == '+' && blueBuffer[2] == '+') {
    Serial.println("peer stop connect command:+++");
    digitalWrite(PIN_BLUE_ENABLE, LOW);
    delay(300);
    *blueBuffer = 0;
  }
#if defined(TASK_MAX7912) && (MAX7912!=0)
  if (len > 0 && tidMax7912 != 0) {
    loadFont16x16(blueBuffer[0], blueBuffer[1], blueBuffer[2], blueBuffer[3]);
    os_signal_set(tidMax7912, EVT_BLUE_CHANGED );
  }
#endif
#if defined(TASK_MAX7912) && (MAX7912!=0)
  if (len > 0 && tidMax7912 != 0) {
    os_signal_set(tidRestClient, EVT_BLUE_CHANGED);
  }
#endif
}


void taskBlue(const void *arg)
{
  static os_event_t evt;
  static bool conn = false;

  do {
    if (tidBlue != 0 && delayBlue != 0)
      evt = os_signal_wait(EVT_WAIT_ALLEVENT, delayBlue);
    now = millis();

    // if (tidRestClient!=0) {
    //   os_signal_set(tidRestClient, EVT_BLE_CHANGED);
    // }

    digitalWrite(PIN_BLUE_STATE, LOW); // delay(3);
    blueState = digitalRead(PIN_BLUE_STATE);
    Serial.print("Check blue status:"); Serial.print(conn); Serial.print("/"); Serial.println(blueState);
    if (!conn && blueState) {
      digitalWrite(PIN_BLUE_ENABLE, HIGH);
      // blueExecuteQuery(blueSerial, "AT+RADDR", blueBuffer, sizeof(blueBuffer));
      readLine(blueSerial, blueBuffer, sizeof(blueBuffer));
      if (blueBuffer[3] == 'L' && blueBuffer[4] == 'O')  readLine(blueSerial, blueBuffer, sizeof(blueBuffer));
      mapRemoteAddr(blueBuffer, blueRADDR);
      Serial.print("\nBluetooth connection from:[");  Serial.print((char *)blueBuffer); Serial.print("] ==> "); Serial.println((char *)blueRADDR);

      strcpy((char *)blueHexBuffer, (char *)blueRADDR);
#if defined(TASK_MAX8912) && (TASK_MAX8912!=0)
      if (tidMax7912) {
        loadFont16x16(blueRADDR[8], blueRADDR[9], blueRADDR[10], blueRADDR[11]);
        os_signal_set(tidMax7912, EVT_BLUE_CHANGED );
      }
#endif
#if defined(TASK_RESTCLIENT) && (TASK_RESTCLIENT!=0)
      if (tidRestClient != 0) {
        os_signal_set(tidRestClient, EVT_BLUE_CHANGED);
      }
#endif
    } else if (conn && !blueState) {
      Serial.print("\nDisconnect from: ");
      Serial.println((char *)blueRADDR);
    }
    conn = blueState ;
    if (conn) {
      processBluetoothInput();
    }

    //    if (!conn) {
    //      PROXY_SERIAL(Serial, blueSerial);
    //      PROXY_SERIAL(blueSerial, Serial);
    //      digitalWrite(PIN_BLUE_ENABLE, HIGH);
    //    }

  } while (tidBlue != 0);

}

void proxyToBluetooth(HardwareSerial &port)
{
  while (port.available() > 0) {
    blueSerial.write(port.read());
  }
  while (blueSerial.available() > 0) {
    port.write(blueSerial.read());
  }
}


