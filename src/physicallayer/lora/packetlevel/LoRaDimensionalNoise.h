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

#ifndef __LABSCIM_LORADIMENSIONALNOISE_H
#define __LABSCIM_LORADIMENSIONALNOISE_H

#include "inet/common/math/Functions.h"
#include "inet/physicallayer/base/packetlevel/NarrowbandNoiseBase.h"
#include "inet/physicallayer/analogmodel/packetlevel/DimensionalNoise.h"

using namespace inet;
using namespace inet::physicallayer;
using namespace inet::math;
using namespace std;


namespace labscim {

namespace physicallayer {

class INET_API LoRaDimensionalNoise : public DimensionalNoise
{
  protected:
    const std::array<const bool,6> LoRaInterfererPresent;
    const bool NonLoRaInterfererPresent;
    const std::array<const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>,6> LoRapower;
    const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>> NonLoRapower;
    const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>> Backgroundpower;
    const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>> totalpower;

  public:
    LoRaDimensionalNoise(simtime_t startTime, simtime_t endTime, Hz centerFrequency, Hz bandwidth, const std::array<const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>,6> LoRapower, const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& NonLoRapower, const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& Backgroundpower, const std::array<const bool, 6> LoRaInterfererPresent, const bool NonLoRaInterfererPresent);


    virtual std::ostream& printToStream(std::ostream& stream, int level) const override;
    virtual const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& getNonLoRapower() const { return NonLoRapower; }
    virtual const std::array<const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>,6>& getLoRapower() const { return LoRapower; }
    virtual const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& getLoRapower(int LoRaSF) const { return LoRapower.at(LoRaSF - 7); }
    virtual bool getLoRaInterfererPresent(int LoRaSF) const { return LoRaInterfererPresent.at(LoRaSF - 7); }
    virtual bool getNonLoRaInterfererPresent() const { return NonLoRaInterfererPresent; }

    virtual const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& getBackgroundpower() const { return Backgroundpower; }

    virtual const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>> ComputeTotalNoise(const std::array<const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>,6>& LoRapower, const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& NonLoRapower, const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& Backgroundpower);



    virtual W computeNonLoRaMinPower(simtime_t startTime, simtime_t endTime) const ;
    virtual W computeNonLoRaMaxPower(simtime_t startTime, simtime_t endTime) const ;

    virtual W computeLoRaMinPower(int LoRaSF, simtime_t startTime, simtime_t endTime) const ;
    virtual W computeLoRaMaxPower(int LoRaSF, simtime_t startTime, simtime_t endTime) const ;

  private:
    W retMinPower(simtime_t startTime, simtime_t endTime, const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>> p) const;
    W retMaxPower(simtime_t startTime, simtime_t endTime, const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>> p) const;

};

} // namespace physicallayer

} // namespace labscim

#endif // ifndef __LABSCIM_LORADIMENSIONALNOISE_H

