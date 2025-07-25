//
// HandoverManager.ned
// Handover management module for dual-interface UE
//

package abcd.handover;

//
// HandoverManager handles the automatic switching between LTE and WiFi interfaces
// based on signal strength measurements and configured thresholds.
//
simple HandoverManager
{
    parameters:
        @display("i=block/control");
        
        string interfaceTableModule = default("^.interfaceTable");
        string routingTableModule = default("^.ipv4.routingTable");
        
        // Measurement and decision parameters
        double measurementInterval @unit(s) = default(1s);  // How often to check signal strength
        double lteRsrpThreshold @unit(dBm) = default(-80dBm);  // RSRP threshold for LTE
        double wifiRssiThreshold @unit(dBm) = default(-70dBm);  // RSSI threshold for WiFi
        double hysteresis @unit(dB) = default(5dB);  // Hysteresis to avoid ping-pong effect
        double handoverDelay @unit(s) = default(0.1s);  // Delay before executing handover
        
        // Interface priorities (higher = more preferred)
        int ltePriority = default(10);
        int wifiPriority = default(20);  // Default prefer WiFi when available
        
        // Enable/disable specific handover types
        bool enableLteToWifi = default(true);  // Allow switching from LTE to WiFi
        bool enableWifiToLte = default(true);  // Allow switching from WiFi to LTE
        
        // Logging options
        bool logHandovers = default(true);
        string logFile = default("handovers.csv");
        
        // Signals and statistics
        @signal[lteToWifiHandover](type=long);
        @signal[wifiToLteHandover](type=long);
        @signal[lteRsrp](type=double);
        @signal[wifiRssi](type=double);
        
        @statistic[lteToWifiHandover](title="LTE to WiFi handovers"; record=count; interpolationmode=none);
        @statistic[wifiToLteHandover](title="WiFi to LTE handovers"; record=count; interpolationmode=none);
        @statistic[lteRsrp](title="LTE RSRP measurements"; record=vector; interpolationmode=linear);
        @statistic[wifiRssi](title="WiFi RSSI measurements"; record=vector; interpolationmode=linear);

    gates:
        // No direct gate connections needed - HandoverManager communicates by direct method calls
}