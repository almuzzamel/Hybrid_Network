//
// VoIPWithHandover.ned
// Network description for VoIP applications with session continuity in hybrid WiFi/LTE networks
//

package abcd.handover;

import inet.applications.contract.IApp;
import inet.node.inet.StandardHost;
import inet.node.inet.Router;
import inet.node.wireless.AccessPoint;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.physicallayer.wireless.common.medium.RadioMedium;
import inet.linklayer.ethernet.EthernetInterface;
import ned.DatarateChannel;
import inet.node.ethernet.Eth10G;
import inet.node.internetcloud.InternetCloud;

// SimuLTE imports for LTE support
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.PgwStandard;
import simu5g.nodes.eNodeB;
import simu5g.world.radio.LteChannelControl;
import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;

// Import your custom base modules
import abcd.handover.CustomVoipSender;
import abcd.handover.CustomVoipReceiver;
import abcd.nodes.DualUe;

//
// Enhanced VoIP sender with hybrid network handover support
// Supports both WiFi-WiFi, LTE-LTE, and WiFi-LTE handovers
//
// Simplified VoIP sender with handover support
// Extends CustomVoipSender but directly implements IApp
module VoIPSenderWithHandover extends CustomVoipSender like IApp
{
    parameters:
        @class(VoIPSenderWithHandover);
        @display("i=block/app2");
        
        // Connection parameters
        string localAddress = default("");
       
        
        // Session parameters
        bool enableSessionContinuity = default(true);
        
        
        // Session continuity parameters
        double bufferTime @unit(s) = default(0.2s);
        int maxRetransmissions = default(5);
        
        // Handover detection parameters
        double handoverDetectionInterval @unit(s) = default(1s);
        double sendInterval @unit(s) = default(20ms);
        double verticalHandoverThreshold = default(-90);
        double horizontalHandoverThreshold = default(-85);
        
        // Voice codec parameters
        string codec = default("G.711");
        int sampleRate @unit(Hz) = default(8000Hz);
        int bitsPerSample = default(16);
        int packetSize @unit(B) = default(160B);
        
        // QoS parameters
        int dscp = default(46);
        int tos = default(-1);
        
        // Network interface monitoring
        bool enableInterfaceMonitoring = default(true);
        double rssiUpdateInterval @unit(s) = default(0.5s);
        string preferredInterface @enum("wifi","lte","auto") = default("auto");
        
        // Statistics signals and collection
        @signal[packetLoss](type=long);
        @signal[retransmissionEvent](type=long);
        @signal[horizontalHandover](type=long);
        @signal[verticalHandover](type=long);
        @signal[wifiToLteHandover](type=long);
        @signal[lteToWifiHandover](type=long);
        @signal[rssiMeasurement](type=double);
        @signal[activeInterface](type=string);
        @signal[handoverDetected](type=long);
        
        @statistic[packetLoss](title="Packet Loss"; source=packetLoss; record=count,sum,vector; interpolationmode=none);
        @statistic[retransmissions](title="Retransmissions"; source=retransmissionEvent; record=count,sum,vector; interpolationmode=none);
        @statistic[horizontalHandovers](title="Horizontal Handovers"; source=horizontalHandover; record=count,sum,vector; interpolationmode=none);
        @statistic[verticalHandovers](title="Vertical Handovers"; source=verticalHandover; record=count,sum,vector; interpolationmode=none);
        @statistic[wifiToLteHandovers](title="WiFi to LTE Handovers"; source=wifiToLteHandover; record=count,sum,vector; interpolationmode=none);
        @statistic[lteToWifiHandovers](title="LTE to WiFi Handovers"; source=lteToWifiHandover; record=count,sum,vector; interpolationmode=none);
        @statistic[rssi](title="RSSI Measurements"; source=rssiMeasurement; record=stats,histogram,vector; interpolationmode=none);
        @statistic[activeInterface](title="Active Interface"; source=activeInterface; record=vector; interpolationmode=none);
        @statistic[handoversDetected](title="Handovers Detected"; source=handoverDetected; record=count,sum,vector; interpolationmode=none);
        
   
}

// Simplified VoIP receiver with handover support
// Extends CustomVoipReceiver but directly implements IApp
module VoIPReceiverWithHandover extends CustomVoipReceiver like IApp
{
    parameters:
        @class(VoIPReceiverWithHandover);
        @display("i=block/app2");
        
        // Connection parameters
        string localAddress = default("");
        
        
        // Session continuity parameters
        bool enableSessionContinuity = default(true);
        double jitterBufferSize @unit(s) = default(150ms);
        double handoverTimeout @unit(s) = default(2s);
        
        // Adaptive jitter buffer parameters
        bool enableAdaptiveJitterBuffer = default(true);
        double maxJitterBufferSize @unit(s) = default(300ms);
        double minJitterBufferSize @unit(s) = default(50ms);
        double jitterBufferAdaptationRate = default(0.1);
        
        // Packet reordering parameters
        int maxReorderingDelay = default(10);
        double reorderingTimeout @unit(s) = default(100ms);
        
        // Audio playback parameters
        double maxPlayoutDelay @unit(s) = default(300ms);
        double minPlayoutDelay @unit(s) = default(20ms);
        
        // Voice activity detection
        bool enableVAD = default(false);
        double silenceThreshold = default(0.1);
        
        // QoS parameters
        int dscp = default(46);
        int tos = default(-1);
        
        // Statistics signals and collection
        @signal[packetReorder](type=long);
        @signal[duplicatePacket](type=long);
        @signal[jitterMeasurement](type=simtime_t);
        @signal[endToEndDelay](type=simtime_t);
        @signal[jitterBufferUnderrun](type=long);
        @signal[jitterBufferOverrun](type=long);
        @signal[handoverInterruption](type=simtime_t);
        @signal[qualityDegradation](type=double);
        
        @statistic[packetsReordered](title="Packets Reordered"; source=packetReorder; record=count,sum,vector; interpolationmode=none);
        @statistic[duplicatePackets](title="Duplicate Packets"; source=duplicatePacket; record=count,sum,vector; interpolationmode=none);
        @statistic[jitter](title="Jitter"; source=jitterMeasurement; record=stats,histogram,vector; interpolationmode=none);
        @statistic[endToEndDelay](title="End-to-End Delay"; source=endToEndDelay; record=stats,histogram,vector; interpolationmode=none);
        @statistic[jitterBufferUnderruns](title="Jitter Buffer Underruns"; source=jitterBufferUnderrun; record=count,sum,vector; interpolationmode=none);
        @statistic[jitterBufferOverruns](title="Jitter Buffer Overruns"; source=jitterBufferOverrun; record=count,sum,vector; interpolationmode=none);
        @statistic[handoverInterruptions](title="Handover Interruptions"; source=handoverInterruption; record=stats,histogram,vector; interpolationmode=none);
        @statistic[qualityDegradation](title="Quality Degradation"; source=qualityDegradation; record=stats,histogram,vector; interpolationmode=none);
        
    

}