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

#include "inet/physicallayer/analogmodel/packetlevel/DimensionalSnir.h"
#include "LoRaDimensionalSnir.h"
#include "LoRaDimensionalReception.h"
#include "LoRaDimensionalNoise.h"

using namespace inet;
using namespace inet::physicallayer;

namespace labscim {

namespace physicallayer {


LoRaDimensionalSnir::LoRaDimensionalSnir(const LoRaDimensionalReception *reception, const LoRaDimensionalNoise *noise) :
    DimensionalSnir(reception, noise),
    minLoRaSNIR  {NaN,NaN,NaN,NaN,NaN,NaN},
    maxLoRaSNIR  {NaN,NaN,NaN,NaN,NaN,NaN},
    meanLoRaSNIR {NaN,NaN,NaN,NaN,NaN,NaN},
    minNonLoRaSNIR(NaN),
    maxNonLoRaSNIR(NaN),
    meanNonLoRaSNIR(NaN)
{

}

std::ostream& LoRaDimensionalSnir::printToStream(std::ostream& stream, int level) const
{
    stream << "LoRaDimensionalSnir";
//    if (level <= PRINT_LEVEL_DETAIL)
//        stream << ", minSNIR = " << minSNIR;
    return DimensionalSnir::printToStream(stream, level);
}


double LoRaDimensionalSnir::retMin(const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& noisepower) const
{
    const DimensionalReception *dimensionalReception = check_and_cast<const DimensionalReception *>(reception);
    EV_TRACE << "Reception power begin " << endl;
    EV_TRACE << *dimensionalReception->getPower() << endl;
    EV_TRACE << "Reception power end" << endl;
    auto receptionPower = dimensionalReception->getPower();
    auto snir = receptionPower->divide(noisepower);
    simsec startTime = simsec(reception->getStartTime());
    simsec endTime = simsec(reception->getEndTime());
    Hz centerFrequency = dimensionalReception->getCenterFrequency();
    Hz bandwidth = dimensionalReception->getBandwidth();
    Point<simsec, Hz> startPoint(startTime, centerFrequency - bandwidth / 2);
    Point<simsec, Hz> endPoint(endTime, centerFrequency + bandwidth / 2);
    EV_TRACE << "SNIR begin " << endl;
    EV_TRACE << *snir << endl;
    EV_TRACE << "SNIR end" << endl;
    double minSNIR = snir->getMin(Interval<simsec, Hz>(startPoint, endPoint, 0b1, 0b0, 0b0));
    EV_DEBUG << "Computing minimum SNIR: start = " << startPoint << ", end = " << endPoint << " -> minimum SNIR = " << math::fraction2dB(minSNIR) << " dB" << endl;
    return minSNIR;
}

double LoRaDimensionalSnir::retMax(const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& noisepower) const
{
    const DimensionalReception *dimensionalReception = check_and_cast<const DimensionalReception *>(reception);
    EV_TRACE << "Reception power begin " << endl;
    EV_TRACE << *dimensionalReception->getPower() << endl;
    EV_TRACE << "Reception power end" << endl;
    auto receptionPower = dimensionalReception->getPower();
    auto snir = receptionPower->divide(noisepower);
    simsec startTime = simsec(reception->getStartTime());
    simsec endTime = simsec(reception->getEndTime());
    Hz centerFrequency = dimensionalReception->getCenterFrequency();
    Hz bandwidth = dimensionalReception->getBandwidth();
    Point<simsec, Hz> startPoint(startTime, centerFrequency - bandwidth / 2);
    Point<simsec, Hz> endPoint(endTime, centerFrequency + bandwidth / 2);
    EV_TRACE << "SNIR begin " << endl;
    EV_TRACE << *snir << endl;
    EV_TRACE << "SNIR end" << endl;
    double maxSNIR = snir->getMax(Interval<simsec, Hz>(startPoint, endPoint, 0b1, 0b0, 0b0));
    EV_DEBUG << "Computing maximum SNIR: start = " << startPoint << ", end = " << endPoint << " -> maximum SNIR = " << math::fraction2dB(maxSNIR) << " dB" << endl;
    return maxSNIR;
}

double LoRaDimensionalSnir::retMean(const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& noisepower) const
{
    const DimensionalReception *dimensionalReception = check_and_cast<const DimensionalReception *>(reception);
    EV_TRACE << "Reception power begin " << endl;
    EV_TRACE << *dimensionalReception->getPower() << endl;
    EV_TRACE << "Reception power end" << endl;
    auto receptionPower = dimensionalReception->getPower();
    auto snir = receptionPower->divide(noisepower);
    simsec startTime = simsec(reception->getStartTime());
    simsec endTime = simsec(reception->getEndTime());
    Hz centerFrequency = dimensionalReception->getCenterFrequency();
    Hz bandwidth = dimensionalReception->getBandwidth();
    Point<simsec, Hz> startPoint(startTime, centerFrequency - bandwidth / 2);
    Point<simsec, Hz> endPoint(endTime, centerFrequency + bandwidth / 2);
    EV_TRACE << "SNIR begin " << endl;
    EV_TRACE << *snir << endl;
    EV_TRACE << "SNIR end" << endl;
    double meanSNIR = snir->getMean(Interval<simsec, Hz>(startPoint, endPoint, 0b1, 0b0, 0b0));
    EV_DEBUG << "Computing mean SNIR: start = " << startPoint << ", end = " << endPoint << " -> mean SNIR = " << math::fraction2dB(meanSNIR) << " dB" << endl;
    return meanSNIR;
}

double LoRaDimensionalSnir::getMinLoRa(int LoRaSF) const
{
    if(LoRaSF < 7 || LoRaSF > 12)
    {
        return NaN;
    }
    if (std::isnan(minLoRaSNIR[LoRaSF-7]))
    {
        const LoRaDimensionalNoise *dimensionalNoise = dynamic_cast<const LoRaDimensionalNoise *>(noise);
        if(dimensionalNoise)
        {
            auto noisePower = dimensionalNoise->getLoRapower(LoRaSF);
            minLoRaSNIR[LoRaSF-7] = retMin(noisePower->add(dimensionalNoise->getBackgroundpower()));
        }
    }
    return minLoRaSNIR[LoRaSF-7];
}


double LoRaDimensionalSnir::getMaxLoRa(int LoRaSF) const
{
    if(LoRaSF < 7 || LoRaSF > 12)
    {
        return NaN;
    }
    if (std::isnan(maxLoRaSNIR[LoRaSF-7]))
    {
        const LoRaDimensionalNoise *dimensionalNoise = dynamic_cast<const LoRaDimensionalNoise *>(noise);
        if(dimensionalNoise)
        {
            auto noisePower = dimensionalNoise->getLoRapower(LoRaSF);
            maxLoRaSNIR[LoRaSF-7] = retMax(noisePower->add(dimensionalNoise->getBackgroundpower()));
        }
    }
    return maxLoRaSNIR[LoRaSF-7];
}


double LoRaDimensionalSnir::getMeanLoRa(int LoRaSF) const
{
    if(LoRaSF < 7 || LoRaSF > 12)
    {
        return NaN;
    }
    if (std::isnan(meanLoRaSNIR[LoRaSF-7]))
    {
        const LoRaDimensionalNoise *dimensionalNoise = dynamic_cast<const LoRaDimensionalNoise *>(noise);
        if(dimensionalNoise)
        {
            auto noisePower = dimensionalNoise->getLoRapower(LoRaSF);
            meanLoRaSNIR[LoRaSF-7] = retMean(noisePower->add(dimensionalNoise->getBackgroundpower()));
        }
    }
    return meanLoRaSNIR[LoRaSF-7];
}

double LoRaDimensionalSnir::getMinNonLoRa() const
{
    if (std::isnan(minNonLoRaSNIR))
    {
        const LoRaDimensionalNoise *dimensionalNoise = dynamic_cast<const LoRaDimensionalNoise *>(noise);
        if(dimensionalNoise)
        {
            auto noisePower = dimensionalNoise->getNonLoRapower();
            minNonLoRaSNIR = retMin(noisePower->add(dimensionalNoise->getBackgroundpower()));
        }
    }
    return minNonLoRaSNIR;
}

double LoRaDimensionalSnir::getMaxNonLoRa() const
{
    if (std::isnan(maxNonLoRaSNIR))
    {
        const LoRaDimensionalNoise *dimensionalNoise = dynamic_cast<const LoRaDimensionalNoise *>(noise);
        if(dimensionalNoise)
        {
            auto noisePower = dimensionalNoise->getNonLoRapower();
            maxNonLoRaSNIR = retMax(noisePower->add(dimensionalNoise->getBackgroundpower()));
        }
    }
    return maxNonLoRaSNIR;
}

double LoRaDimensionalSnir::getMeanNonLoRa() const
{
    if (std::isnan(meanNonLoRaSNIR))
    {
        const LoRaDimensionalNoise *dimensionalNoise = dynamic_cast<const LoRaDimensionalNoise *>(noise);
        if(dimensionalNoise)
        {
            auto noisePower = dimensionalNoise->getNonLoRapower();
            meanNonLoRaSNIR = retMean(noisePower->add(dimensionalNoise->getBackgroundpower()));
        }
    }
    return meanNonLoRaSNIR;
}

} // namespace physicallayer

} // namespace labscim

