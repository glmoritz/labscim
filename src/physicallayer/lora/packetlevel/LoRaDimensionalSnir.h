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

#ifndef __LABSCIM_LORADIMENSIONALSNIR_H
#define __LABSCIM_LORADIMENSIONALSNIR_H

#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/DimensionalNoise.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/DimensionalReception.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/DimensionalSnir.h"
#include "LoRaDimensionalReception.h"
#include "LoRaDimensionalNoise.h"

using namespace inet;
using namespace inet::physicallayer;

namespace labscim {

namespace physicallayer {

class INET_API LoRaDimensionalSnir : public DimensionalSnir
{
  protected:
    mutable double minLoRaSNIR[6];
    mutable double maxLoRaSNIR[6];
    mutable double meanLoRaSNIR[6];

    mutable double minNonLoRaSNIR;
    mutable double maxNonLoRaSNIR;
    mutable double meanNonLoRaSNIR;


  protected:
    virtual double retMin(const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& noisepower) const;
    virtual double retMax(const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& noisepower) const;
    virtual double retMean(const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& noisepower) const;

  public:
    LoRaDimensionalSnir(const LoRaDimensionalReception *reception, const LoRaDimensionalNoise *noise);

    virtual std::ostream& printToStream(std::ostream& stream, int level, int evFlags = 0) const override;

    virtual double getMinLoRa(int LoRaSF) const;
    virtual double getMaxLoRa(int LoRaSF) const;
    virtual double getMeanLoRa(int LoRaSF) const;

    virtual double getMinNonLoRa() const;
    virtual double getMaxNonLoRa() const;
    virtual double getMeanNonLoRa() const;

    virtual bool getLoRaInterfererPresent(int LoRaSF) const;
    virtual bool getNonLoRaInterfererPresent() const;

};

} // namespace physicallayer

} // namespace labscim

#endif // ifndef __LABSCIM_LORADIMENSIONALSNIR_H

