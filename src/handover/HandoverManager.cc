//
// HandoverManager.cc
// Implementation of handover decision logic and execution
//

#include "HandoverManager.h"
#include "inet/networklayer/ipv4/Ipv4Route.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#include "inet/common/ModuleAccess.h"
#include "inet/mobility/contract/IMobility.h"
#include "stack/phy/layer/LtePhyUe.h"
#include "inet/linklayer/ieee80211/mgmt/Ieee80211MgmtSta.h"

using namespace omnetpp;
using namespace inet::ieee80211;
//using namespace simu5g;


namespace inet{


void HandoverManager::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        // Initialize parameters
        measurementInterval = par("measurementInterval");
        lteRsrpThreshold = par("lteRsrpThreshold").doubleValue();
        wifiRssiThreshold = par("wifiRssiThreshold").doubleValue();
        hysteresis = par("hysteresis").doubleValue();
        handoverDelay = par("handoverDelay");

        ltePriority = par("ltePriority");
        wifiPriority = par("wifiPriority");

        enableLteToWifi = par("enableLteToWifi");
        enableWifiToLte = par("enableWifiToLte");
        logHandovers = par("logHandovers");

        // Initialize state
        currentActiveInterface = LTE_INTERFACE;
        handoverInProgress = false;
        measurementTimer = nullptr;
        handoverTimer = nullptr;

        // Statistics
        lteToWifiHandovers = 0;
        wifiToLteHandovers = 0;

        // Register statistics
        lteToWifiHandoverSignal = registerSignal("lteToWifiHandover");
        wifiToLteHandoverSignal = registerSignal("wifiToLteHandover");
        lteRsrpSignal = registerSignal("lteRsrp");
        wifiRssiSignal = registerSignal("wifiRssi");
    }


    else if (stage == INITSTAGE_NETWORK_LAYER) {
        // Get references to interfaces
        cModule *host = getContainingNode(this);
        interfaceTable = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        routingTable = getModuleFromPar<IRoutingTable>(par("routingTableModule"), this);

        // DEBUG: Print all interfaces with available info
        EV_DEBUG << "=== DEBUG: Available interfaces ===" << endl;
        for (int i = 0; i < interfaceTable->getNumInterfaces(); i++) {
            NetworkInterface *iface = interfaceTable->getInterface(i);
            EV_DEBUG << "Interface " << i << ": '" << iface->getInterfaceName()
                     << "' (ID: " << iface->getInterfaceId() << ")" << endl;
        }
        EV_DEBUG << "=== END DEBUG ===" << endl;

        // Find LTE and WiFi interfaces based on name patterns
        lteInterface = nullptr;
        wifiInterface = nullptr;

        // Look for interfaces with correct naming patterns
        for (int i = 0; i < interfaceTable->getNumInterfaces(); i++) {
            NetworkInterface *iface = interfaceTable->getInterface(i);
            const char *name = iface->getInterfaceName();

            // Skip loopback interface
            if (strstr(name, "lo") != nullptr) continue;

            // LTE interface detection - common patterns
            if (strstr(name, "cellular") != nullptr ||
                strstr(name, "lte") != nullptr ||
                strstr(name, "LTE") != nullptr ||
                strstr(name, "cell") != nullptr) {
                lteInterface = iface;
                EV_INFO << "Found LTE interface: " << name << endl;
            }
            // WiFi interface detection - common patterns
            else if (strstr(name, "wlan") != nullptr ||
                     strstr(name, "wifi") != nullptr ||
                     strstr(name, "WiFi") != nullptr ||
                     strstr(name, "80211") != nullptr) {
                wifiInterface = iface;
                EV_INFO << "Found WiFi interface: " << name << endl;
            }
        }

        if (!lteInterface || !wifiInterface) {
            EV_ERROR << "ERROR: LTE found: " << (lteInterface ? lteInterface->getInterfaceName() : "NO")
                   << ", WiFi found: " << (wifiInterface ? wifiInterface->getInterfaceName() : "NO") << endl;
            throw cRuntimeError("Could not find both LTE and WiFi interfaces");
        }

        EV_INFO << "Successfully found both interfaces!" << endl;

        // Start the measurement timer to periodically check signal strengths
        measurementTimer = new cMessage("measurementTimer");
        scheduleAt(simTime() + measurementInterval, measurementTimer);

        // Initially set LTE as the active interface
        setActiveInterface(LTE_INTERFACE);

        EV_INFO << "HandoverManager initialized and measurements started" << endl;
    }
}

void HandoverManager::handleMessage(cMessage *msg)
{
    if (msg == measurementTimer) {
        performMeasurements();
        scheduleAt(simTime() + measurementInterval, measurementTimer);
    }
    else if (msg == handoverTimer) {
        executeHandover();
        handoverTimer = nullptr;
    }
    else {
        delete msg;
    }
}

void HandoverManager::performMeasurements()
{
    // Get current measurements
    double lteRsrp = getLteRsrp();
    double wifiRssi = getWifiRssi();

    // Emit signals for statistics
    emit(lteRsrpSignal, lteRsrp);
    emit(wifiRssiSignal, wifiRssi);

    EV_DEBUG << "Measurements - LTE RSRP: " << lteRsrp << " dBm, WiFi RSSI: "
             << wifiRssi << " dBm" << endl;

    // Make handover decision
    InterfaceType targetInterface = makeHandoverDecision(lteRsrp, wifiRssi);

    if (targetInterface != currentActiveInterface && !handoverInProgress) {
        initiateHandover(targetInterface);
    }
}

HandoverManager::InterfaceType HandoverManager::makeHandoverDecision(double lteRsrp, double wifiRssi)
{
    InterfaceType bestInterface = currentActiveInterface;

    // Apply hysteresis to prevent ping-pong handovers
    double lteThreshold = (currentActiveInterface == LTE_INTERFACE) ?
                         lteRsrpThreshold - hysteresis : lteRsrpThreshold + hysteresis;
    double wifiThreshold = (currentActiveInterface == WIFI_INTERFACE) ?
                          wifiRssiThreshold - hysteresis : wifiRssiThreshold + hysteresis;

    // Debug output
    EV_DEBUG << "Current interface: " << interfaceTypeToString(currentActiveInterface)
             << ", LTE RSRP: " << lteRsrp << " dBm (threshold: " << lteThreshold << " dBm)"
             << ", WiFi RSSI: " << wifiRssi << " dBm (threshold: " << wifiThreshold << " dBm)" << endl;

    // Enhanced handover decision logic with interface priorities
    if (wifiRssi > wifiThreshold) {
        // WiFi signal is good
        if (currentActiveInterface == LTE_INTERFACE && enableLteToWifi) {
            // Currently on LTE, switch to WiFi since it has good signal
            bestInterface = WIFI_INTERFACE;
            EV_INFO << "Handover decision: LTE->WiFi (RSRP: " << lteRsrp
                    << ", RSSI: " << wifiRssi << ") - Good WiFi signal detected" << endl;
        }
    } else {
        // WiFi signal is poor or not available
        if (currentActiveInterface == WIFI_INTERFACE && enableWifiToLte && lteRsrp > lteThreshold) {
            // Currently on WiFi but signal is poor, switch to LTE if it has good signal
            bestInterface = LTE_INTERFACE;
            EV_INFO << "Handover decision: WiFi->LTE (RSRP: " << lteRsrp
                    << ", RSSI: " << wifiRssi << ") - Poor WiFi signal" << endl;
        }
    }

    // Advanced decision logic: If both signals are good, use priority
    if (wifiRssi > wifiThreshold && lteRsrp > lteThreshold) {
        if (wifiPriority > ltePriority) {
            bestInterface = WIFI_INTERFACE;
            EV_INFO << "Both signals good, selecting WiFi based on priority" << endl;
        } else {
            bestInterface = LTE_INTERFACE;
            EV_INFO << "Both signals good, selecting LTE based on priority" << endl;
        }
    }

    return bestInterface;
}

void HandoverManager::initiateHandover(InterfaceType targetInterface)
{
    EV_INFO << "Initiating handover from " << interfaceTypeToString(currentActiveInterface)
            << " to " << interfaceTypeToString(targetInterface) << endl;

    handoverInProgress = true;
    targetActiveInterface = targetInterface;

    // Schedule handover execution after delay
    handoverTimer = new cMessage("handoverTimer");
    scheduleAt(simTime() + handoverDelay, handoverTimer);
}

void HandoverManager::executeHandover()
{
    EV_INFO << "Executing handover to " << interfaceTypeToString(targetActiveInterface) << endl;

    // Update statistics
    if (currentActiveInterface == LTE_INTERFACE && targetActiveInterface == WIFI_INTERFACE) {
        lteToWifiHandovers++;
        emit(lteToWifiHandoverSignal, 1);
    }
    else if (currentActiveInterface == WIFI_INTERFACE && targetActiveInterface == LTE_INTERFACE) {
        wifiToLteHandovers++;
        emit(wifiToLteHandoverSignal, 1);
    }

    // Perform the actual handover
    setActiveInterface(targetActiveInterface);

    // Log handover if enabled
    if (logHandovers) {
        logHandoverEvent();
    }

    handoverInProgress = false;
}

void HandoverManager::setActiveInterface(InterfaceType interfaceType)
{
    NetworkInterface *newActiveInterface = nullptr;
    NetworkInterface *newInactiveInterface = nullptr;

    if (interfaceType == LTE_INTERFACE) {
        newActiveInterface = lteInterface;
        newInactiveInterface = wifiInterface;
    }
    else {
        newActiveInterface = wifiInterface;
        newInactiveInterface = lteInterface;
    }

    // Update routing table to prefer the new active interface
    updateRouting(newActiveInterface, newInactiveInterface);

    currentActiveInterface = interfaceType;

    EV_INFO << "Active interface set to: " << newActiveInterface->getInterfaceName() << endl;
}

void HandoverManager::updateRouting(NetworkInterface *activeInterface, NetworkInterface *inactiveInterface)
{
    // Set the active interface up and inactive interface down
    // This is the key function that will enable/disable interfaces based on signal

    // First, set interface state (up/down)
    if (activeInterface == lteInterface && inactiveInterface == wifiInterface) {
        // When switching to LTE, turn off WiFi
        lteInterface->setState(NetworkInterface::UP);
        wifiInterface->setState(NetworkInterface::DOWN);
        EV_INFO << "Turned LTE interface ON and WiFi interface OFF" << endl;
    }
    else if (activeInterface == wifiInterface && inactiveInterface == lteInterface) {
        // When switching to WiFi, turn off LTE
        wifiInterface->setState(NetworkInterface::UP);
        lteInterface->setState(NetworkInterface::DOWN);
        EV_INFO << "Turned WiFi interface ON and LTE interface OFF" << endl;
    }

    // Remove old default route
    for (int i = routingTable->getNumRoutes() - 1; i >= 0; i--) {
        // Cast IRoute to Ipv4Route
        Ipv4Route *route = dynamic_cast<Ipv4Route*>(routingTable->getRoute(i));
        if (!route) continue; // Skip non-IPv4 routes

        if (route->getDestination().isUnspecified() && route->getNetmask().isUnspecified()) {
            routingTable->deleteRoute(route);
        }
    }

    // Add new default route via active interface
    Ipv4Route *defaultRoute = new Ipv4Route();
    defaultRoute->setDestination(Ipv4Address::UNSPECIFIED_ADDRESS);
    defaultRoute->setNetmask(Ipv4Address::UNSPECIFIED_ADDRESS);
    defaultRoute->setInterface(activeInterface);
    defaultRoute->setSourceType(IRoute::MANUAL);
    defaultRoute->setMetric(1);

    // Set gateway based on interface
    if (activeInterface == lteInterface) {
        // LTE gateway (typically the PGW)
        defaultRoute->setGateway(Ipv4Address("10.0.0.2"));
    }
    else {
        // WiFi gateway (typically the AP)
        defaultRoute->setGateway(Ipv4Address("192.168.1.1"));
    }

    routingTable->addRoute(defaultRoute);

    // Log interface change for applications to observe
    EV_INFO << "Interface change notification: Now using "
            << interfaceTypeToString(currentActiveInterface) << " interface" << endl;
}

double HandoverManager::getLteRsrp()
{
    // Get LTE PHY module and extract RSRP
    cModule *host = getContainingNode(this);
    cModule *lteNic = host->getSubmodule("cellularNic");
    if (!lteNic) return -150.0; // Very poor signal if no LTE

    cModule *ltePhy = lteNic->getSubmodule("phy");
    if (!ltePhy) return -150.0;

    // Try to get RSRP from LTE PHY layer
    LtePhyUe *phyUe = dynamic_cast<LtePhyUe*>(ltePhy);
    if (phyUe) {
        // Get the current master RSSI directly from the UE PHY
        // This value is updated during handover consideration and whenever
        // broadcasts are received from the current master eNB
        double rsrp = phyUe->getCurrentMasterRssi();

        // Check if we have a valid RSRP measurement
        if (rsrp > -990.0) {  // -999.0 is the initialization value
            EV_DEBUG << "Using real RSRP measurement from LtePhyUe: " << rsrp << " dBm" << endl;
            return rsrp;
        }
        else {
            EV_DEBUG << "No valid RSRP measurement available, using simulation" << endl;
        }
    }

    // Fallback: simulate RSRP based on distance
    return simulateRsrpFromDistance();
}

double HandoverManager::getWifiRssi()
{
    // Get WiFi management module and extract RSSI
    cModule *host = getContainingNode(this);
    cModule *wlan = host->getSubmodule("wlan", 0);
    if (!wlan) return -100.0; // Very poor signal if no WiFi

    // First try to get the radio module which might have signal strength info
    cModule *radio = wlan->getSubmodule("radio");
    if (radio) {
        // Try to access RSSI from the radio's reception state if available
        cModule *receptionState = radio->getSubmodule("receptionState");
        if (receptionState) {
            // Some radio models store the current RSSI/SINR in the reception state
            // This is implementation-specific, so we need to check what's available
            cProperty *rssiProp = receptionState->getProperties()->get("rssi");
            if (rssiProp) {
                const char* rssiStr = rssiProp->getValue(0);
                if (rssiStr) {
                    double rssi = atof(rssiStr);
                    if (rssi != 0) {
                        EV_DEBUG << "Using RSSI from radio reception state: " << rssi << " dBm" << endl;
                        return rssi;
                    }
                }
            }
        }
    }

    // Try to get RSSI from WiFi management
    cModule *mgmt = wlan->getSubmodule("mgmt");
    if (mgmt) {
        Ieee80211MgmtSta *mgmtSta = dynamic_cast<Ieee80211MgmtSta*>(mgmt);
        if (mgmtSta) {
            const auto *apInfo = mgmtSta->getAssociatedAp();
            if (apInfo) {
                EV_DEBUG << "Using RSSI from IEEE 802.11 management: " << apInfo->rxPower << " dBm" << endl;
                return apInfo->rxPower; // This gives you the received power in dBm
            }
            EV_DEBUG << "WiFi not associated with any AP, using poor signal value" << endl;
            return -95.0; // Default poor signal if not associated
        }
    }

    // If we couldn't get a real measurement, fall back to simulation
    EV_DEBUG << "No WiFi management module found, using simulated RSSI" << endl;
    return simulateRssiFromDistance();
}

double HandoverManager::simulateRsrpFromDistance()
{
    // Simple path loss model for LTE simulation
    cModule *host = getContainingNode(this);
    IMobility *mobility = check_and_cast<IMobility*>(host->getSubmodule("mobility"));
    Coord uePos = mobility->getCurrentPosition();

    // Assume eNB position (you may need to get this dynamically)
    Coord enbPos(391, 260, 0);

    double distance = uePos.distance(enbPos);
    double pathLoss = 128.1 + 37.6 * log10(distance / 1000.0); // 3GPP path loss model
    double txPower = 40.0; // eNB TX power in dBm

    return txPower - pathLoss + uniform(-5, 5); // Add some randomness
}

double HandoverManager::simulateRssiFromDistance()
{
    // Simple path loss model for WiFi simulation
    cModule *host = getContainingNode(this);
    IMobility *mobility = check_and_cast<IMobility*>(host->getSubmodule("mobility"));
    Coord uePos = mobility->getCurrentPosition();

    // Assume AP position (you may need to get this dynamically)
    Coord apPos(600, 350, 0);

    double distance = uePos.distance(apPos);
    double pathLoss = 20 * log10(distance) + 20 * log10(2400) - 147.55; // Free space path loss
    double txPower = 20.0; // AP TX power in dBm

    return txPower - pathLoss + uniform(-3, 3); // Add some randomness
}

void HandoverManager::logHandoverEvent()
{
    std::ofstream logFile;
    logFile.open(par("logFile").stringValue(), std::ios::app);

    logFile << simTime() << ","
            << interfaceTypeToString(currentActiveInterface) << ","
            << lteToWifiHandovers << ","
            << wifiToLteHandovers << std::endl;

    logFile.close();
}

std::string HandoverManager::interfaceTypeToString(InterfaceType type)
{
    switch (type) {
        case LTE_INTERFACE: return "LTE";
        case WIFI_INTERFACE: return "WiFi";
        default: return "Unknown";
    }
}

void HandoverManager::finish()
{
    // Clean up timers
    if (measurementTimer) {
        cancelAndDelete(measurementTimer);
    }
    if (handoverTimer) {
        cancelAndDelete(handoverTimer);
    }

    // Record final statistics
    recordScalar("lteToWifiHandovers", lteToWifiHandovers);
    recordScalar("wifiToLteHandovers", wifiToLteHandovers);
    recordScalar("totalHandovers", lteToWifiHandovers + wifiToLteHandovers);
}
}
Define_Module(inet::HandoverManager);
