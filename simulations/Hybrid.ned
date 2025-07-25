
package abcd.simulations;

import inet.applications.dhcp.DhcpServer;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import inet.node.internetcloud.InternetCloud;
import inet.node.wireless.AccessPoint;
import inet.physicallayer.wireless.common.medium.RadioMedium;
import ned.IdealChannel;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.PgwStandard;
import simu5g.nodes.Ue;
import abcd.nodes.DualUe;
import simu5g.nodes.eNodeB;
import inet.node.ethernet.Eth10G;
import simu5g.world.radio.ChannelControl;
import simu5g.world.radio.LteChannelControl;
import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;
import inet.networklayer.common.NetworkInterface;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.ipv4.Ipv4RoutingTable;
import inet.routing.ospfv2.Ospfv2;
import inet.node.ospfv2.OspfRouter;





//
// This is A Hybrid Network (Wi-Fi & LTE) for Graduation Project Implemented by Al Muzzamil Mohammed
//
network Hybrid
{
    parameters:
        int numUe = default(1);
        @display("bgb=1746.96,999.98395");
         // Handover parameters
        string handoverAlgorithm @enum("rssi","load","hybrid") = default("rssi");
        double handoverHysteresis = default(3); // dB
        double handoverMargin = default(2); // dB


    submodules:
        visualizer: IntegratedCanvasVisualizer {
            @display("p=38.615387,670.3631");
        }


        channelControl: LteChannelControl {
            @display("p=50,25;is=s");
        }
        routingRecorder: RoutingTableRecorder {
            @display("p=50,75;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=50,125");
        }

        binder: Binder {
            @display("p=50,175;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=50.993748,258.7;is=s");
        }
        server: StandardHost {
            @display("p=311.2981,60.576927;is=n;i=device/server");
        }
        router2: Router {
            @display("p=881.73083,270.91348;i=device/smallrouter");
        }
        pgw: PgwStandard {
            @display("p=1364.6636,257.45193;is=l");
        }
        eNB: eNodeB {
            @display("p=1549.7598,528.3654;is=vl");
        }
        ue[numUe]: DualUe {
            @display("p=535.0962,721.87506");
        }
        internetCloud: InternetCloud {
            @display("p=668.0289,60.576927");
        }
        router1: Router {
            @display("p=447.5962,284.37503;i=device/smallrouter");
        }
        accessPoint: AccessPoint {
            @display("p=302.88464,585.57697");
        }
        radioMedium: RadioMedium {
            @display("p=40.16,335.336");
        }
        router3: Router {
            @display("p=496.39426,58.894234;i=device/smallrouter");
        }
    connections:
        router1.ethg++ <--> Eth10G <--> accessPoint.ethg++;
        router2.pppg++ <--> Eth10G <--> pgw.filterGate;
        pgw.pppg++ <--> Eth10G <--> eNB.ppp;
        internetCloud.pppg++ <--> Eth10G <--> router3.pppg++;
        router3.pppg++ <--> Eth10G <--> server.pppg++;
        router1.pppg++ <--> Eth10G <--> internetCloud.pppg++;
        router2.pppg++ <--> Eth10G <--> internetCloud.pppg++;
}
