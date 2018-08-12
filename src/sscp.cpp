/*
  Library for Simple Serial Communication Protocol (SSCP)

  Copyright (c) 2018 Fabian Schmieder. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <Arduino.h>
#include <sscp.h>

// #define DEBUG 1
// DEBUG = 2 be verbose
// DEBUG = 3 very verbose
#ifdef DEBUG
#define Debug(str)                                                             \
  OnTransmissionBegin();                                                       \
  Serial.print(millis());                                                      \
  Serial.print(": ");                                                          \
  Serial.print(__func__);                                                      \
  Serial.print(' ');                                                           \
  Serial.print(__FILE__);                                                      \
  Serial.print(':');                                                           \
  Serial.print(__LINE__);                                                      \
  Serial.print(' ');                                                           \
  Serial.println(str);                                                         \
  Serial.flush();                                                              \
  OnTransmissionEnd();
#else              // ifdef DEBUG
#define Debug(str) //
#endif             // ifdef DEBUG

// Callback Functions

void sscp::SetOnTransmissionBeginHandler(TransmissionStateFunction func) {
  OnTransmissionBegin = func;
}

void sscp::SetOnTransmissionEndHandler(TransmissionStateFunction func) {
  OnTransmissionEnd = func;
}

// private rx methods

SSCP_PACKET_ERROR sscp::IsValidPacket(unsigned short int pos) {

  // STX LEN SRC DST TYPE CMD PAYLOAD CRC ETX
  // 02  05  01  02  01   10  10      CS  03

  // STX
  if (rxBuf[pos + POS_STX] != STX) {
    Debug("no STX");
    return SSCP_PACKET_ERROR_NOSTX;
  }

  // lenght
  int len =
      rxBuf[pos + POS_LEN] + 3; // STX + LEN + SRC + DST + TYPE + CMD + CS + ETX
  if (len < OVERHEAD_LENGTH || len > (MAXIMUM_PAYLOAD + OVERHEAD_LENGTH)) {
    Debug("lenght failure");
    return SSCP_PACKET_ERROR_LENGTH;
  }

  // packet type
  if ((rxBuf[pos + POS_TYPE] != SSCP_PACKET_TYPE_CMD) &
      (rxBuf[pos + POS_TYPE] != SSCP_PACKET_TYPE_ACK) &
      (rxBuf[pos + POS_TYPE] != SSCP_PACKET_TYPE_NAK) &
      (rxBuf[pos + POS_TYPE] != SSCP_PACKET_TYPE_NUL)) {
    Debug("wrong packet type");
    return SSCP_PACKET_ERROR_TYPE;
  }

  // ETX
  if ((pos + len - 1 < sizeof(rxBuf)) & (rxBuf[pos + len - 1] != ETX)) {
    Debug("no ETX");
    return SSCP_PACKET_ERROR_NOETX;
  }

  // checksum
  unsigned char cs = 0x7f;
  for (unsigned short int i = pos + 1; i < pos + len - 2; i++) {
    // Debug("CS: " + String(rxBuf[i], HEX));
    cs = cs + rxBuf[i];
  }
  cs = cs & 255;

  if (cs != rxBuf[pos + len - 2]) {
    Debug("Checksum wrong! Is " + String(rxBuf[pos + len - 2], HEX) +
          " should " + String(cs, HEX));
    return SSCP_PACKET_ERROR_CHECKSUM;
  }

  // then packet seems to be valid
  Debug("valid packet");
  return SSCP_PACKET_ERROR_NONE;
}

void sscp::rxBuf_lshift() {
  for (unsigned short int i = 0; i < sizeof(rxBuf) - 1; i++) {
    rxBuf[i] = rxBuf[i + 1];
  }
  rxBuf[sizeof(rxBuf) - 1] = 0x00;
  rxBuf_len = rxBuf_len - 1;

  // FIXME - overflow
  // Debug("len: " + String(rxBuf_len));
}

// public loop method
void sscp::loop() {
  // processing of incomming bytes
  if (stream.available()) {
    unsigned char rx = (unsigned char)stream.read();
#if DEBUG > 2
    Debug("read: " + String(rx, HEX));
#endif
    if (rxBuf_len == sizeof(rxBuf)) {
      rxBuf_lshift();
    }
    rxBuf[rxBuf_len] = rx;
    rxBuf_len++;

    if ((rxState == SYNCING) && (rx == STX))
      rxState = RECEIVING;
    else if ((rxState == RECEIVING) && (rx == ETX)) {
      // check for valid packet in buffer
      for (unsigned short int i = 0; i < sizeof(rxBuf) - OVERHEAD_LENGTH; i++) {
#if DEBUG
        if (rxBuf[i] == STX && IsValidPacket(i) == SSCP_PACKET_ERROR_NONE)
#else
        if (IsValidPacket(i) == SSCP_PACKET_ERROR_NONE)
#endif
        {
          // copy packet to rxPacket and delete packet in buffer
          short int k = (rxBuf[1] + 3); // FIXME: don't know why addition in
                                        // for-statement doesn't work
          for (short int n = 0; n < k; n++) {
            rxPacket[n] = rxBuf[i + n];
            rxBuf[i + n] = 0x00;
          }
#if DEBUG > 1
          String debug = "rxPacket: ";
          for (unsigned short int r = 0; r < sizeof(rxPacket); r++) {
            debug = debug + String(rxPacket[r], HEX) + " ";
          }
          Debug(debug);
#endif
        }
      }

      rxState = SYNCING;
    }

    // lshifting garabge
    while ((rxBuf_len > 0) & (rxBuf[0] != STX)) {
      rxBuf_lshift();
    }

#if DEBUG > 1
    if (rxBuf_len > 0) {
      String rxDebug = " | rxBuf: ";
      for (unsigned short int d = 0; d < sizeof(rxBuf); d++) {
        rxDebug = rxDebug + String(rxBuf[d], HEX) + " ";
      }

      Debug("len: " + String(rxBuf_len, DEC) + rxDebug);
    }
#endif

    if (rxBuf_len == 0) {
      rxState = SYNCING;
    }
  }
}

// rx functions
bool sscp::IsPacketAvailable(unsigned char node) {
  return (rxPacket[POS_STX] == STX &
          (rxPacket[POS_DST] == node | rxPacket[POS_DST] == 0x00));
}

unsigned char sscp::GetSource() { return rxPacket[POS_SRC]; }

unsigned char sscp::GetDestination() { return rxPacket[POS_DST]; }

unsigned char sscp::GetType() { return rxPacket[POS_TYPE]; }

unsigned char sscp::GetCommand() { return rxPacket[POS_CMD]; }

unsigned short int sscp::GetPayloadLength() {
  unsigned short int len = rxPacket[POS_LEN] - OVERHEAD_LENGTH + 2;
#if DEBUG > 2
  Debug("Payload length: " + String(len));
#endif
  return len;
}

unsigned char sscp::GetPayload(unsigned short int pos) {
  if (pos < GetPayloadLength()) {
#if DEBUG > 2
    Debug("Get real rxPacket: " + String(rxPacket[POS_CMD + 1 + pos], HEX));
#endif
    return rxPacket[POS_CMD + 1 + pos];
  } else {
#if DEBUG > 2
    Debug("Get error rxPacket: 0x00");
#endif
    return 0x00;
  }
}

void sscp::ReleasePacket() {
  for (unsigned short int i = 0; i < sizeof(rxPacket); i++)
    rxPacket[i] = 0x00;
}

// tx functions

void sscp::MessageBegin(unsigned char src, unsigned char dst,
                        SSCP_PACKET_TYPE type, unsigned char cmd) {
#ifdef DEBUG
  Debug("MessageBegin with src=" + String(src, HEX) +
        " dst=" + String(dst, HEX) + " type=" + String(type, HEX) +
        " cmd=" + String(cmd, HEX));
  // empty buffer
  for (unsigned short int i = 0; i < sizeof(txBuf); i++) {
    txBuf[i] = 0x00;
  }
#endif
  txBuf[0] = STX;
  txBuf[1] = OVERHEAD_LENGTH - 4;
  txBuf[2] = src;
  txBuf[3] = dst;
  txBuf[4] = type;
  txBuf[5] = cmd;
  txBuf_len = OVERHEAD_LENGTH - 1;

  // Debug
#if DEBUG > 1
  String txDebug = " | txBuf: ";
  for (unsigned short int d = 0; d < sizeof(txBuf); d++) {
    txDebug = txDebug + String(txBuf[d], HEX) + " ";
  }
  Debug("len: " + String(txBuf_len, DEC) + txDebug);
#endif
}

void sscp::MessageAdd(unsigned char c) {
  txBuf[1]++; // length
  txBuf[txBuf_len] = c;
  txBuf_len++;
#if DEBUG > 1
  String txDebug = " | txBuf: ";
  for (unsigned short int d = 0; d < sizeof(txBuf); d++) {
    txDebug = txDebug + String(txBuf[d], HEX) + " ";
  }
  Debug("len: " + String(txBuf_len, DEC) + txDebug);
#endif
}

void sscp::MessageSend() {
  txBuf[1]++; // length
  // checksum
  unsigned char cs = 0x7f;
  for (unsigned short int i = 1; i < txBuf_len; i++) {
    // Debug("txCS: " + String(txBuf[i], HEX));
    cs = cs + txBuf[i];
  }

  txBuf[txBuf_len] = (cs & 255);
  txBuf_len++;

#if DEBUG > 1
  Debug("tx Checksum is " + String(cs, HEX));
#endif

  // ETX
  txBuf[txBuf_len] = ETX;
  txBuf_len++;

#if DEBUG > 1
  String txDebug = " | txBuf: ";
  for (unsigned short int d = 0; d < sizeof(txBuf); d++) {
    txDebug = txDebug + String(txBuf[d], HEX) + " ";
  }
  Debug("len: " + String(txBuf_len, DEC) + txDebug);
#endif

  if (_OnTransmissionBegin != NULL)
    OnTransmissionBegin();
  PORTD |= (1 << PD2);
  delay(15);
  for (unsigned short int i = 0; i < txBuf_len; i++) {
    stream.write(txBuf[i]);
#ifdef DEBUG
    txBuf[i] = 0x00;
#endif
  }
  stream.flush();
  if (_OnTransmissionEnd != NULL)
    OnTransmissionEnd();

#ifdef DEBUG
  // empty buffer
  for (unsigned short int i = 0; i < sizeof(txBuf); i++) {
    txBuf[i] = 0x00;
  }
#endif
  txBuf_len = 0;
}