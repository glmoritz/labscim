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

#ifndef __LABSCIM_STATEBASEDEPENERGYCONSUMER_H
#define __LABSCIM_STATEBASEDEPENERGYCONSUMER_H

#include "inet/physicallayer/wireless/common/energyconsumer/StateBasedEpEnergyConsumer.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h"
#include "inet/power/contract/IEpEnergyConsumer.h"
#include "inet/power/contract/IEpEnergySource.h"

using namespace inet::physicallayer;
using namespace inet::power;

namespace labscim {

namespace physicallayer {

/**
 * This consumer model extends the baseline to consider the transmission power in the transmission consumption power
 *
 * @author Guilherme Luiz Moritz
 */
class INET_API LabSCimStateBasedEpEnergyConsumer : public StateBasedEpEnergyConsumer
{
  protected:
    // parameters
    double transmitterTransmittingPowerConsumptionIncreasePerWatt = NaN;
    double transmitterTransmittingPreamblePowerConsumptionIncreasePerWatt = NaN;
    double transmitterTransmittingHeaderPowerConsumptionIncreasePerWatt = NaN;
    double transmitterTransmittingDataPowerConsumptionIncreasePerWatt = NaN;

  protected:
    virtual W InterpolatePower(W PowerAt0, W Power, double IncreasePerWatt) const;
    virtual W computePowerConsumption() const override;

  public:
    void initialize(int stage) override;

};

} // namespace physicallayer

} // namespace labscim

#endif // ifndef __LABSCIM_STATEBASEDEPENERGYCONSUMER_H

