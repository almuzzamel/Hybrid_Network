//
// Copyright (C) 2011 Adriano (University of Pisa)
// Copyright (C) 2012 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#ifndef __CUSTOM_VOIP_SENDER_H
#define __CUSTOM_VOIP_SENDER_H

#include <string.h>

#include "inet/common/lifecycle/LifecycleUnsupported.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"

namespace inet {
#ifndef ABCD_API
#define ABCD_API
#endif

/**
 * Implements a simple VoIP source. See the NED file for more information.
 */
class ABCD_API CustomVoipSender : public inet::cSimpleModule, public LifecycleUnsupported
{
  public:
    UdpSocket socket;

    // parameters
    simtime_t stopTime;
    simtime_t packetizationInterval;
    int localPort = -1;
    int destPort = -1;
    int talkPacketSize = 0;
    L3Address destAddress;

    // state
    cMessage *selfSender = nullptr; // timer for sending packets
    cMessage *selfSource = nullptr; // timer for changing talkspurt/silence periods - FIXME be more specific with the name of this self message
    simtime_t silenceDuration;
    simtime_t talkspurtDuration;
    int packetID = -1;
    int talkspurtID = -1;
    int talkspurtNumPackets = 0;
    bool isTalk = false;

  protected:
    void talkspurt(simtime_t dur);
    void selectTalkOrSilenceInterval();
    void sendVoIPPacket();

    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;

  public:
    virtual ~CustomVoipSender();
    CustomVoipSender();
};

} // namespace inet

#endif

