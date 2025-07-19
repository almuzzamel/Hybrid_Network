//

// HandoverManager.h

// Header file for handover management functionality

//



#ifndef __HANDOVER_MANAGER_H__

#define __HANDOVER_MANAGER_H__



#include "inet/common/INETDefs.h"

#include "inet/networklayer/common/InterfaceTable.h"

#include "inet/networklayer/contract/IRoutingTable.h"

#include "inet/networklayer/ipv4/Ipv4RoutingTable.h"

#include "inet/networklayer/ipv4/Ipv4Route.h"

#include "inet/common/geometry/common/Coord.h"

#include <fstream>



using namespace omnetpp;
namespace inet {




class HandoverManager : public cSimpleModule

{

  public:

    enum InterfaceType {

        LTE_INTERFACE = 0,

        WIFI_INTERFACE = 1

    };



  protected:

    // Module references

    IInterfaceTable *interfaceTable;

    IRoutingTable *routingTable;

    NetworkInterface *lteInterface;

    NetworkInterface *wifiInterface;



    // Parameters

    simtime_t measurementInterval;

    double lteRsrpThreshold;

    double wifiRssiThreshold;

    double hysteresis;

    simtime_t handoverDelay;



    int ltePriority;

    int wifiPriority;



    bool enableLteToWifi;

    bool enableWifiToLte;

    bool logHandovers;



    // State variables

    InterfaceType currentActiveInterface;

    InterfaceType targetActiveInterface;

    bool handoverInProgress;



    // Timers

    cMessage *measurementTimer;

    cMessage *handoverTimer;



    // Statistics

    int lteToWifiHandovers;

    int wifiToLteHandovers;



    // Signals for statistics

    simsignal_t lteToWifiHandoverSignal;

    simsignal_t wifiToLteHandoverSignal;

    simsignal_t lteRsrpSignal;

    simsignal_t wifiRssiSignal;



  protected:

    virtual void initialize(int stage) override;

    virtual void handleMessage(cMessage *msg) override;

    virtual void finish() override;

    virtual int numInitStages() const override { return NUM_INIT_STAGES; }



    // Measurement functions

    virtual void performMeasurements();

    virtual double getLteRsrp();

    virtual double getWifiRssi();

    virtual double simulateRsrpFromDistance();

    virtual double simulateRssiFromDistance();



    // Handover decision and execution

    virtual InterfaceType makeHandoverDecision(double lteRsrp, double wifiRssi);

    virtual void initiateHandover(InterfaceType targetInterface);

    virtual void executeHandover();

    virtual void setActiveInterface(InterfaceType interfaceType);

    virtual void updateRouting(NetworkInterface *activeInterface, NetworkInterface *inactiveInterface);



    // Utility functions

    virtual void logHandoverEvent();

    virtual std::string interfaceTypeToString(InterfaceType type);

};
}


#endif // __HANDOVER_MANAGER_H__
