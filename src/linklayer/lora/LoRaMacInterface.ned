//
// Copyright (C) 2014 Florian Meier
// Copyright (C) 2013 OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

//package labscim.linklayer.ContikiNG_IEEE802154;1

import inet.linklayer.contract.IWirelessInterface;
import inet.networklayer.common.NetworkInterface;
import inet.physicallayer.wireless.common.contract.packetlevel.IRadio;
import labscim.linklayer.lora.LoRaMacNodeGlueMac;
package labscim.linklayer.lora;


module LoRaMacInterface extends NetworkInterface like IWirelessInterface
{
    parameters:
        @class(inet::NetworkInterface);
        string interfaceTableModule;
        string energySourceModule = default("^.energyStorage");
        *.interfaceTableModule = default(absPath(this.interfaceTableModule));
        *.energySourceModule = default(absPath(this.energySourceModule));
        @display("i=block/ifcard");
    gates:
        input upperLayerIn;
        output upperLayerOut;
        input radioIn @labels(ISignal);
    submodules:
        mac: LoRaMacNodeGlueMac {
            parameters:
                @display("p=100,100");
        }
        radio: <default("LoRaDimensionalRadio")> like IRadio {
            parameters:
                @display("p=100,200");
        }
    connections:
        upperLayerIn --> mac.upperLayerIn;
        mac.lowerLayerOut --> radio.upperLayerIn;
        mac.upperLayerOut --> { @display("m=n"); } --> upperLayerOut;
        radioIn --> { @display("m=s"); } --> radio.radioIn;
        radio.upperLayerOut --> mac.lowerLayerIn;
}

