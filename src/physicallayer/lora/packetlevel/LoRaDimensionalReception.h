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

#ifndef __LABSCIM_LORADIMENSIONALRECEPTION_H
#define __LABSCIM_LORADIMENSIONALRECEPTION_H

// INET 4.7 port: the old DimensionalReception (a Reception that carried the power
// function) was removed by INET's analog-model separation refactor. A reception is now a
// generic Reception composing an IReceptionAnalogModel (concretely a
// DimensionalReceptionAnalogModel, which holds power/centerFrequency/bandwidth). We keep
// LoRaDimensionalReception as a Reception subclass carrying the LoRa-specific SF/CR and
// expose getPower()/getCenterFrequency()/getBandwidth() convenience accessors that delegate
// to the composed analog model, so LoRaDimensionalAnalogModel's interference math is unchanged.
#include "inet/common/math/Functions.h"
#include "inet/physicallayer/wireless/common/analogmodel/dimensional/DimensionalSignalAnalogModel.h"
#include "inet/physicallayer/wireless/common/radio/packetlevel/Reception.h"
#include "inet/common/Units.h"

using namespace inet;
using namespace inet::physicallayer;
using namespace inet::math;

namespace labscim {

namespace physicallayer {


class INET_API LoRaDimensionalReception : public Reception
{
  protected:
    const int LoRaSF;
    const double LoRaCR;

  public:
    LoRaDimensionalReception(const IRadio *radio, const ITransmission *transmission, const simtime_t startTime, const simtime_t endTime, const Coord startPosition, const Coord endPosition, const Quaternion startOrientation, const Quaternion endOrientation, const IReceptionAnalogModel *analogModel, int LoRaSF, int LoRaCR);
    int getLoRaSF() const { return LoRaSF; }
    double getLoRaCR() const { return LoRaCR; }

    // convenience accessors delegating to the composed dimensional analog model (INET 4.7)
    const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& getPower() const { return check_and_cast<const DimensionalSignalAnalogModel *>(getAnalogModel())->getPower(); }
    Hz getCenterFrequency() const { return check_and_cast<const DimensionalSignalAnalogModel *>(getAnalogModel())->getCenterFrequency(); }
    Hz getBandwidth() const { return check_and_cast<const DimensionalSignalAnalogModel *>(getAnalogModel())->getBandwidth(); }
    W computeMinPower(simtime_t startTime, simtime_t endTime) const { return check_and_cast<const DimensionalSignalAnalogModel *>(getAnalogModel())->computeMinPower(startTime, endTime); }
};

} // namespace physicallayer

} // namespace labscim

#endif // ifndef __LABSCIM_LORADIMENSIONALRECEPTION_H
