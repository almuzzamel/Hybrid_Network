//
// Handover Manager Module
// Manages handover decisions between LTE and WiFi interfaces
//

package abcd.handover;


simple HandoverManager
{
    parameters:
        @class(HandoverManager);
        @display("i=block/control");
        
        // Measurement parameters
        double measurementInterval @unit(s) = default(1s);
        
        // Handover thresholds
        double lteRsrpThreshold @unit(dBm) = default(-100dBm);
        double wifiRssiThreshold @unit(dBm) = default(-70dBm);
        double hysteresis @unit(dB) = default(3dB);
        
        // Handover delays
        double handoverDelay @unit(s) = default(0.1s);
        double measurementDelay @unit(s) = default(0.05s);
        
        // Interface priorities (lower number = higher priority)
        int ltePriority = default(1);
        int wifiPriority = default(2);
        
    string interfaceTableModule;  // For accessing interface table
    string routingTableModule;    // For accessing routing table
        
        
        // Enable/disable specific handover types
        bool enableLteToWifi = default(true);
        bool enableWifiToLte = default(true);
        
        // Logging
        bool logHandovers = default(true);
        string logFile = default("handover.log");
        
    gates:
        // No gates needed - communicates via direct method calls
}