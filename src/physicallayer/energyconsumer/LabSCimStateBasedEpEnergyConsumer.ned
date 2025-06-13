//
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

package labscim.physicallayer.energyconsumer;

import inet.physicallayer.wireless.common.energyconsumer.StateBasedEpEnergyConsumer;

//
// This module provides a radio power consumer model. The power consumption is
// determined by the radio mode, the transmitter state and the receiver state
// using constant parameters. Transmission consumption increases linearly with transmission power
//
simple LabSCimStateBasedEpEnergyConsumer extends StateBasedEpEnergyConsumer
{
    parameters:        
        double transmitterTransmittingPowerConsumptionIncreasePerWatt = default(1);        
        double transmitterTransmittingPreamblePowerConsumptionIncreasePerWatt = default(transmitterTransmittingPowerConsumptionIncreasePerWatt);
        double transmitterTransmittingHeaderPowerConsumptionIncreasePerWatt = default(transmitterTransmittingPowerConsumptionIncreasePerWatt);
        double transmitterTransmittingDataPowerConsumptionIncreasePerWatt = default(transmitterTransmittingPowerConsumptionIncreasePerWatt);
        @display("i=block/cogwheel");
        @class(labscim::physicallayer::LabSCimStateBasedEpEnergyConsumer);
}

