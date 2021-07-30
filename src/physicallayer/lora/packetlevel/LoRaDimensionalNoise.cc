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

#include "inet/physicallayer/analogmodel/packetlevel/DimensionalNoise.h"
#include "LoRaDimensionalNoise.h"

namespace labscim {

namespace physicallayer {



LoRaDimensionalNoise::LoRaDimensionalNoise(simtime_t startTime, simtime_t endTime, Hz centerFrequency, Hz bandwidth, const std::array<const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>,6> LoRapower, const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& NonLoRapower, const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& Backgroundpower, const std::array<const bool, 6> LoRaInterfererPresent, const bool NonLoRaInterfererPresent):
    DimensionalNoise(startTime, endTime, centerFrequency, bandwidth, ComputeTotalNoise(LoRapower,NonLoRapower,Backgroundpower)),
    LoRapower(LoRapower),
    NonLoRapower(NonLoRapower),
    Backgroundpower(Backgroundpower),
    LoRaInterfererPresent(LoRaInterfererPresent),
    NonLoRaInterfererPresent(NonLoRaInterfererPresent)
{
}

const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>> LoRaDimensionalNoise::ComputeTotalNoise(const std::array<const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>,6>& LoRapower, const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& NonLoRapower, const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& Backgroundpower)
{
    std::vector<Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>> Powers(LoRapower.begin(), LoRapower.end());
    Powers.push_back(NonLoRapower);
    Powers.push_back(Backgroundpower);
    return makeShared<SummedFunction<WpHz, Domain<simsec, Hz>>>(Powers);
}

std::ostream& LoRaDimensionalNoise::printToStream(std::ostream& stream, int level) const
{
    stream << "LoRaDimensionalNoise";
//too lazy to print all the max and min powers
//    if (level <= PRINT_LEVEL_DEBUG)
//        stream << ", powerMax = " << power->getMax()
//               << ", powerMin = " << power->getMin();
//    if (level <= PRINT_LEVEL_TRACE)
//        stream << ", power = " << power;
    return DimensionalNoise::printToStream(stream, level);
}



W LoRaDimensionalNoise::retMinPower(simtime_t startTime, simtime_t endTime, const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>> p) const
{
    Point<simsec> startPoint{simsec(startTime)};
    Point<simsec> endPoint{simsec(endTime)};
    W minPower = integrate<WpHz, Domain<simsec, Hz>, 0b10, W, Domain<simsec>>(p->add(Backgroundpower))->getMin(Interval<simsec>(startPoint, endPoint, 0b1, 0b1, 0b0));
    EV_DEBUG << "Computing minimum noise power: start = " << startPoint << ", end = " << endPoint << " -> " << minPower << endl;
    return minPower;
}

W LoRaDimensionalNoise::retMaxPower(simtime_t startTime, simtime_t endTime, const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>> p) const
{
    Point<simsec> startPoint{simsec(startTime)};
    Point<simsec> endPoint{simsec(endTime)};
    W maxPower = integrate<WpHz, Domain<simsec, Hz>, 0b10, W, Domain<simsec>>(p->add(Backgroundpower))->getMax(Interval<simsec>(startPoint, endPoint, 0b1, 0b1, 0b0));
    EV_DEBUG << "Computing maximum noise power: start = " << startPoint << ", end = " << endPoint << " -> " << maxPower << endl;
    return maxPower;
}

W LoRaDimensionalNoise::computeNonLoRaMinPower(simtime_t startTime, simtime_t endTime) const
{
    return retMinPower(startTime, endTime, NonLoRapower);
}

W LoRaDimensionalNoise::computeNonLoRaMaxPower(simtime_t startTime, simtime_t endTime) const
{
    return retMaxPower(startTime, endTime, NonLoRapower);
}

W LoRaDimensionalNoise::computeLoRaMinPower(int LoRaSF, simtime_t startTime, simtime_t endTime) const
{
    return retMinPower(startTime, endTime, LoRapower[LoRaSF - 7]);
}

W LoRaDimensionalNoise::computeLoRaMaxPower(int LoRaSF, simtime_t startTime, simtime_t endTime) const
{
    return retMaxPower(startTime, endTime, LoRapower[LoRaSF - 7]);
}


} // namespace physicallayer

} // namespace labscim

