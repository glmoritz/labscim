//
// Copyright (C) 2014 Florian Meier
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

#include "LoRaDimensionalReceiver.h"
#include "LoRaDimensionalReception.h"
#include "LoRaDimensionalTransmission.h"
#include "LoRaDimensionalNoise.h"
#include "LoRaBandListening.h"
#include "LoRaDimensionalSnir.h"

#include "inet/common/math/Functions.h"
#include "inet/common/Units.h"



using namespace inet;
using namespace inet::physicallayer;
using namespace inet::math;


namespace labscim {

namespace physicallayer {

Define_Module(LoRaDimensionalReceiver);

LoRaDimensionalReceiver::LoRaDimensionalReceiver() :
    FlatReceiverBase()
{
}

void LoRaDimensionalReceiver::initialize(int stage)
{
    FlatReceiverBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        minInterferencePower = mW(dBmW2mW(par("minInterferencePower")));
        snirNonLoRaThreshold = mW(dBmW2mW(par("snirNonLoRaThreshold")));
        LoRaSF = par("LoRaSF");
    }
}

std::ostream& LoRaDimensionalReceiver::printToStream(std::ostream& stream, int level) const
{
    stream << "LoRaDimensionalReceiver";
    return FlatReceiverBase::printToStream(stream, level);
}


W LoRaDimensionalReceiver::getSensitivity(const LoRaDimensionalReception *reception) const
{
    //function returns sensitivity -- according to LoRa documentation, it changes with LoRa parameters
    //Sensitivity values from Semtech SX1272/73 datasheet, table 10, Rev 3.1, March 2017
    W sensitivity = W(math::dBmW2mW(-126.5) / 1000);
    if(reception->getLoRaSF() == 6)
    {
        if(reception->getBandwidth() == Hz(125000)) sensitivity = W(math::dBmW2mW(-121) / 1000);
        if(reception->getBandwidth() == Hz(250000)) sensitivity = W(math::dBmW2mW(-118) / 1000);
        if(reception->getBandwidth() == Hz(500000)) sensitivity = W(math::dBmW2mW(-111) / 1000);
    }

    if (reception->getLoRaSF() == 7)
    {
        if(reception->getBandwidth() == Hz(125000)) sensitivity = W(math::dBmW2mW(-124) / 1000);
        if(reception->getBandwidth() == Hz(250000)) sensitivity = W(math::dBmW2mW(-122) / 1000);
        if(reception->getBandwidth() == Hz(500000)) sensitivity = W(math::dBmW2mW(-116) / 1000);
    }

    if(reception->getLoRaSF() == 8)
    {
        if(reception->getBandwidth() == Hz(125000)) sensitivity = W(math::dBmW2mW(-127) / 1000);
        if(reception->getBandwidth() == Hz(250000)) sensitivity = W(math::dBmW2mW(-125) / 1000);
        if(reception->getBandwidth() == Hz(500000)) sensitivity = W(math::dBmW2mW(-119) / 1000);
    }
    if(reception->getLoRaSF() == 9)
    {
        if(reception->getBandwidth() == Hz(125000)) sensitivity = W(math::dBmW2mW(-130) / 1000);
        if(reception->getBandwidth() == Hz(250000)) sensitivity = W(math::dBmW2mW(-128) / 1000);
        if(reception->getBandwidth() == Hz(500000)) sensitivity = W(math::dBmW2mW(-122) / 1000);
    }
    if(reception->getLoRaSF() == 10)
    {
        if(reception->getBandwidth() == Hz(125000)) sensitivity = W(math::dBmW2mW(-133) / 1000);
        if(reception->getBandwidth() == Hz(250000)) sensitivity = W(math::dBmW2mW(-130) / 1000);
        if(reception->getBandwidth() == Hz(500000)) sensitivity = W(math::dBmW2mW(-125) / 1000);
    }
    if(reception->getLoRaSF() == 11)
    {
        if(reception->getBandwidth() == Hz(125000)) sensitivity = W(math::dBmW2mW(-135) / 1000);
        if(reception->getBandwidth() == Hz(250000)) sensitivity = W(math::dBmW2mW(-132) / 1000);
        if(reception->getBandwidth() == Hz(500000)) sensitivity = W(math::dBmW2mW(-128) / 1000);
    }
    if(reception->getLoRaSF() == 12)
    {
        if(reception->getBandwidth() == Hz(125000)) sensitivity = W(math::dBmW2mW(-137) / 1000);
        if(reception->getBandwidth() == Hz(250000)) sensitivity = W(math::dBmW2mW(-135) / 1000);
        if(reception->getBandwidth() == Hz(500000)) sensitivity = W(math::dBmW2mW(-129) / 1000);
    }
    return sensitivity;
}



bool LoRaDimensionalReceiver::computeIsReceptionPossible(const IListening *listening, const ITransmission *transmission) const
{
    //here we can check compatibility of LoRaTx parameters (or being a gateway)
    const LoRaDimensionalTransmission *loRaTransmission = dynamic_cast<const LoRaDimensionalTransmission *>(transmission);
    const LoRaBandListening* loRaListening = dynamic_cast<const LoRaBandListening *>(listening);

    return loRaTransmission && loRaListening && (iAmGateway || (loRaListening->getCenterFrequency() == loRaTransmission->getCenterFrequency() && loRaListening->getBandwidth() == loRaTransmission->getBandwidth() && loRaListening->getLoRaSF() == loRaTransmission->getLoRaSF()));
}

// TODO: this is not purely functional, see interface comment
bool LoRaDimensionalReceiver::computeIsReceptionPossible(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part) const
{
    //here we can check compatibility of LoRaTx parameters (or being a gateway) and reception above sensitivity level
    const LoRaBandListening *loRaListening = check_and_cast<const LoRaBandListening *>(listening);
    const LoRaDimensionalReception *loRaReception = check_and_cast<const LoRaDimensionalReception *>(reception);
    if (iAmGateway == false && (loRaListening->getCenterFrequency() != loRaReception->getCenterFrequency() || loRaListening->getBandwidth() != loRaReception->getBandwidth() || loRaListening->getLoRaSF() != loRaReception->getLoRaSF())) {
        return false;
    }
    else
    {
        W minReceptionPower = loRaReception->computeMinPower(reception->getStartTime(part), reception->getEndTime(part));
        W sensitivity = getSensitivity(loRaReception);
        bool isReceptionPossible = minReceptionPower >= sensitivity;
        EV_DEBUG << "Computing whether reception is possible: minimum reception power = " << minReceptionPower << ", sensitivity = " << sensitivity << " -> reception is " << (isReceptionPossible ? "possible" : "impossible") << endl;
        return isReceptionPossible;
    }
}

bool LoRaDimensionalReceiver::computeIsReceptionAttempted(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference) const
{
    if(iAmGateway)
    {
        return computeIsReceptionPossible(listening,reception,part);
    }
    else
    {
        //this simulation is still not considering the LoRa capture effect (this is a good place to implement it)
        return ReceiverBase::computeIsReceptionAttempted(listening, reception, part, interference);
    }
}

bool LoRaDimensionalReceiver::computeIsReceptionSuccessful(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference, const ISnir *snir) const
{
    auto loradimensionalReception = check_and_cast<const LoRaDimensionalReception *>(reception);
    auto loradimensionalsnir = check_and_cast<const LoRaDimensionalSnir *>(snir);


    //check interference in each LoRaSF using:

    //D. Croce, M. Gucciardo, S. Mangione, G. Santaromita and I. Tinnirello,
    //"Impact of LoRa Imperfect Orthogonality: Analysis of Link-Level Performance,"
    //in IEEE Communications Letters, vol. 22, no. 4, pp. 796-799, April 2018,
    //doi: 10.1109/LCOMM.2018.2797057.

    for(int LoRaSF = 7;LoRaSF <= 12;LoRaSF++)
    {
        if (snirThresholdMode == SnirThresholdMode::STM_MIN)
        {
            if( W(loradimensionalsnir->getMinLoRa(LoRaSF)) < W(math::dBmW2mW(nonOrthDelta[loradimensionalReception->getLoRaSF()-7][LoRaSF-7])) )
            {
                return false;
            }
        }
        else if (snirThresholdMode == SnirThresholdMode::STM_MEAN)
        {
            if( W(loradimensionalsnir->getMeanLoRa(LoRaSF)) < W(math::dBmW2mW(nonOrthDelta[loradimensionalReception->getLoRaSF()-7][LoRaSF-7])) )
            {
                return false;
            }
        }
        else
        {
            throw cRuntimeError("Unknown SNIR threshold mode: '%s'", snirThresholdMode);
        }
    }

    //check non lora interference using:

    //C. Orfanidis, L. M. Feeney, M. Jacobsson and P. Gunningberg,
    //"Investigating interference between LoRa and IEEE 802.15.4g networks,"
    //2017 IEEE 13th International Conference on Wireless and Mobile Computing, Networking and Communications (WiMob), Rome, 2017, pp. 1-8,
    //doi: 10.1109/WiMOB.2017.8115772.

    //K. Mikhaylov, J. Petäjäjärvi and J. Janhunen,
    //"On LoRaWAN scalability: Empirical evaluation of susceptibility to inter-network interference,"
    //2017 European Conference on Networks and Communications (EuCNC), Oulu, 2017, pp. 1-6,
    //doi: 10.1109/EuCNC.2017.7980757.
    //-> this paper indicates reception above -6db SNR on fig 4d

    //L. E. Marquez, A. Osorio, M. Calle, J. C. Velez, A. Serrano and J. E. Candelo-Becerra,
    //"On the Use of LoRaWAN in Smart Cities: A Study With Blocking Interference,"
    //in IEEE Internet of Things Journal, vol. 7, no. 4, pp. 2806-2815, April 2020,
    //doi: 10.1109/JIOT.2019.2962976.
    //->this paper indicates reception above -15db for SF12 (TABLE III)


    if (snirThresholdMode == SnirThresholdMode::STM_MIN)
    {
        if( W(loradimensionalsnir->getMinNonLoRa()) < snirNonLoRaThreshold )
        {
            return false;
        }
    }
    else if (snirThresholdMode == SnirThresholdMode::STM_MEAN)
    {
        if( W(loradimensionalsnir->getMeanNonLoRa()) <  snirNonLoRaThreshold )
        {
            return false;
        }
    }
    else
    {
        throw cRuntimeError("Unknown SNIR threshold mode: '%s'", snirThresholdMode);
    }

    return true;
}

const IListening *LoRaDimensionalReceiver::createListening(const IRadio *radio, const simtime_t startTime, const simtime_t endTime, const Coord startPosition, const Coord endPosition) const
{
    return new LoRaBandListening(radio, startTime, endTime, startPosition, endPosition, centerFrequency, bandwidth, LoRaSF);
}



} // namespace physicallayer

} // namespace labscim

