sscp
====

An Arduino-framework based library for the simplified but structured communication over serial lines. It's some kind of a [Modbus](https://en.wikipedia.org/wiki/Modbus)-like approach of communication but more flexible and customizable.

## Download

The git repository on GitHub contains the development version of the library. As this is work in progress it may take some time to see "stable" releases [on the releases page](https://github.com/metaneutrons/sscp/releases).

## Features

* complete capsulation of serial communcation
* use of StreamObject makes the library compatible with hardware UARTs, software serial and even I2C communication
* due to callback functions it's easily possible to implement RS485 tx-enable/disable functions

## Protocol

The protocol is pretty straight forward. Every message has the following format:

``<STX> <LEN> <TYPE> <CMD> <PAYLOAD 1 ... n> <CS> <ETX>``

**STX** = 0x02 = marks start of message

**LEN** = byte = length of following bytes including CRC but excluding ETX

**TYPE** = byte = type of message as follows

>     SSCP_PACKET_TYPE_NUL = 0x00 = do nothing
>     SSCP_PACKET_TYPE_CMD = 0x01 = send command
>     SSCP_PACKET_TYPE_ACK = 0x06 = acknowledge of command
>     SSCP_PACKET_TYPE_NAK = 0x15 = not acknowledge of command

**CMD** = byte = self definable command

**PAYLOAD** = 1 to 16 bytes of payload (MAXIMUM_PAYLOAD const in sscp.h can be raised!)

**CS** = byte = checksum calculated as follows

```c++
  unsigned char payload[16] = "Payload!Payload!"
  unsigned char cs = 0x7f;
  
  for (int i = 0; i < sizeof(payload); i++)
    cs = cs + payload[i];

  cs = cs & 255;
```

**ETX** = 0x03 = marks end of message

## Example 
```c++
#include <Arduino.h>
#include <sscp.h>

// set address of this node
#define NODE 1

// set Serial1 as stream object for sscp library
sscp msg = sscp(Serial1);

// timer variable for sending new message every 1s
unsigned int milli = millis();

void setup()
{
    Serial.begin(115200);  // for debug output
    Serial1.begin(115200); // for sscp library

    // set callback handler for eg RS485 tx-enable/disable
    msg.SetOnTransmissionBeginHandler(TransmissionBegin);
    msg.SetOnTransmissionEndHandler(TransmissionEnd);
}

void loop()
{
    // sscp loop function
    msg.loop();

    // Is packet for this node available?
    if (msg.IsPacketAvailable(NODE))
    {
        Serial.print(String(millis()) + ": Rcvd packet from " +
                     String(msg.GetSource(), HEX) + " of type " + String(msg.GetType(), HEX));

        Serial.print(" with CMD " + String(msg.GetCommand(), HEX));

        Serial.print(" and payload");

        for (unsigned short int i = 0; i < msg.GetPayloadLength(); i++)
                         Serial.print(" " + String(msg.GetPayload(i), HEX)));

        Serial.println(".");

        // clear RX buffer and avoid processing this packet again
        msg.ReleasePacket();
    }

    // send message every 1s
    if (milli < millis())
    {
        // start new message from adr 1 to adr 2 of type CMD with CMD byte set to
        // 0x10 and payload 0xFA 0xB1 0xA4
        msg.MessageBegin(NODE, 2, SSCP_PACKET_TYPE_CMD, 0x10);
        msg.Add(0xFA);
        msg.Add(0xB1);
        msg.Add(0xA4);
        msg.MessageSend();

        milli = millis() + 1000; // next tick
    }
}
```

## Requirements, installation and usage

The library itself has no dependencies. Put it in your libs folder and include 'sscp.h'.

As there is no special magic in the library it should work on all platforms the Arduino-framework is ported to. I personally tested the library on an ESP8266 and a Teensy 3.1 and 3.6.

## Todos

I would like to implement some more callback functions (eg OnPacket) and implement non-blocking waiting for ACK or NAK. This logic has to be implemented by the developer at the moment.

## Licensing

The library is licensed unter [GNU Lesser General Public License v3](https://www.gnu.org/licenses/lgpl-3.0.en.html). The examples are licensed under [MIT license](https://opensource.org/licenses/MIT).

## Donate

I developed this library for my personal learning experience. The library is far from being perfect and probably is not even a good example of how to develop a library. Therefore, I do not ask for money, but I hope that others will develop the library and remove my mistakes. But if you absolutely want to send an attention to me, then I do like an invite for a coffee...

[![Donate button](https://www.paypal.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.me/metaneutrons)
