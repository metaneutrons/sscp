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

#ifndef __SSCP_h__
#define __SSCP_h__
#include <Arduino.h>

#define SSCP_VERSION 1.0

// buffer length
const int MAXIMUM_PAYLOAD = 18;
const int OVERHEAD_LENGTH =
    7; // minimum packet size due to overhead = STX LEN SRC DST TYPE CMD ETX

// constant for control bytes
const char STX = 0x02;
const char ETX = 0x03;
const char NUL = 0x00;
const char ACK = 0x06;
const char NAK = 0x15;

// fields

#define POS_STX 0
#define POS_LEN 1
#define POS_SRC 2
#define POS_DST 3
#define POS_TYPE 4
#define POS_CMD 5

// maximum message transmit tries and timeout
const int MAXIMUM_TX_TRYS = 3;
const int ACK_TIMEOUT = 200;

enum RX_STATE
{
  SYNCING,
  RECEIVING
};

enum TX_STATE
{
  IDLE,
  PACKET_READY,
  PACKET_SENT
};

enum SSCP_PACKET_TYPE
{
  SSCP_PACKET_TYPE_NUL = 0,
  SSCP_PACKET_TYPE_CMD = 1,
  SSCP_PACKET_TYPE_ACK = 6,
  SSCP_PACKET_TYPE_NAK = 21
};

enum SSCP_PACKET_ERROR
{
  SSCP_PACKET_ERROR_NONE,
  SSCP_PACKET_ERROR_NOSTX,
  SSCP_PACKET_ERROR_LENGTH,
  SSCP_PACKET_ERROR_TYPE,
  SSCP_PACKET_ERROR_NOETX,
  SSCP_PACKET_ERROR_CHECKSUM
};

typedef void (*TransmissionStateFunction)();

// sscp class
class sscp
{
public:
  // public functions
  sscp(Stream &s = Serial) : stream(s) {}

  void SetOnTransmissionBeginHandler(TransmissionStateFunction func);
  void SetOnTransmissionEndHandler(TransmissionStateFunction func);

  void loop();

  // rx functions
  bool IsPacketAvailable(unsigned char node);
  unsigned char GetSource();
  unsigned char GetDestination();
  unsigned short int GetPayloadLength();
  unsigned char GetCommand();
  unsigned char GetType();
  unsigned char GetPayload(unsigned short int pos);
  void ReleasePacket();
  void rxBuf_nextSTX();
  void rxBuf_lshift();

  // tx functions
  void MessageBegin(unsigned char src, unsigned char dst, SSCP_PACKET_TYPE type,
                    unsigned char cmd);
  void MessageAdd(unsigned char c);
  void MessageSend();

private:
  // private functions
  void (*_OnTransmissionBegin)(bool);
  void (*_OnTransmissionEnd)(bool);

  TransmissionStateFunction OnTransmissionBegin;
  TransmissionStateFunction OnTransmissionEnd;

  // private variables
  Stream &stream;

  // rx
  RX_STATE rxState = SYNCING;
  unsigned char rxBuf[(MAXIMUM_PAYLOAD + OVERHEAD_LENGTH)];
  unsigned short int rxBuf_len = 0;
  unsigned char rxPacket[MAXIMUM_PAYLOAD + OVERHEAD_LENGTH];

  SSCP_PACKET_ERROR IsValidPacket(unsigned short int pos);

  // tx
  TX_STATE txState = IDLE;
  short int txPin = -1;
  unsigned char txBuf[MAXIMUM_PAYLOAD + OVERHEAD_LENGTH];
  unsigned short int txBuf_len = 0;
};

#endif