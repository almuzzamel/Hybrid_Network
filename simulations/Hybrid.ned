//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

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
            @display("p=1036.5386,255.76926;i=device/smallrouter");
        }
        pgw: PgwStandard {
            @display("p=1364.6636,257.45193;is=l");
        }
        eNB: eNodeB {
            @display("p=1549.7598,528.3654;is=vl");
        }
        ue[numUe]: DualUe {
            @display("p=466.1058,735.3366");
        }
        internetCloud: InternetCloud {
            @display("p=668.0289,60.576927");
        }
        router1: Router {
            @display("p=311.2981,255.76926;i=device/smallrouter");
        }
        accessPoint: AccessPoint {
            @display("p=302.88464,585.57697");
        }
        radioMedium: RadioMedium {
            @display("p=40.16,335.336");
        }
        router0: Router {
            @display("p=698.3174,255.76926;i=device/smallrouter");
        }
        
    connections:
        server.pppg++ <--> Eth10G <--> internetCloud.pppg++;
        internetCloud.pppg++ <--> Eth10G <--> router0.pppg++;
        router0.ethg++ <--> Eth10G <--> router2.ethg++;
        router0.ethg++ <--> Eth10G <--> router1.ethg++;
        router1.ethg++ <--> Eth10G <--> accessPoint.ethg++;
        router2.pppg++ <--> Eth10G <--> pgw.filterGate;
        pgw.pppg++ <--> Eth10G <--> eNB.ppp;
}
