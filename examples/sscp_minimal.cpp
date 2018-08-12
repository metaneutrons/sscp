/*
    Minimal example for sscp library
    Copyright 2018 Fabian Schmieder

    This example only is licensed under MIT license.

    Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#include <Arduino.h>
#include <sscp.h>

// set address of this node
#define NODE 1

// set Serial1 as stream object for sscp library
sscp msg = sscp(Serial1);

// timer variable for sending new message every 1s
unsigned int milli = millis();

void TransmissionBegin()
{
    // set PD2 high (eg for RS485 tx-enable )
    PORTD |= (1 << PD2);

    // give the RS485 transceiver some time
    delay(15);
}

void TransmissionEnd()
{
    // set PD2 low (eg for RS485 tx-disable )
    PORTD &= ~(1 << PD2);

    // give the RS485 transceiver some time
    delay(15);
}

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
                     String(msg.GetSource(), HEX) + " of type ");

        switch (msg.GetType())
        {
        case SSCP_PACKET_TYPE_CMD:
            Serial.print("CMD");
            break;
        case SSCP_PACKET_TYPE_NAK:
            Serial.print("NAK");
            break;
        case SSCP_PACKET_TYPE_ACK:
            Serial.print("ACK");
            break;
        case SSCP_PACKET_TYPE_NUL:
            Serial.print("NUL");
            break;
        }

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