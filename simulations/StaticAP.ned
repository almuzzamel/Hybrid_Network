package abcd.simulations;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.ethernet.Eth10G;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import inet.node.inet.WirelessHost;
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




network StaticAP
{
    parameters:
        int numUe = default(1);
        @display("bgb=1200,700");

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
            @display("p=64.715,245.315");
        }

        // Server
        server: StandardHost {
            @display("p=200,150;is=n;i=device/server");
        }

        // WiFi Infrastructure
        wifiRouter: Router {
            @display("p=517.72,124.915;i=device/smallrouter");
        }

        accessPoint: AccessPoint {
            @display("p=663.705,296.485;i=device/accesspoint");
        }

        // WiFi Clients (using standard WirelessHost instead of DualUe)
        client[numUe]: WirelessHost {
            @display("p=500,500");
       }
        

    connections:
        server.ethg++ <--> Eth10G <--> wifiRouter.ethg++;
        wifiRouter.ethg++ <--> Eth10G <--> accessPoint.ethg++;
}
