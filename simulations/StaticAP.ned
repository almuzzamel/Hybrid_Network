package abcd.simulations;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.ethernet.Eth10G;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import inet.node.inet.WirelessHost;
import inet.node.internetcloud.InternetCloud;
import inet.node.wireless.AccessPoint;
import abcd.nodes.DualUe;

import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;
import inet.visualizer.canvas.mobility.MobilityCanvasVisualizer;
import inet.visualizer.canvas.flow.PacketFlowCanvasVisualizer;
import inet.visualizer.canvas.networklayer.NetworkRouteCanvasVisualizer;
import inet.visualizer.canvas.networklayer.RoutingTableCanvasVisualizer;

import inet.networklayer.diffserv.DiffservQueue;
import ned.IdealChannel;
import ned.DatarateChannel;



network StaticAP
{
    parameters:
        int numUe = default(1);
        @display("bgb=1200,700");
        
         types:
        channel C extends DatarateChannel
        {
            delay = 2ms;
            datarate = 1Gbps;
        }

    submodules:
        visualizer: IntegratedCanvasVisualizer {
            @display("p=50,150");

        }
        // WiFi Radio Medium
        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=50,50;is=s");
        }


        routingRecorder: RoutingTableRecorder {
            @display("p=50,75;is=s");
        }

        configurator: Ipv4NetworkConfigurator {
            @display("p=108.36,459.025");
        }

        // Server
        server: StandardHost {
            @display("p=153.51,346.15;is=n;i=device/server");
        }

        // WiFi Infrastructure
        wifiRouter: Router {
            @display("p=611.02997,158.025;i=device/smallrouter");
        }

        accessPoint: AccessPoint {
            @display("p=717.885,331.1;i=device/accesspoint");
        }

        // WiFi Clients (using standard WirelessHost instead of DualUe)
        client[numUe]: WirelessHost {
            @display("p=500,500");
        }


        internetCloud: InternetCloud {
            @display("p=462.035,58.695");
        }
        // WiFi Infrastructure
        serverRouter: Router {
            @display("p=293.475,147.48999;i=device/smallrouter");
        }

    connections:
        wifiRouter.pppg++ <--> Eth10G <--> internetCloud.pppg++;
        wifiRouter.ethg++ <--> Eth10G <--> accessPoint.ethg++;
       serverRouter.pppg++  <--> Eth10G <--> internetCloud.pppg++;
        serverRouter.pppg++ <--> Eth10G <--> server.pppg++;
}
