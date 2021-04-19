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

package labscim.physicallayer.lora.packetlevel;

import inet.physicallayer.base.packetlevel.NarrowbandReceiverBase;

module LoRaDimensionalReceiver extends NarrowbandReceiverBase
{
    parameters:
        int LoRaSF;
        int LoRaCR;
        bool iAmGateway;        
        LoRaSF = default(7);
        iAmGateway = default(false);
        LoRaCR = default(1); //coding rate at 4 / (4+LoRaCR)  
        
        
        modulation = default("LoRa-CSS");  
        //TODO: implement an error model using something like doi: 10.1109/LCOMM.2018.2849718 or doi: 10.1109/TWC.2019.2952584      
        //for now we just set it to null, and the packet or arrives without error or does not arrive at all
        errorModel.typename = default("");   
        double minInterferencePower @unit(dBm);
        double snirNonLoRaThreshold @unit(dB);
        @class(labscim::physicallayer::LoRaDimensionalReceiver);
}

