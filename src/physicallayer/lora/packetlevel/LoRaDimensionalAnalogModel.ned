//
// Copyright (C) 2013 OpenSim Ltd
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

// INET 4.7 port: the single IAnalogModel interface was split into IMediumAnalogModel /
// IReceiverAnalogModel / ITransmitterAnalogModel. The medium's DimensionalAnalogModel became
// DimensionalMediumAnalogModel; the C++ class extends it, so extend it here too and inherit
// its attenuateWithCenterFrequency parameter.
import inet.physicallayer.wireless.common.analogmodel.dimensional.DimensionalMediumAnalogModel;

//
// This analog model computes with dimensional analog power representation. In
// other words the signal power may change over time and/or frequency.
//
module LoRaDimensionalAnalogModel extends DimensionalMediumAnalogModel
{
    parameters:
        @display("i=block/tunnel");
        @class(labscim::physicallayer::LoRaDimensionalAnalogModel);
}

