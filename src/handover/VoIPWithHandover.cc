//
// VoIPWithHandover.cc
// Implementation of VoIP applications with session continuity
//

#include "VoIPWithHandover.h"
#include "CustomVoipSender.h"
#include "CustomVoipReceiver.h"
#include "CustomVoipPacket_m.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/chunk/ByteCountChunk.h"
#include "inet/transportlayer/udp/UdpHeader_m.h"
#include "inet/common/socket/SocketTag_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/TransportPseudoHeader_m.h"

Define_Module(inet::VoIPSenderWithHandover);
Define_Module(inet::VoIPReceiverWithHandover);
using namespace inet;
using namespace omnetpp;
namespace inet{


// VoIPSenderWithHandover Implementation
void VoIPSenderWithHandover::initialize(int stage)
{
    CustomVoipSender::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        // Initialize session continuity parameters
        enableSessionContinuity = par("enableSessionContinuity").boolValue();
        bufferTime = par("bufferTime").doubleValue();
        maxRetransmissions = par("maxRetransmissions").intValue();
        handoverDetectionInterval = par("handoverDetectionInterval");

        // Initialize state
        nextSequenceNumber = 1;
        ackSequenceNumber = 0;
        packetsLostDuringHandover = 0;
        packetsRetransmitted = 0;
        handoverEvents = 0;
        isAppActive = false;

        // Initialize timers
        handoverDetectionTimer = new cMessage("handoverDetection");
        selfMsg = new cMessage("sendPacket");

        // Register signals
        packetSentSignal = registerSignal("packetSent");
        packetLossSignal = registerSignal("packetLoss");
        retransmissionSignal = registerSignal("retransmission");
        handoverDetectedSignal = registerSignal("handoverDetected");

        EV_INFO << "VoIPSenderWithHandover initialized with session continuity: "
                << enableSessionContinuity << endl;
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        // Start handover detection if enabled
        if (enableSessionContinuity) {
            scheduleAt(simTime() + handoverDetectionInterval, handoverDetectionTimer);
        }
    }
}

void VoIPSenderWithHandover::handleMessage(cMessage *msg)
{
    if (msg == handoverDetectionTimer) {
        processHandoverDetection();
        if (isAppActive) {
            scheduleAt(simTime() + handoverDetectionInterval, handoverDetectionTimer);
        }
    }
    else if (msg == selfMsg) {
        // Handle timer-based packet sending
        sendVoIPPacket();
    }
    else if (msg->getKind() == UDP_I_ERROR) {
        // Handle UDP socket errors (including ICMP errors)
        EV_WARN << "UDP socket error received: " << msg->getName() << endl;
        handleSocketError(msg);
    }
    else {
        // Handle received packets (e.g., ACKs)
        Packet *packet = dynamic_cast<Packet*>(msg);
        if (packet) {
            handleAckPacket(packet);
        }
        else {
            // Check if it's an indication message (generic handling)
            if (dynamic_cast<cObject*>(msg) && strstr(msg->getClassName(), "Indication")) {
                EV_INFO << "Received indication: " << msg->getName()
                        << " of type: " << msg->getClassName() << endl;
                handleIndication(msg);
            }
            else {
                // Fall back to parent class handling
                CustomVoipSender::handleMessage(msg);
            }
        }
    }
}

// Simplified socket error handler
void VoIPSenderWithHandover::handleSocketError(cMessage *msg)
{
    EV_WARN << "Socket error: " << msg->getName() << " - may indicate network connectivity issues" << endl;

    // If this is during active communication, it might indicate a handover
    if (isAppActive) {
        EV_INFO << "Socket error during active session - potential handover scenario" << endl;
        // You might want to trigger handover detection here
        if (enableSessionContinuity) {
            processHandoverDetection();
        }
    }

    delete msg;
}

// Simplified indication handler
void VoIPSenderWithHandover::handleIndication(cMessage *msg)
{
    EV_INFO << "Handling indication: " << msg->getName()
            << " of type: " << msg->getClassName() << endl;

    // Check if this might be a socket error that could indicate handover
    if (strstr(msg->getClassName(), "Error") || strstr(msg->getName(), "error")) {
        EV_WARN << "Error indication received - potential handover scenario" << endl;
        if (isAppActive && enableSessionContinuity) {
            processHandoverDetection();
        }
    }

    delete msg;
}
void VoIPSenderWithHandover::sendVoIPPacket()
{
    if (!isAppActive) return;

    // Check if destination address is unspecified and resolve it if needed
    if (destAddress.isUnspecified()) {
        destAddress = L3AddressResolver().resolve(par("destAddress"));

        // If still unspecified, log error and return
        if (destAddress.isUnspecified()) {
            EV_ERROR << "Cannot resolve destination address: " << par("destAddress").stringValue() << endl;
            return;
        }
    }
    // Create packet with sequence number
    Packet *packet = createVoIPPacketWithSeq(nextSequenceNumber);

    if (enableSessionContinuity) {
        // Buffer packet for potential retransmission
        bufferPacket(packet->dup(), nextSequenceNumber);
    }

    // Send packet
    socket.sendTo(packet, destAddress, destPort);

    lastPacketSent = simTime();
    nextSequenceNumber++;

    // Update statistics
    numSent++;
    emit(packetSentSignal, packet);

    // Schedule next packet if not finished
    if (!stopTime.isZero() && simTime() >= stopTime) {
        return;
    }
    if (nextSequenceNumber <= par("numPackets").intValue()) {
        scheduleAfter(par("sendInterval"), selfMsg);
    }

    EV_DEBUG << "Sent VoIP packet with sequence " << (nextSequenceNumber - 1) << endl;
}

Packet* VoIPSenderWithHandover::createVoIPPacketWithSeq(int seqNum)
{
    // Create VoIP packet
    Packet *packet = new Packet("VoIPPacket");

    // Create VoIP header with sequence number
    auto voipHeader = makeShared<CustomVoipPacket>();
    voipHeader->setSeqNum(seqNum);  // Use setSeqNum instead of setSequenceNumber
    voipHeader->setSendTime(simTime());  // Use setSendTime instead of setTimestamp
    voipHeader->setTalkspurtID(talkspurtID);
    voipHeader->setVoipTimestamp(getCurrentVoipTimestamp());  // Use method to get timestamp
    voipHeader->setSampleBits(getSampleBits());  // Use getter method
    voipHeader->setSampleRate(getSampleRate());  // Use getter method
    voipHeader->setTransmitBitrate(getTransmitBitrate());  // Use getter method
    voipHeader->setRetransmissionCount(0);  // Initialize retransmission count

    packet->insertAtFront(voipHeader);

    // Add payload
    int payloadSize = par("packetSize").intValue() - voipHeader->getChunkLength().get();
    if (payloadSize > 0) {
        auto payload = makeShared<ByteCountChunk>(B(payloadSize));
        packet->insertAtBack(payload);
    }

    return packet;
}

void VoIPSenderWithHandover::bufferPacket(Packet *packet, int seqNum)
{
    // Store packet for potential retransmission
    packetBuffer[seqNum] = packet;

    // Clean old packets from buffer
    simtime_t currentTime = simTime();
    for (auto it = packetBuffer.begin(); it != packetBuffer.end();) {
        auto voipHeader = it->second->peekAtFront<CustomVoipPacket>();
        if (currentTime - voipHeader->getSendTime() > bufferTime) {  // Use getSendTime
            delete it->second;
            it = packetBuffer.erase(it);
        }
        else {
            ++it;
        }
    }
}

void VoIPSenderWithHandover::processHandoverDetection()
{
    // Check if handover is in progress
    if (isHandoverInProgress()) {
        EV_INFO << "Handover detected, checking for lost packets" << endl;
        handoverEvents++;
        emit(handoverDetectedSignal, 1);

        if (enableSessionContinuity) {
            retransmitLostPackets();
        }
    }
}

bool VoIPSenderWithHandover::isHandoverInProgress()
{
    // Simple handover detection based on lack of ACKs
    simtime_t timeSinceLastAck = simTime() - lastPacketSent;
    return timeSinceLastAck > 2 * par("sendInterval").doubleValue();
}

void VoIPSenderWithHandover::retransmitLostPackets()
{
    // Retransmit packets that haven't been acknowledged
    for (auto& pair : packetBuffer) {
        int seqNum = pair.first;
        if (seqNum > ackSequenceNumber) {
            Packet *retransmitPacket = pair.second->dup();

            // Update retransmission count in packet
            auto voipHeader = retransmitPacket->removeAtFront<CustomVoipPacket>();
            voipHeader->setRetransmissionCount(voipHeader->getRetransmissionCount() + 1);
            retransmitPacket->insertAtFront(voipHeader);

            if (voipHeader->getRetransmissionCount() <= maxRetransmissions) {
                socket.sendTo(retransmitPacket, destAddress, destPort);
                packetsRetransmitted++;
                emit(retransmissionSignal, 1);

                EV_DEBUG << "Retransmitted packet " << seqNum
                         << " (attempt " << voipHeader->getRetransmissionCount() << ")" << endl;
            }
            else {
                delete retransmitPacket;
                packetsLostDuringHandover++;
                emit(packetLossSignal, 1);
                EV_WARN << "Packet " << seqNum << " lost after max retransmissions" << endl;
            }
        }
    }
}

void VoIPSenderWithHandover::handleAckPacket(Packet *packet)
{
    // Extract ACK sequence number
    auto voipHeader = packet->peekAtFront<CustomVoipPacket>();
    int ackedSeq = voipHeader->getSeqNum();  // Use getSeqNum instead of getSequenceNumber

    if (ackedSeq > ackSequenceNumber) {
        ackSequenceNumber = ackedSeq;

        // Remove acknowledged packets from buffer
        for (auto it = packetBuffer.begin(); it != packetBuffer.end();) {
            if (it->first <= ackedSeq) {
                delete it->second;
                it = packetBuffer.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    delete packet;
}

// Helper methods to get VoIP parameters
simtime_t VoIPSenderWithHandover::getCurrentVoipTimestamp()
{
    // This should return the current VoIP timestamp
    // You may need to implement this based on your VoIP timing
    return simTime();
}

int VoIPSenderWithHandover::getSampleBits()
{
    // Return sample bits parameter
    return par("sampleBits").intValue();
}

int VoIPSenderWithHandover::getSampleRate()
{
    // Return sample rate parameter
    return par("sampleRate").intValue();
}

int VoIPSenderWithHandover::getTransmitBitrate()
{
    // Return transmit bitrate parameter
    return par("transmitBitrate").intValue();
}

VoIPSenderWithHandover::~VoIPSenderWithHandover()
{
    // Clean up buffer
    for (auto& pair : packetBuffer) {
        delete pair.second;
    }
    packetBuffer.clear();

    if (handoverDetectionTimer) {
        cancelAndDelete(handoverDetectionTimer);
    }
    
    if (selfMsg) {
        cancelAndDelete(selfMsg);
    }
}

void VoIPSenderWithHandover::finish()
{
    CustomVoipSender::finish();

    recordScalar("packetsLostDuringHandover", packetsLostDuringHandover);
    recordScalar("packetsRetransmitted", packetsRetransmitted);
    recordScalar("handoverEvents", handoverEvents);
}

// VoIPReceiverWithHandover Implementation
void VoIPReceiverWithHandover::initialize(int stage)
{
    CustomVoipReceiver::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        // Initialize session continuity parameters
        enableSessionContinuity = par("enableSessionContinuity").boolValue();
        jitterBufferSize = par("jitterBufferSize").doubleValue();
        handoverTimeout = par("handoverTimeout");

        // Initialize state
        expectedSequenceNumber = 1;
        lastAckedSequence = 0;
        packetsReordered = 0;
        duplicatePackets = 0;
        outOfOrderPackets = 0;

        // Initialize timer
        handoverTimeoutTimer = new cMessage("handoverTimeout");

        // Register signals
        packetReorderSignal = registerSignal("packetReorder");
        duplicatePacketSignal = registerSignal("duplicatePacket");
        jitterSignal = registerSignal("jitter");

        EV_INFO << "VoIPReceiverWithHandover initialized with session continuity: "
                << enableSessionContinuity << endl;
    }
}


void VoIPReceiverWithHandover::handleMessage(cMessage *msg)
{
    if (msg == handoverTimeoutTimer) {
        handleHandoverTimeout();
    }
    else if (Packet *packet = dynamic_cast<Packet*>(msg)) {
        if (enableSessionContinuity) {
            processReceivedPacket(packet);
        }
        else {
            CustomVoipReceiver::handleMessage(msg);
        }
    }
    else {
        CustomVoipReceiver::handleMessage(msg);
    }
}
void VoIPReceiverWithHandover::extractSenderInfo(Packet *packet)
{
    // Extract sender information from packet headers

    // Get the L3 address tag to extract source address
    auto l3AddressTag = packet->getTag<L3AddressInd>();
    if (l3AddressTag) {
        srcAddress = l3AddressTag->getSrcAddress();
    }

    // Get the UDP header to extract source port
    auto udpHeader = packet->peekAtFront<UdpHeader>();
    if (udpHeader) {
        srcPort = udpHeader->getSrcPort();
    }
}

void VoIPReceiverWithHandover::processReceivedPacket(Packet *packet)
{

        lastPacketReceived = simTime();

        // Extract sender information from packet
        extractSenderInfo(packet);

        // Reset handover timeout
        if (handoverTimeoutTimer->isScheduled()) {
            cancelEvent(handoverTimeoutTimer);
        }

    scheduleAt(simTime() + handoverTimeout, handoverTimeoutTimer);

    // Extract sequence number
    int seqNum = extractSequenceNumber(packet);

    // Check for duplicates
    if (receivedPackets.find(seqNum) != receivedPackets.end()) {
        duplicatePackets++;
        emit(duplicatePacketSignal, 1);
        delete packet;
        return;
    }

    // Store packet
    receivedPackets[seqNum] = packet;

    // Send ACK
    sendAckPacket(seqNum);

    // Check if packet is in order
    if (seqNum == expectedSequenceNumber) {
        reorderAndPlayPackets();
    }
    else if (seqNum < expectedSequenceNumber) {
        // Late packet - might be retransmission
        EV_DEBUG << "Received late packet " << seqNum
                 << " (expected " << expectedSequenceNumber << ")" << endl;
    }
    else {
        // Out of order packet
        outOfOrderPackets++;
        EV_DEBUG << "Received out-of-order packet " << seqNum
                 << " (expected " << expectedSequenceNumber << ")" << endl;
    }
}


int VoIPReceiverWithHandover::extractSequenceNumber(Packet *packet)
{
    auto voipHeader = packet->peekAtFront<CustomVoipPacket>();
    return voipHeader->getSeqNum();  // Use getSeqNum instead of getSequenceNumber
}

void VoIPReceiverWithHandover::sendAckPacket(int seqNum)
{
    // Create ACK packet
    Packet *ackPacket = new Packet("VoIPAck");
    auto ackHeader = makeShared<CustomVoipPacket>();
    ackHeader->setSeqNum(seqNum);  // Use setSeqNum
    ackHeader->setSendTime(simTime());  // Use setSendTime
    ackPacket->insertAtFront(ackHeader);

    // Send back to sender (this is simplified - you may need proper addressing)
    socket.sendTo(ackPacket, srcAddress, srcPort);
    lastAckedSequence = seqNum;
}

void VoIPReceiverWithHandover::reorderAndPlayPackets()
{
    // Play packets in sequence
    while (receivedPackets.find(expectedSequenceNumber) != receivedPackets.end()) {
        Packet *packet = receivedPackets[expectedSequenceNumber];

        // Calculate jitter
        auto voipHeader = packet->peekAtFront<CustomVoipPacket>();
        simtime_t jitter = simTime() - voipHeader->getSendTime();  // Use getSendTime
        emit(jitterSignal, jitter.dbl());

        // Process packet (delegate to base class)
        CustomVoipReceiver::handleMessage(packet);

        // Remove from buffer
        receivedPackets.erase(expectedSequenceNumber);
        expectedSequenceNumber++;

        if (expectedSequenceNumber > 1) {
            packetsReordered++;
            emit(packetReorderSignal, 1);
        }
    }
}

void VoIPReceiverWithHandover::handleHandoverTimeout()
{
    EV_INFO << "Handover timeout - assuming lost packets" << endl;

    // Skip missing packets and continue with available ones
    auto it = receivedPackets.upper_bound(expectedSequenceNumber);
    if (it != receivedPackets.end()) {
        expectedSequenceNumber = it->first;
        reorderAndPlayPackets();
    }
}

VoIPReceiverWithHandover::~VoIPReceiverWithHandover()
{
    // Clean up received packets buffer
    for (auto& pair : receivedPackets) {
        delete pair.second;
    }
    receivedPackets.clear();

    if (handoverTimeoutTimer) {
        cancelAndDelete(handoverTimeoutTimer);
    }
}

void VoIPReceiverWithHandover::finish()
{
    CustomVoipReceiver::finish();

    recordScalar("packetsReordered", packetsReordered);
    recordScalar("duplicatePackets", duplicatePackets);
    recordScalar("outOfOrderPackets", outOfOrderPackets);
}
}
