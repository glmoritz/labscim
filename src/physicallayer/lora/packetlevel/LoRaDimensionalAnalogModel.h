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

#ifndef __LABSCIM_LORADIMENSIONALANALOGMODEL_H
#define __LABSCIM_LORADIMENSIONALANALOGMODEL_H

#include "inet/physicallayer/analogmodel/packetlevel/DimensionalAnalogModel.h"

using namespace inet::physicallayer;

namespace labscim {

namespace physicallayer {

class INET_API LoRaDimensionalAnalogModel : public DimensionalAnalogModel
{
  public:
    virtual std::ostream& printToStream(std::ostream& stream, int level) const override;

    virtual const IReception *computeReception(const IRadio *radio, const ITransmission *transmission, const IArrival *arrival) const override;

    const INoise *computeNoise(const IListening *listening, const IInterference *interference) const override;
    const INoise *computeNoise(const IReception *reception, const INoise *noise) const override;
    const ISnir *computeSNIR(const IReception *reception, const INoise *noise) const override;
};

} // namespace physicallayer

} // namespace labscim

#endif // ifndef __LABSCIM_LORADIMENSIONALANALOGMODEL_H

