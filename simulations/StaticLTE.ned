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

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import inet.node.ethernet.Eth10M;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import abcd.nodes.DualUe;
import simu5g.nodes.Ue;
import simu5g.nodes.eNodeB;
import simu5g.nodes.PgwStandard;
import simu5g.world.radio.LteChannelControl;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;
import inet.visualizer.canvas.mobility.MobilityCanvasVisualizer;
import inet.visualizer.canvas.flow.PacketFlowCanvasVisualizer;
import inet.visualizer.canvas.networklayer.NetworkRouteCanvasVisualizer;
import inet.visualizer.canvas.networklayer.RoutingTableCanvasVisualizer;



network StaticLTE
{
    parameters:
        int numUe = default(1);
        @display("i=block/network2;bgb=991,558");
    submodules:
        channelControl: LteChannelControl {
            @display("p=50,25;is=s");
        }
        visualizer: IntegratedCanvasVisualizer {
            @display("p=50,150");

        }
        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=50,350;is=s");
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
            numComponentCarriers = default(2);
        }
        server: StandardHost {
            @display("p=212,118;is=n;i=device/server");
        }
        router: Router {
            @display("p=321,136;i=device/smallrouter");
        }
        pgw: PgwStandard {
            @display("p=519,175;is=l");
        }
        eNB: eNodeB {
            @display("p=391,259;is=vl");
        }
        ue[numUe]: Ue {
            @display("p=885.55,324.61874");
        }
    connections:
        server.pppg++ <--> Eth10G <--> router.pppg++;
        router.pppg++ <--> Eth10G <--> pgw.filterGate;
        pgw.pppg++ <--> Eth10G <--> eNB.ppp;
}

