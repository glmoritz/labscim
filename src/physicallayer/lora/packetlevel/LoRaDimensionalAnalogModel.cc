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

#include "inet/physicallayer/analogmodel/packetlevel/DimensionalAnalogModel.h"
#include "inet/physicallayer/analogmodel/packetlevel/DimensionalNoise.h"
#include "inet/physicallayer/analogmodel/packetlevel/DimensionalReception.h"
#include "inet/physicallayer/analogmodel/packetlevel/DimensionalSnir.h"
#include "inet/physicallayer/analogmodel/packetlevel/DimensionalTransmission.h"
#include "inet/physicallayer/common/packetlevel/BandListening.h"
#include "inet/physicallayer/contract/packetlevel/IRadioMedium.h"
#include "inet/common/Units.h"
#include "LoRaDimensionalAnalogModel.h"
#include "LoRaDimensionalTransmission.h"
#include "LoRaDimensionalReception.h"
#include "LoRaDimensionalNoise.h"
#include "LoRaBandListening.h"
#include "LoRaDimensionalSnir.h"

using namespace inet;
using namespace inet::physicallayer;

namespace labscim {

namespace physicallayer {

Define_Module(LoRaDimensionalAnalogModel);

std::ostream& LoRaDimensionalAnalogModel::printToStream(std::ostream& stream, int level) const
{
    stream << "LoRaDimensionalAnalogModel";
    return DimensionalAnalogModel::printToStream(stream, level);
}

const IReception *LoRaDimensionalAnalogModel::computeReception(const IRadio *receiverRadio, const ITransmission *transmission, const IArrival *arrival) const
{
    const LoRaDimensionalTransmission *loRaTransmission = check_and_cast<const LoRaDimensionalTransmission *>(transmission);
    const simtime_t receptionStartTime = arrival->getStartTime();
    const simtime_t receptionEndTime = arrival->getEndTime();
    const Coord receptionStartPosition = arrival->getStartPosition();
    const Coord receptionEndPosition = arrival->getEndPosition();
    const Quaternion receptionStartOrientation = arrival->getStartOrientation();
    const Quaternion receptionEndOrientation = arrival->getEndOrientation();
    const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& receptionPower = computeReceptionPower(receiverRadio, transmission, arrival);
    int LoRaSF = loRaTransmission->getLoRaSF();
    int LoRaCR = loRaTransmission->getLoRaCR();
    return new LoRaDimensionalReception(receiverRadio, transmission, receptionStartTime, receptionEndTime, receptionStartPosition, receptionEndPosition, receptionStartOrientation, receptionEndOrientation, loRaTransmission->getCenterFrequency(), loRaTransmission->getBandwidth(), receptionPower,LoRaSF,LoRaCR);
}


const INoise *LoRaDimensionalAnalogModel::computeNoise(const IListening *listening, const IInterference *interference) const
{
    const LoRaBandListening *bandListening = check_and_cast<const LoRaBandListening *>(listening);
    Hz centerFrequency = bandListening->getCenterFrequency();
    Hz bandwidth = bandListening->getBandwidth();

    std::array<std::vector<Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>>,6> LoRaReceptionPowers;
    std::vector<Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>> OtherReceptionPowers;

    const DimensionalNoise *dimensionalBackgroundNoise = check_and_cast_nullable<const DimensionalNoise *>(interference->getBackgroundNoise());
    //if (dimensionalBackgroundNoise) {
    //    //add background noise for all kinds of interference
    //    const auto& backgroundNoisePower = dimensionalBackgroundNoise->getPower();
    //    LoRaIntraSFReceptionPowers.push_back(backgroundNoisePower);
    //    LoRaInterSFReceptionPowers.push_back(backgroundNoisePower);
    //    OtherReceptionPowers.push_back(backgroundNoisePower);
    //}
    const std::vector<const IReception *> *interferingReceptions = interference->getInterferingReceptions();
    for (const auto & interferingReception : *interferingReceptions) {
        const LoRaDimensionalReception *loradimensionalReception = dynamic_cast<const LoRaDimensionalReception *>(interferingReception);
        if(loradimensionalReception)
        {
            //check whether the transmissions are orthogonal
            //Hz signalCarrierFrequency = loradimensionalReception->getCenterFrequency();
            //Hz signalBandwidth = loradimensionalReception->getBandwidth();
            //Hz interferenceChirpRate = signalBandwidth / pow(2,loradimensionalReception->getLoRaSF());
            int EquivalentLoRaSF = loradimensionalReception->getLoRaSF() - (int)log2( loradimensionalReception->getBandwidth().get() / bandwidth.get() );
            auto receptionPower = loradimensionalReception->getPower();
            //intraSF interference
            if((EquivalentLoRaSF >= 7)&&(EquivalentLoRaSF <= 12))
            {

                LoRaReceptionPowers[EquivalentLoRaSF - 7].push_back(receptionPower);
                EV_TRACE << "LoRa Interference power begin " << endl;
                EV_TRACE << *receptionPower << endl;
                EV_TRACE << "LoRa Interference power end" << endl;
            }
        }
        else
        {
            const DimensionalReception *dimensionalReception = check_and_cast<const DimensionalReception *>(interferingReception);
            auto receptionPower = dimensionalReception->getPower();
            OtherReceptionPowers.push_back(receptionPower);
            EV_TRACE << "NonLoRa Interference power begin " << endl;
            EV_TRACE << *receptionPower << endl;
            EV_TRACE << "NonLoRaInterference power end" << endl;
        }
    }

    const auto& bandpassFilter = makeShared<Boxcar2DFunction<double, simsec, Hz>>(simsec(listening->getStartTime()), simsec(listening->getEndTime()), centerFrequency - bandwidth / 2, centerFrequency + bandwidth / 2, 1);
    std::array<const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>,6> LoRapower{\
        makeShared<SummedFunction<WpHz, Domain<simsec, Hz>>>(LoRaReceptionPowers[0])->multiply(bandpassFilter),\
        makeShared<SummedFunction<WpHz, Domain<simsec, Hz>>>(LoRaReceptionPowers[1])->multiply(bandpassFilter),\
        makeShared<SummedFunction<WpHz, Domain<simsec, Hz>>>(LoRaReceptionPowers[2])->multiply(bandpassFilter),\
        makeShared<SummedFunction<WpHz, Domain<simsec, Hz>>>(LoRaReceptionPowers[3])->multiply(bandpassFilter),\
        makeShared<SummedFunction<WpHz, Domain<simsec, Hz>>>(LoRaReceptionPowers[4])->multiply(bandpassFilter),\
        makeShared<SummedFunction<WpHz, Domain<simsec, Hz>>>(LoRaReceptionPowers[5])->multiply(bandpassFilter)} ;

    const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& NonLoRanoisePower = makeShared<SummedFunction<WpHz, Domain<simsec, Hz>>>(OtherReceptionPowers);

    /*for(int i=0;i<7;i++)
    {
        LoRapower[i] = makeShared<SummedFunction<WpHz, Domain<simsec, Hz>>>(LoRaReceptionPowers[i]);//->multiply(bandpassFilter);
    }*/


    return new LoRaDimensionalNoise(listening->getStartTime(), listening->getEndTime(), centerFrequency, bandwidth, LoRapower, NonLoRanoisePower->multiply(bandpassFilter), dimensionalBackgroundNoise->getPower()->multiply(bandpassFilter));
}

const INoise *LoRaDimensionalAnalogModel::computeNoise(const IReception *reception, const INoise *noise) const
{
    auto loradimensionalReception = dynamic_cast<const LoRaDimensionalReception *>(reception);
    auto loradimensionalNoise = dynamic_cast<const LoRaDimensionalNoise *>(noise);
    auto dimensionalReception = check_and_cast<const DimensionalReception *>(reception);
    auto dimensionalNoise = check_and_cast<const DimensionalNoise *>(noise);
    const auto& bandpassFilter = makeShared<Boxcar2DFunction<double, simsec, Hz>>(simsec(dimensionalReception->getStartTime()), simsec(dimensionalReception->getEndTime()),  dimensionalNoise->getCenterFrequency() - dimensionalNoise->getBandwidth()/2, dimensionalNoise->getCenterFrequency() + dimensionalNoise->getBandwidth()/2, 1);


    const auto filtered_reception = loradimensionalReception->getPower()->multiply(bandpassFilter);

    //TODO: why is there no need to filter the noise?
    //const auto& bandpassFilter = makeShared<Boxcar2DFunction<double, simsec, Hz>>(simsec(reception->getStartTime()), simsec(reception->getEndTime()), dimensionalReception->getCenterFrequency() - dimensionalReception->getBandwidth() / 2, dimensionalReception->getCenterFrequency() + dimensionalReception->getBandwidth() / 2, 1);

    if(loradimensionalNoise)
    {
        if(loradimensionalReception)
        {
            //lora interference on lora noise
            int EquivalentLoRaSF = loradimensionalReception->getLoRaSF() - (int)log2( (loradimensionalReception->getBandwidth().get() / loradimensionalNoise->getBandwidth().get() ));
            //TODO: couldn't find a clean way to declare a new array of const pointers but adding the new noise to the correct SF
            //i think that this is the ugliest code I've ever made
            switch(EquivalentLoRaSF)
            {
            case 7:
            {
                std::array<const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>,6> LoRapower { \
                    makeShared<AddedFunction<WpHz, Domain<simsec, Hz>>>(filtered_reception, loradimensionalNoise->getLoRapower(7)),\
                    loradimensionalNoise->getLoRapower(8),\
                    loradimensionalNoise->getLoRapower(9),\
                    loradimensionalNoise->getLoRapower(10),\
                    loradimensionalNoise->getLoRapower(11),\
                    loradimensionalNoise->getLoRapower(12)};
                return new LoRaDimensionalNoise(reception->getStartTime(), reception->getEndTime(), loradimensionalReception->getCenterFrequency(), loradimensionalReception->getBandwidth(), LoRapower,loradimensionalNoise->getNonLoRapower(),loradimensionalNoise->getBackgroundpower());
                break;
            }
            case 8:
            {
                std::array<const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>,6> LoRapower { \
                    loradimensionalNoise->getLoRapower(7),\
                    makeShared<AddedFunction<WpHz, Domain<simsec, Hz>>>(filtered_reception, loradimensionalNoise->getLoRapower(8)),\
                    loradimensionalNoise->getLoRapower(9),\
                    loradimensionalNoise->getLoRapower(10),\
                    loradimensionalNoise->getLoRapower(11),\
                    loradimensionalNoise->getLoRapower(12)};
                return new LoRaDimensionalNoise(reception->getStartTime(), reception->getEndTime(), loradimensionalReception->getCenterFrequency(), loradimensionalReception->getBandwidth(), LoRapower,loradimensionalNoise->getNonLoRapower(),loradimensionalNoise->getBackgroundpower());
                break;
            }
            case 9:
            {
                std::array<const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>,6> LoRapower { \
                    loradimensionalNoise->getLoRapower(7),\
                    loradimensionalNoise->getLoRapower(8),\
                    makeShared<AddedFunction<WpHz, Domain<simsec, Hz>>>(filtered_reception, loradimensionalNoise->getLoRapower(9)),\
                    loradimensionalNoise->getLoRapower(10),\
                    loradimensionalNoise->getLoRapower(11),\
                    loradimensionalNoise->getLoRapower(12)};
                return new LoRaDimensionalNoise(reception->getStartTime(), reception->getEndTime(), loradimensionalReception->getCenterFrequency(), loradimensionalReception->getBandwidth(), LoRapower,loradimensionalNoise->getNonLoRapower(),loradimensionalNoise->getBackgroundpower());
                break;
            }
            case 10:
            {
                std::array<const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>,6> LoRapower { \
                    loradimensionalNoise->getLoRapower(7),\
                    loradimensionalNoise->getLoRapower(8),\
                    loradimensionalNoise->getLoRapower(9),\
                    makeShared<AddedFunction<WpHz, Domain<simsec, Hz>>>(filtered_reception, loradimensionalNoise->getLoRapower(10)),\
                    loradimensionalNoise->getLoRapower(11),\
                    loradimensionalNoise->getLoRapower(12)};
                return new LoRaDimensionalNoise(reception->getStartTime(), reception->getEndTime(), loradimensionalReception->getCenterFrequency(), loradimensionalReception->getBandwidth(), LoRapower,loradimensionalNoise->getNonLoRapower(),loradimensionalNoise->getBackgroundpower());
                break;
            }
            case 11:
            {
                std::array<const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>,6> LoRapower { \
                    loradimensionalNoise->getLoRapower(7),\
                    loradimensionalNoise->getLoRapower(8),\
                    loradimensionalNoise->getLoRapower(9),\
                    loradimensionalNoise->getLoRapower(10),\
                    makeShared<AddedFunction<WpHz, Domain<simsec, Hz>>>(filtered_reception, loradimensionalNoise->getLoRapower(11)),\
                    loradimensionalNoise->getLoRapower(12)};
                return new LoRaDimensionalNoise(reception->getStartTime(), reception->getEndTime(), loradimensionalReception->getCenterFrequency(), loradimensionalReception->getBandwidth(), LoRapower,loradimensionalNoise->getNonLoRapower(),loradimensionalNoise->getBackgroundpower());
                break;
            }
            case 12:
            {
                std::array<const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>,6> LoRapower { \
                    loradimensionalNoise->getLoRapower(7),\
                    loradimensionalNoise->getLoRapower(8),\
                    loradimensionalNoise->getLoRapower(9),\
                    loradimensionalNoise->getLoRapower(10),\
                    loradimensionalNoise->getLoRapower(11),\
                    makeShared<AddedFunction<WpHz, Domain<simsec, Hz>>>(filtered_reception, loradimensionalNoise->getLoRapower(12))};
                return new LoRaDimensionalNoise(reception->getStartTime(), reception->getEndTime(), loradimensionalReception->getCenterFrequency(), loradimensionalReception->getBandwidth(), LoRapower,loradimensionalNoise->getNonLoRapower(),loradimensionalNoise->getBackgroundpower());
                break;
            }
            default:
            {
                return new LoRaDimensionalNoise(reception->getStartTime(), reception->getEndTime(), loradimensionalReception->getCenterFrequency(), loradimensionalReception->getBandwidth(), loradimensionalNoise->getLoRapower(),loradimensionalNoise->getNonLoRapower(),loradimensionalNoise->getBackgroundpower());
                break;
            }
            }
        }
        else
        {
            //non-lora interference on lora noise
            const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& NonLoRaPower = makeShared<AddedFunction<WpHz, Domain<simsec, Hz>>>(filtered_reception, loradimensionalNoise->getNonLoRapower());
            return new LoRaDimensionalNoise(reception->getStartTime(), reception->getEndTime(), loradimensionalReception->getCenterFrequency(), loradimensionalReception->getBandwidth(), loradimensionalNoise->getLoRapower(),NonLoRaPower,loradimensionalNoise->getBackgroundpower());
        }
    }
    else
    {
        //common dimensional noise
        return DimensionalAnalogModel::computeNoise(reception, noise);
    }
}

const ISnir *LoRaDimensionalAnalogModel::computeSNIR(const IReception *reception, const INoise *noise) const
{
    auto loradimensionalReception = dynamic_cast<const LoRaDimensionalReception *>(reception);
    auto loradimensionalNoise = dynamic_cast<const LoRaDimensionalNoise *>(noise);
    if(loradimensionalReception)
    {
        if(loradimensionalNoise)
        {
            return new LoRaDimensionalSnir(loradimensionalReception, loradimensionalNoise);
        }
        else
        {
            return DimensionalAnalogModel::computeSNIR(reception, noise);
        }
    }
    else
    {
        return DimensionalAnalogModel::computeSNIR(reception, noise);
    }
}



} // namespace physicallayer

} // namespace labscim

