//
// Copyright (C) 2011 Adriano (University of Pisa)
// Copyright (C) 2012 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

namespace inet;

//
// Represents a voice packet used by SimpleVoipSender / SimpleVoipReceiver
//
class CustomVoipPacket extends FieldsChunk
{
    uint16_t totalLengthField;         // length of total SimpleVoipPacket in bytes
    unsigned int talkspurtID;          // identifies the talkspurt
    unsigned int talkspurtNumPackets;  // number of all packets of current talkspurt
    unsigned int packetID;             // 0..talkspurtNumPackets-1
    simtime_t voipTimestamp;            // "mouth" timestamp of begin of voice in this packet
    simtime_t voiceDuration;            // duration of voice in this packet
    // Additional fields needed by VoIPWithHandover.cc
    unsigned int seqNum;              // sequence number for the packet
    simtime_t sendTime;               // time when packet was sent
    unsigned int sampleBits;          // sample bits for audio
    unsigned int sampleRate;          // sample rate for audio
    unsigned int transmitBitrate;     // transmission bitrate
    unsigned int retransmissionCount; // number of retransmissions
}

