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
#include "LoRaDimensionalFHSSTransmission.h"
#include "LoRaDimensionalFHSSSnir.h"

using namespace inet;
using namespace inet::physicallayer;

namespace labscim {

namespace physicallayer {


LoRaDimensionalFHSSSnir::LoRaDimensionalFHSSSnir(const DimensionalReception *reception, const DimensionalNoise *noise) :
    DimensionalSnir(reception, noise)
{
    const std::vector<labscim::physicallayer::LoRaFHSSHopEntry>& hopTable = check_and_cast<const LoRaDimensionalFHSSTransmission*>(reception->getTransmission())->getHopTable();
    std::vector<double> const_nan(hopTable.size(),NaN);
    minHopSNR = const_nan;
    maxHopSNR = const_nan;
    meanHopSNR = const_nan;

    for(uint32_t i=0;i<hopTable.size();i++)
    {
        if(!hopTable[i].isHeader())
        {
            numHeaders = i;
            break;
        }
    }
}

std::ostream& LoRaDimensionalFHSSSnir::printToStream(std::ostream& stream, int level) const
{
    stream << "LoRaDimensionalFHSSSnir";
//    if (level <= PRINT_LEVEL_DETAIL)
//        stream << ", minSNIR = " << minSNIR;
    return DimensionalSnir::printToStream(stream, level);
}


void LoRaDimensionalFHSSSnir::computeHopMin(uint32_t numhops) const
{
    // TODO: factor out common part
    const DimensionalNoise *dimensionalNoise = check_and_cast<const DimensionalNoise *>(noise);
    const DimensionalReception *dimensionalReception = check_and_cast<const DimensionalReception *>(reception);
    EV_TRACE << "Reception power begin " << endl;
    EV_TRACE << *dimensionalReception->getPower() << endl;
    EV_TRACE << "Reception power end" << endl;
    auto noisePower = dimensionalNoise->getPower();
    auto receptionPower = dimensionalReception->getPower();
    auto snir = receptionPower->divide(noisePower);
    const std::vector<labscim::physicallayer::LoRaFHSSHopEntry>& hopTable = dynamic_cast<const LoRaDimensionalFHSSTransmission*>(reception->getTransmission())->getHopTable();

    simsec hopstart = simsec(reception->getStartTime());
    simsec hopend;

    EV_TRACE << "SNIR begin " << endl;
    EV_TRACE << *snir << endl;
    EV_TRACE << "SNIR end" << endl;
    uint32_t i = 0;

    //calculate by-hop min SNR
    for(const auto& hop : hopTable)
    {
        if(std::isnan(minHopSNR[i]))
        {
            hopend =  hopstart + simsec(hop.getDuration());
            Hz centerFrequency = hop.getCenterFrequency();
            Hz bandwidth = hop.getBandwidth();
            Point<simsec, Hz> startPoint(hopstart, centerFrequency - bandwidth / 2);
            Point<simsec, Hz> endPoint(hopend, centerFrequency + bandwidth / 2);

            minHopSNR[i] = snir->getMin(Interval<simsec, Hz>(startPoint, endPoint, 0b1, 0b0, 0b0));
            EV_DEBUG << "Computing minimum Hop SNIR: start = " << startPoint << ", end = " << endPoint << " -> minimum SNIR = " << minHopSNR[i] << endl;
            hopstart = hopend;
        }
        i++;
        if(i>=numhops)
        {
            break;
        }
    }
}

void LoRaDimensionalFHSSSnir::computeHopMean(uint32_t numhops) const
{
    // TODO: factor out common part
    const DimensionalNoise *dimensionalNoise = check_and_cast<const DimensionalNoise *>(noise);
    const DimensionalReception *dimensionalReception = check_and_cast<const DimensionalReception *>(reception);
    EV_TRACE << "Reception power begin " << endl;
    EV_TRACE << *dimensionalReception->getPower() << endl;
    EV_TRACE << "Reception power end" << endl;
    auto noisePower = dimensionalNoise->getPower();
    auto receptionPower = dimensionalReception->getPower();
    auto snir = receptionPower->divide(noisePower);
    const std::vector<labscim::physicallayer::LoRaFHSSHopEntry>& hopTable = dynamic_cast<const LoRaDimensionalFHSSTransmission*>(reception->getTransmission())->getHopTable();

    simsec hopstart = simsec(reception->getStartTime());
    simsec hopend;

    EV_TRACE << "SNIR begin " << endl;
    EV_TRACE << *snir << endl;
    EV_TRACE << "SNIR end" << endl;
    uint32_t i = 0;

    //calculate by-hop mean SNR
    for(const auto& hop : hopTable)
    {
        if(std::isnan(meanHopSNR[i]))
        {
            hopend =  hopstart + simsec(hop.getDuration());
            Hz centerFrequency = hop.getCenterFrequency();
            Hz bandwidth = hop.getBandwidth();
            Point<simsec, Hz> startPoint(hopstart, centerFrequency - bandwidth / 2);
            Point<simsec, Hz> endPoint(hopend, centerFrequency + bandwidth / 2);

            meanHopSNR[i] = snir->getMean(Interval<simsec, Hz>(startPoint, endPoint, 0b1, 0b0, 0b0));
            EV_DEBUG << "Computing mean Hop SNIR: start = " << startPoint << ", end = " << endPoint << " -> minimum SNIR = " << meanHopSNR[i] << endl;
            hopstart = hopend;
        }
        i++;
        if(i>=numhops)
        {
            break;
        }
    }
}

void LoRaDimensionalFHSSSnir::computeHopMax(uint32_t numhops) const
{
    // TODO: factor out common part
    const DimensionalNoise *dimensionalNoise = check_and_cast<const DimensionalNoise *>(noise);
    const DimensionalReception *dimensionalReception = check_and_cast<const DimensionalReception *>(reception);
    EV_TRACE << "Reception power begin " << endl;
    EV_TRACE << *dimensionalReception->getPower() << endl;
    EV_TRACE << "Reception power end" << endl;
    auto noisePower = dimensionalNoise->getPower();
    auto receptionPower = dimensionalReception->getPower();
    auto snir = receptionPower->divide(noisePower);
    const std::vector<labscim::physicallayer::LoRaFHSSHopEntry>& hopTable = dynamic_cast<const LoRaDimensionalFHSSTransmission*>(reception->getTransmission())->getHopTable();

    simsec hopstart = simsec(reception->getStartTime());
    simsec hopend;

    EV_TRACE << "SNIR begin " << endl;
    EV_TRACE << *snir << endl;
    EV_TRACE << "SNIR end" << endl;
    uint32_t i = 0;

    //calculate by-hop max SNR
    for(const auto& hop : hopTable)
    {
        if(std::isnan(maxHopSNR[i]))
        {
            hopend =  hopstart + simsec(hop.getDuration());
            Hz centerFrequency = hop.getCenterFrequency();
            Hz bandwidth = hop.getBandwidth();
            Point<simsec, Hz> startPoint(hopstart, centerFrequency - bandwidth / 2);
            Point<simsec, Hz> endPoint(hopend, centerFrequency + bandwidth / 2);

            maxHopSNR[i] = snir->getMax(Interval<simsec, Hz>(startPoint, endPoint, 0b1, 0b0, 0b0));
            EV_DEBUG << "Computing maximum Hop SNIR: start = " << startPoint << ", end = " << endPoint << " -> minimum SNIR = " << maxHopSNR[i] << endl;
            hopstart = hopend;
        }
        i++;
        if(i>=numhops)
        {
            break;
        }
    }
}

 uint32_t LoRaDimensionalFHSSSnir::getHeadersWithMinBelowThresold(double Threshold) const
 {
     if(std::isnan(minHopSNR[numHeaders-1]))
     {
         computeHopMin(numHeaders);
     }
     std::vector<double>::iterator end = minHopSNR.begin();
     std::advance(end,numHeaders);
     int32_t res = std::count_if(minHopSNR.begin(), end, [&](double i) { return i < Threshold; });
     return res;

 }

 uint32_t LoRaDimensionalFHSSSnir::getHeadersWithMeanBelowThresold(double Threshold) const
 {
     if(std::isnan(meanHopSNR[numHeaders-1]))
     {
         computeHopMean(numHeaders);
     }
     std::vector<double>::iterator end = meanHopSNR.begin();
     std::advance(end,numHeaders);
     int32_t res = std::count_if(meanHopSNR.begin(), end, [&](double i) { return i < Threshold; });
     return res;
 }

 uint32_t LoRaDimensionalFHSSSnir::getHeadersWithMaxBelowThresold(double Threshold) const
 {
      if(std::isnan(maxHopSNR[numHeaders-1]))
      {
          computeHopMax(numHeaders);
      }
      std::vector<double>::iterator end = maxHopSNR.begin();
      std::advance(end,numHeaders);
      int32_t res = std::count_if(maxHopSNR.begin(), end, [&](double i) { return i < Threshold; });
      return res;
  }

 uint32_t LoRaDimensionalFHSSSnir::getHopsWithMinBelowThresold(double Threshold) const
 {
     if(std::isnan(minHopSNR[numHeaders]))
     {
         computeHopMin(getNumHeaders());
     }
     std::vector<double>::iterator begin = minHopSNR.begin();
     std::advance(begin,numHeaders);
     int32_t res = std::count_if(begin, minHopSNR.end(), [&](double i) { return i < Threshold; });
     return res;
 }

 uint32_t LoRaDimensionalFHSSSnir::getHopsWithMaxBelowThresold(double Threshold) const
 {
      if(std::isnan(maxHopSNR[numHeaders]))
      {
          computeHopMax(getNumHeaders());
      }
      std::vector<double>::iterator begin = maxHopSNR.begin();
      std::advance(begin,numHeaders);
      int32_t res = std::count_if(begin, maxHopSNR.end(), [&](double i) { return i < Threshold; });
      return res;
  }

 uint32_t LoRaDimensionalFHSSSnir::getHopsWithMeanBelowThresold(double Threshold) const
 {
     if(std::isnan(meanHopSNR[numHeaders]))
     {
         computeHopMean(getNumHeaders());
     }
     std::vector<double>::iterator begin = meanHopSNR.begin();
     std::advance(begin,numHeaders);
     int32_t res = std::count_if(begin, meanHopSNR.end(), [&](double i) { return i < Threshold; });
     return res;
 }


 uint32_t LoRaDimensionalFHSSSnir::getNumHops() const
 {
     const std::vector<labscim::physicallayer::LoRaFHSSHopEntry>& hopTable = dynamic_cast<const LoRaDimensionalFHSSTransmission*>(reception->getTransmission())->getHopTable();
     return hopTable.size();
 }



} // namespace physicallayer

} // namespace labscim

