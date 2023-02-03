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

#ifndef __LABSCIM_LORADIMENSIONALFHSSSNIR_H
#define __LABSCIM_LORADIMENSIONALFHSSSNIR_H

#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/DimensionalNoise.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/DimensionalReception.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/DimensionalSnir.h"
#include "LoRaDimensionalReception.h"
#include "LoRaDimensionalNoise.h"

using namespace inet;
using namespace inet::physicallayer;

namespace labscim {

namespace physicallayer {

class INET_API LoRaDimensionalFHSSSnir : public DimensionalSnir
{
  protected:
    mutable std::vector<double> minHopSNR;
    mutable std::vector<double> maxHopSNR;
    mutable std::vector<double> meanHopSNR;

    mutable int32_t numHeaders;

  protected:
    virtual void computeHopMin(uint32_t numhops) const;
    virtual void computeHopMax(uint32_t numhops) const;
    virtual void computeHopMean(uint32_t numhops) const;

  public:
    LoRaDimensionalFHSSSnir(const DimensionalReception *reception, const DimensionalNoise *noise);

    virtual std::ostream& printToStream(std::ostream& stream, int level, int evFlags = 0) const override;

    virtual uint32_t getHeadersWithMinBelowThresold(double Threshold) const;
    virtual uint32_t getHeadersWithMaxBelowThresold(double Threshold) const;
    virtual uint32_t getHeadersWithMeanBelowThresold(double Threshold) const;
    virtual int32_t getNumHeaders() const {return numHeaders;};

    virtual uint32_t getHopsWithMinBelowThresold(double Threshold) const;
    virtual uint32_t getHopsWithMaxBelowThresold(double Threshold) const;
    virtual uint32_t getHopsWithMeanBelowThresold(double Threshold) const;
    virtual uint32_t getNumHops() const;
};

} // namespace physicallayer

} // namespace labscim

#endif // ifndef __LABSCIM_LORADIMENSIONALSNIR_H

