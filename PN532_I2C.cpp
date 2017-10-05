
#include "PN532_I2C.h"
#include "PN532_debug.h"
#include "Arduino.h"
#include <Wire.h>
#include <SPI.h>

#define PN532_I2C_ADDRESS       (0x48 >> 1)

#define WIRE Wire1

PN532_I2C::PN532_I2C(TwoWire &wire)
{
  _wire = &wire;
  command = 0;
  /*
    if(_wire == &Wire1) {
    Serial.println("*** using WIRE 1 ***");
    } else if(_wire == &Wire) {
    Serial.println("*** using WIRE 0 ***");
    }

    if(&WIRE == &Wire1) {
    Serial.println("*** using WIRE 1 ***");
    } else if(&WIRE == &Wire) {
    Serial.println("*** using WIRE 0 ***");
    }
  */
}

void PN532_I2C::begin()
{
  WIRE.begin();
  delay(100);

  digitalWrite(12, LOW);
  delay(100);
  digitalWrite(12, HIGH);
  delay(100);
  digitalWrite(12, LOW);
}

void PN532_I2C::wakeup()
{
  WIRE.beginTransmission(PN532_I2C_ADDRESS); // I2C start
  delay(400);
  WIRE.endTransmission();                    // I2C end
}

int8_t PN532_I2C::writeCommand(const uint8_t *header, uint8_t hlen, const uint8_t *body, uint8_t blen)
{
  uint8_t checksum = 0xd4;
  command = header[0];

  Serial.print("\nsend out:");
  WIRE.beginTransmission(0x24); // transmit to device #8
  WIRE.write((uint8_t)0x00);
  WIRE.write((uint8_t)0x00);
  WIRE.write((uint8_t)0xff);

  uint8_t totalLen = hlen + blen + 1;
  WIRE.write((uint8_t)(totalLen));
  WIRE.write((uint8_t)(~totalLen) + 1);
  WIRE.write((uint8_t)checksum);

  Serial.print(" 0x"); Serial.print((uint8_t)0x00, HEX);
  Serial.print(" 0x"); Serial.print((uint8_t)0x00, HEX);
  Serial.print(" 0x"); Serial.print((uint8_t)0xff, HEX);

  Serial.print(" 0x"); Serial.print((uint8_t)(totalLen), HEX);
  Serial.print(" 0x"); Serial.print((uint8_t)(~totalLen) + 1, HEX);
  Serial.print(" 0x"); Serial.print((uint8_t)checksum, HEX);

  for (int i = 0; i < hlen; i++) {
    WIRE.write((uint8_t)header[i]);
    checksum += header[i];
    Serial.print(" 0x"); Serial.print(header[i], HEX);
  }

  for (int i = 0; i < blen; i++) {
    WIRE.write((uint8_t)body[i]);
    checksum += header[i];
    Serial.print(" 0x"); Serial.print(body[i], HEX);
  }

  checksum = ~checksum + 1;
  WIRE.write((uint8_t)checksum);
  WIRE.write((uint8_t)0x00);
  Serial.print(" 0x"); Serial.print((uint8_t)(checksum), HEX);
  Serial.print(" 0x"); Serial.print((uint8_t)(0x00), HEX);

  WIRE.endTransmission();    // stop transmitting


  Serial.println(";  Write down");

  return readAckFrame();

}

int16_t PN532_I2C::readResponse(uint8_t buf[], uint8_t len, uint16_t timeout)
{
  static uint8_t header[7];
  uint16_t time = 0;
  uint8_t c;
  uint16_t p = 0;
  if (len > 250) len = 250;

  Serial.println("==> waitting for ready");
while(!digitalRead(12)) {delay(10); }

  Serial.print("\nRead Response, len="); Serial.println(len + 2);

  WIRE.requestFrom(PN532_I2C_ADDRESS, len + 2);
  if (WIRE.available()) {
    c=WIRE.read();
    Serial.print(" (0x"); Serial.print(c, HEX);Serial.print(")");
  }

  p=0;
  while (WIRE.available() && p<sizeof(header)) { // slave may send less than requested
    header[p++] = c = WIRE.read();
    Serial.print(" 0x"); Serial.print(c, HEX);
    delay(1);
  }
  if(p==sizeof(header)) len=header[3];
  p=0;
  while (WIRE.available() && p<len) { // slave may send less than requested
    buf[p++] = c = WIRE.read();
    Serial.print(" 0x"); Serial.print(c, HEX);
    delay(1);
  }

  Serial.print(", totalLen="); Serial.println(p);
  return p;
}

int8_t PN532_I2C::readAckFrame()
{
  const uint8_t PN532_ACK[] = {0, 0, 0xFF, 0, 0xFF, 0};
  uint8_t ackBuf[sizeof(PN532_ACK)];

  Serial.print("\r\nWait for ack at: "); Serial.println(millis());
  DMSG("wait for ack at : ");
  DMSG(millis());
  DMSG('\n');

  Serial.print(" [ 0x"); Serial.print(PN532_I2C_ADDRESS, HEX); Serial.print(" ] ");
  uint16_t time = 0;

  uint8_t i = 0;
  uint8_t ackCommand;
  WIRE.requestFrom(0x24, 7);    // request 6 bytes from slave device #8
  if (WIRE.available()) {
    ackCommand = WIRE.read();
    Serial.print(" (0x"); Serial.print(ackCommand, HEX);Serial.print(")");
  }
  while (WIRE.available()) { // slave may send less than requested
    ackBuf[i] = WIRE.read();
    Serial.print(" 0x"); Serial.print(ackBuf[i], HEX);       // print the character
    i++;
  }
  // TODO: if i <> 6 ?
  if (memcmp(ackBuf, PN532_ACK, sizeof(PN532_ACK))) {
    Serial.println(" result: not same");
    return PN532_INVALID_ACK;
  }
  Serial.println(" result: OK");

  return 0;
}
