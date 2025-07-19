//
// VoIPWithHandover.h
// Enhanced VoIP application that maintains session continuity during handovers
//
#ifndef __VOIP_WITH_HANDOVER_H__
#define __VOIP_WITH_HANDOVER_H__

#include "CustomVoipSender.h"
#include "CustomVoipReceiver.h"
#include "CustomVoipPacket_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/common/socket/SocketTag_m.h"
#include "inet/applications/base/ApplicationBase.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"

#include "inet/common/packet/Packet.h"
#include <map>

using namespace inet;
using namespace omnetpp;
namespace inet{


class VoIPSenderWithHandover : public CustomVoipSender
{
  protected:
    // Session continuity parameters
    bool enableSessionContinuity;
    double bufferTime;
    int maxRetransmissions;

    // Handover detection
    cMessage *handoverDetectionTimer;
    cMessage *selfMsg;  // Added missing selfMsg


    simtime_t lastPacketSent;
    simtime_t handoverDetectionInterval;

    // Application state
    bool isAppActive;  // Added missing isAppActive
    int numSent;       // Added missing numSent counter

    // Packet buffering for continuity
    std::map<int, Packet*> packetBuffer;
    int nextSequenceNumber;
    int ackSequenceNumber;

    // Statistics
    int packetsLostDuringHandover;
    int packetsRetransmitted;
    int handoverEvents;
    simsignal_t packetSentSignal;
    simsignal_t packetLossSignal;
    simsignal_t retransmissionSignal;
    simsignal_t handoverDetectedSignal;

  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    // Enhanced VoIP functions
    virtual void sendVoIPPacket();
    virtual void processHandoverDetection();
    virtual void bufferPacket(Packet *packet, int seqNum);
    virtual void retransmitLostPackets();
    virtual void handleAckPacket(Packet *packet);
    virtual bool isHandoverInProgress();

    // Packet creation with sequence numbers
    virtual Packet* createVoIPPacketWithSeq(int seqNum);

    // Helper methods to get VoIP parameters
    virtual simtime_t getCurrentVoipTimestamp();
    virtual int getSampleBits();
    virtual int getSampleRate();
    virtual int getTransmitBitrate();
    virtual void handleSocketError(cMessage *msg);
        virtual void handleIndication(cMessage *msg);
  public:
    VoIPSenderWithHandover() : CustomVoipSender() {
        handoverDetectionTimer = nullptr;
        selfMsg = nullptr;
        enableSessionContinuity = false;
        nextSequenceNumber = 1;
        ackSequenceNumber = 0;
        packetsLostDuringHandover = 0;
        packetsRetransmitted = 0;
        handoverEvents = 0;
        isAppActive = false;
        numSent = 0;
    }
    virtual ~VoIPSenderWithHandover();
};

class VoIPReceiverWithHandover : public CustomVoipReceiver
{
  protected:
    // Session continuity parameters
    bool enableSessionContinuity;
    double jitterBufferSize;

    // Packet reordering and duplicate detection
    std::map<int, Packet*> receivedPackets;
    int expectedSequenceNumber;
    int lastAckedSequence;

    // Handover detection
    simtime_t lastPacketReceived;
    simtime_t handoverTimeout;
    cMessage *handoverTimeoutTimer;

    // Sender address tracking for ACKs
    L3Address srcAddress;
    int srcPort;

    // Statistics
    int packetsReordered;
    int duplicatePackets;
    int outOfOrderPackets;
    simsignal_t packetReorderSignal;
    simsignal_t duplicatePacketSignal;
    simsignal_t jitterSignal;

  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    // Enhanced VoIP functions
    virtual void processReceivedPacket(Packet *packet);
    virtual int extractSequenceNumber(Packet *packet);
    virtual void sendAckPacket(int seqNum);
    virtual void reorderAndPlayPackets();
    virtual void handleHandoverTimeout();
    virtual void extractSenderInfo(Packet *packet);  // Added helper method

  public:
    VoIPReceiverWithHandover() : CustomVoipReceiver() {
        handoverTimeoutTimer = nullptr;
        enableSessionContinuity = false;
        expectedSequenceNumber = 1;
        lastAckedSequence = 0;
        packetsReordered = 0;
        duplicatePackets = 0;
        outOfOrderPackets = 0;
        srcPort = 0;
    }
    virtual ~VoIPReceiverWithHandover();
};
}

#endif // __VOIP_WITH_HANDOVER_H__
