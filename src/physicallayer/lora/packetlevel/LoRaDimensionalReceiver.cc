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
#include "LoRaDimensionalFHSSTransmission.h"
#include "LoRaDimensionalNoise.h"
#include "LoRaBandListening.h"
#include "LoRaDimensionalSnir.h"
#include "LoRaDimensionalFHSSSnir.h"
#include "LoRaTags_m.h"

#include <fstream>


#include "inet/physicallayer/wireless/common/radio/packetlevel/ReceptionDecision.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"
#include "inet/common/math/Functions.h"
#include "inet/common/Units.h"

#include "../../../common/lr_fhss_v1_base_types.h"


using namespace inet;
using namespace inet::physicallayer;
using namespace inet::math;


namespace labscim {

namespace physicallayer {

Define_Module(LoRaDimensionalReceiver);


LoRaDimensionalReceiver::LoRaDimensionalReceiver() :
    FlatReceiverBase(),
    iAmGateway(false)
{
}

void LoRaDimensionalReceiver::initialize(int stage)
{
    FlatReceiverBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        minInterferencePower = mW(dBmW2mW(par("minInterferencePower")));
        snirNonLoRaThreshold = mW(dBmW2mW(par("snirNonLoRaThreshold")));
        iAmGateway = par("iAmGateway");
        LoRaSF = par("LoRaSF");
    }
}

std::ostream& LoRaDimensionalReceiver::printToStream(std::ostream& stream, int level, int evFlags) const
{
    stream << "LoRaDimensionalReceiver. Gateway = " << iAmGateway << " LoRaSF =" << LoRaSF << ", ";
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
    const LoRaDimensionalFHSSTransmission *FHSSTransmission = dynamic_cast<const LoRaDimensionalFHSSTransmission *>(transmission);
    const NarrowbandTransmissionBase *nbtransmission = dynamic_cast<const NarrowbandTransmissionBase *>(transmission);

    const LoRaBandListening* loRaListening = dynamic_cast<const LoRaBandListening *>(listening);
    bool GatewayRxPossible = false;
    bool isReceptionPossible = false;

    if(iAmGateway)
    {
        //we are ignoring the case where only some FHSS bands are out of listening band.
        //Anyway, the gateway never should be configured this way
        Hz GWminFreq = loRaListening->getCenterFrequency() - loRaListening->getBandwidth()/2;
        Hz GWmaxFreq = loRaListening->getCenterFrequency() + loRaListening->getBandwidth()/2;
        Hz TxminFreq = nbtransmission->getCenterFrequency() - nbtransmission->getBandwidth()/2;
        Hz TxMaxFreq = nbtransmission->getCenterFrequency() + nbtransmission->getBandwidth()/2;
        GatewayRxPossible = (GWminFreq <= TxminFreq) && (GWmaxFreq >= TxMaxFreq);
    }

    //transmission is LoRa or LR-FHSS received by a gateway
    bool isReceptionSupported = (loRaTransmission || (FHSSTransmission && iAmGateway));
    bool LoRaRXOk = false;
    if(loRaListening)
    {
        LoRaRXOk = (loRaListening->getCenterFrequency() == loRaTransmission->getCenterFrequency()) && \
                        (loRaListening->getBandwidth() == loRaTransmission->getBandwidth()) && \
                        (loRaListening->getLoRaSF() == loRaTransmission->getLoRaSF());
    }
    return  isReceptionSupported && loRaListening && (GatewayRxPossible || LoRaRXOk);
}

// TODO: this is not purely functional, see interface comment
bool LoRaDimensionalReceiver::computeIsReceptionPossible(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part) const
{
    //here we can check compatibility of LoRaTx parameters (or being a gateway) and reception above sensitivity level
    const LoRaBandListening *loRaListening = check_and_cast<const LoRaBandListening *>(listening);

    const NarrowbandReceptionBase *nbreception = dynamic_cast<const NarrowbandReceptionBase *>(reception);
    const LoRaDimensionalReception *loRaReception = dynamic_cast<const LoRaDimensionalReception *>(reception);


    if(!iAmGateway)
    {
        //I must be listening in the exact parameters
        if(loRaReception)
        {
            if (                                                                                              \
                    (loRaListening->getCenterFrequency() != loRaReception->getCenterFrequency())              \
                    ||  (loRaListening->getBandwidth() != loRaReception->getBandwidth())                      \
                    ||  (loRaListening->getLoRaSF() != loRaReception->getLoRaSF())                            \
            )
            {
                EV_DEBUG << "Reception is not possible: Listening:" <<  loRaListening->getCenterFrequency() << ", BW" << loRaListening->getBandwidth() << "SF: "  << loRaListening->getLoRaSF() << ". Transmission: " << loRaReception->getCenterFrequency() << ", BW" << loRaReception->getBandwidth() << " SF: " << loRaReception->getLoRaSF();
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    const NarrowbandTransmissionBase *nbtransmission = dynamic_cast<const NarrowbandTransmissionBase *>(reception->getTransmission());

    bool TransmissionWithinBW = false;
    Hz GWminFreq = loRaListening->getCenterFrequency() - loRaListening->getBandwidth()/2;
    Hz GWmaxFreq = loRaListening->getCenterFrequency() + loRaListening->getBandwidth()/2;
    Hz TxminFreq = nbtransmission->getCenterFrequency() - nbtransmission->getBandwidth()/2;
    Hz TxMaxFreq = nbtransmission->getCenterFrequency() + nbtransmission->getBandwidth()/2;
    TransmissionWithinBW = (GWminFreq <= TxminFreq) && (GWmaxFreq >= TxMaxFreq);

    if(TransmissionWithinBW)
    {
        const LoRaDimensionalFHSSTransmission *FHSSTransmission = dynamic_cast<const LoRaDimensionalFHSSTransmission *>(reception->getTransmission());
        if(FHSSTransmission)
        {
            //check whether at least one header power is above radio sensitivity
            const std::vector<labscim::physicallayer::LoRaFHSSHopEntry>& HopTable = FHSSTransmission->getHopTable();
            bool isReceptionPossible = false;
            uint32_t i=0;
            simtime_t starttime = reception->getStartTime(part);
            const W sensitivity = W(math::dBmW2mW(-121) / 1000); //the lowest 125kHz sensitivity for LoRa. This is a conservative approximation
            while(HopTable[i].isHeader())
            {
                W minReceptionPower = nbreception->computeMinPower(starttime, starttime+HopTable[i].getDuration());
                isReceptionPossible |= minReceptionPower >= sensitivity;
                if(isReceptionPossible)
                {
                    break;
                }
                starttime += HopTable[i].getDuration();
            }
            return isReceptionPossible;
        }
        else if(loRaReception)
        {
            W minReceptionPower = loRaReception->computeMinPower(reception->getStartTime(part), reception->getEndTime(part));
            W sensitivity = getSensitivity(loRaReception);
            bool isReceptionPossible = minReceptionPower >= sensitivity;
            EV_DEBUG << "Computing whether reception is possible: minimum reception power = " << minReceptionPower << ", sensitivity = " << sensitivity << " -> reception is " << (isReceptionPossible ? "possible" : "impossible") << endl;
            return isReceptionPossible;
        }
        else
        {
            //not a lora nor FHSS packet
            return false;
        }
    }
    else
    {
        //transmission does not fit my bandwidth
        return false;
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
        EV_DEBUG << "Check whether reception is attempted";
        return ReceiverBase::computeIsReceptionAttempted(listening, reception, part, interference);
    }
}

bool LoRaDimensionalReceiver::computeIsReceptionSuccessful(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference, const ISnir *snir) const
{

    auto loradimensionalsnir = dynamic_cast<const LoRaDimensionalSnir *>(snir);
    auto lorafhsssnir = dynamic_cast<const LoRaDimensionalFHSSSnir *>(snir);
    bool InterfererPresent = false;


    if(loradimensionalsnir)
    {
        auto loradimensionalReception = check_and_cast<const LoRaDimensionalReception *>(reception);
        //check interference in each LoRaSF using:

        //D. Croce, M. Gucciardo, S. Mangione, G. Santaromita and I. Tinnirello,
        //"Impact of LoRa Imperfect Orthogonality: Analysis of Link-Level Performance,"
        //in IEEE Communications Letters, vol. 22, no. 4, pp. 796-799, April 2018,
        //doi: 10.1109/LCOMM.2018.2797057.

        for(int LoRaSF = 7;LoRaSF <= 12;LoRaSF++)
        {
            if (snirThresholdMode == SnirThresholdMode::STM_MIN)
            {
                if(loradimensionalsnir->getLoRaInterfererPresent(LoRaSF))
                {
                    InterfererPresent = true;
                    if( W(loradimensionalsnir->getMinLoRa(LoRaSF)) < W(math::dBmW2mW(nonOrthDelta[loradimensionalReception->getLoRaSF()-7][LoRaSF-7])) )
                    {
                        EV_DEBUG << "Reception is not successful: Strong interference from SF " << LoRaSF << endl;
                        return false;
                    }
                }
            }
            else if (snirThresholdMode == SnirThresholdMode::STM_MEAN)
            {
                if(loradimensionalsnir->getLoRaInterfererPresent(LoRaSF))
                {
                    InterfererPresent = true;
                    if( W(loradimensionalsnir->getMeanLoRa(LoRaSF)) < W(math::dBmW2mW(nonOrthDelta[loradimensionalReception->getLoRaSF()-7][LoRaSF-7])) )
                    {
                        EV_DEBUG << "Reception is not successful: Strong interference from SF " << LoRaSF  << endl;
                        return false;
                    }
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

        if(loradimensionalsnir->getNonLoRaInterfererPresent() )
        {
            InterfererPresent = true;
            if (snirThresholdMode == SnirThresholdMode::STM_MIN)
            {
                if( W(loradimensionalsnir->getMinNonLoRa()) < snirNonLoRaThreshold )
                {
                    EV_DEBUG << "Reception is not successful: Strong interference from non lora interferers ";
                    return false;
                }
            }
            else if (snirThresholdMode == SnirThresholdMode::STM_MEAN)
            {
                if( W(loradimensionalsnir->getMeanNonLoRa()) <  snirNonLoRaThreshold )
                {
                    EV_DEBUG << "Reception is not successful: Strong interference from non lora interferers ";
                    return false;
                }
            }
            else
            {
                throw cRuntimeError("Unknown SNIR threshold mode: '%s'", snirThresholdMode);
            }
        }

        if(!InterfererPresent)
        {
            if (snirThresholdMode == SnirThresholdMode::STM_MIN)
            {
                if( W(loradimensionalsnir->getMin()) < W(math::dBmW2mW(AWGNDelta[loradimensionalReception->getLoRaSF()-7])))
                {
                    EV_DEBUG << "Reception is not successful. Low SNR on awgn reception " << endl;
                    return false;
                }

            }
            else if (snirThresholdMode == SnirThresholdMode::STM_MEAN)
            {
                InterfererPresent = true;
                if( W(loradimensionalsnir->getMean()) < W(math::dBmW2mW(AWGNDelta[loradimensionalReception->getLoRaSF()-7])) )
                {
                    EV_DEBUG << "Reception is not successful. Low SNR on awgn reception " << endl;
                    return false;
                }
            }
        }
        return true;
    }
    else if (lorafhsssnir)
    {
        //using the same criteria from
        //G. Boquet, P. Tuset-Peiró, F. Adelantado, T. Watteyne and X. Vilajosana,
        //"LR-FHSS: Overview and Performance Analysis,"
        //in IEEE Communications Magazine, vol. 59, no. 3, pp. 30-36, March 2021,
        //doi: 10.1109/MCOM.001.2000627.
        //but instead of considering only collisions, we check the SNR of each Header/Hop, comparing to a decoding threshold

        //from the paper:
        //..., the gateway device is able to reassemble a packet transmitted
        //using DR9 with high probability, even if 1 of the 2 headers and 1/3 of the bits within the payload fragments are lost.
        const int32_t headers = lorafhsssnir->getNumHeaders();
        int32_t failed_headers;

        const int32_t data_hops = lorafhsssnir->getNumHops() - headers;
        int32_t failed_hops;
        const auto lorafhsstransmission = dynamic_cast<const LoRaDimensionalFHSSTransmission *>(reception->getTransmission());



        if (snirThresholdMode == SnirThresholdMode::STM_MIN)
        {
            failed_headers = lorafhsssnir->getHeadersWithMinBelowThresold(snirNonLoRaThreshold.get());
            failed_hops = lorafhsssnir->getHopsWithMinBelowThresold(snirNonLoRaThreshold.get());
        }
        else if (snirThresholdMode == SnirThresholdMode::STM_MEAN)
        {
            failed_headers = lorafhsssnir->getHeadersWithMeanBelowThresold(snirNonLoRaThreshold.get());
            failed_hops = lorafhsssnir->getHopsWithMeanBelowThresold(snirNonLoRaThreshold.get());
        }
        else
        {
            throw cRuntimeError("Unknown SNIR threshold mode: '%s'", snirThresholdMode);
        }

        //check if at least one header was received
        if(failed_headers >= headers)
        {
            return false;
        }

        //the number of failed hops will depend on convolutional code rate used in FHSS
        //a better study of the code performance under erasure channels may be made in the future
        switch(lorafhsstransmission->getFHSSCR())
        {
            case LR_FHSS_V1_CR_1_3:
            {
                   if( (float)failed_hops/(float)data_hops > (2.0f/3.0f) )
                   {
                       return false;
                   }
                   else
                   {
                       return true;
                   }
            }
            case LR_FHSS_V1_CR_2_3:
            {
                if( (float)failed_hops/(float)data_hops > (1.0f/3.0f) )
                {
                    return false;
                }
                else
                {
                    return true;
                }
            }
            case LR_FHSS_V1_CR_1_2:
            case LR_FHSS_V1_CR_5_6:
            default:
            {
                throw cRuntimeError("Unimplemented code rate for FHSS receiver");
                return false;
            }
        }
    }
    throw cRuntimeError("Not a LoRa nor a LR-FHSS Transmission received in a LoRa Receiver. Something is wrong");
    return false; //not a lora nor a lora fhss transmission, don't know how the flow reached here
}

const IListening *LoRaDimensionalReceiver::createListening(const IRadio *radio, const simtime_t startTime, const simtime_t endTime, const Coord& startPosition, const Coord& endPosition) const
{
    return new LoRaBandListening(radio, startTime, endTime, startPosition, endPosition, centerFrequency, bandwidth, LoRaSF, iAmGateway);
}



const IReceptionResult *LoRaDimensionalReceiver::computeReceptionResult(const IListening *listening, const IReception *reception, const IInterference *interference, const ISnir *snir, const std::vector<const IReceptionDecision *> *decisions) const
{
    /** @brief Channel count as function of bandwidth index, from Table 9 specification v18 */
    const uint16_t lr_fhss_channel_count[] = { 80, 176, 280, 376, 688, 792, 1480, 1584, 3120, 3224 };


    auto receptionResult = FlatReceiverBase::computeReceptionResult(listening, reception, interference, snir, decisions);

    auto nbtransmission =  dynamic_cast<const NarrowbandTransmissionBase *>(reception->getTransmission());
    auto loratransmission = dynamic_cast<const LoRaDimensionalTransmission *>(reception->getTransmission());
    auto lorafhsstransmission = dynamic_cast<const LoRaDimensionalFHSSTransmission *>(reception->getTransmission());

    auto bandparameters = const_cast<Packet *>(receptionResult->getPacket())->addTagIfAbsent<SignalBandInd>();
    bandparameters->setBandwidth(nbtransmission->getBandwidth());
    bandparameters->setCenterFrequency(nbtransmission->getCenterFrequency());

    if(loratransmission)
    {
        auto loraparameters = const_cast<Packet *>(receptionResult->getPacket())->addTagIfAbsent<LoRaParamsInd>();
        loraparameters->setLoRaSF(loratransmission->getLoRaSF());
        loraparameters->setLoRaCR(loratransmission->getLoRaCR());
    }
    else if(lorafhsstransmission)
    {
        auto loraparameters = const_cast<Packet *>(receptionResult->getPacket())->addTagIfAbsent<LoRaFHSSParamsInd>();
        uint32_t channel_count = lr_fhss_channel_count[lorafhsstransmission->getFHSSBW()];
        if( lorafhsstransmission->getFHSSGrid() == LR_FHSS_V1_GRID_3906_HZ )
        {
            loraparameters->setHPW(channel_count / 8);
        }
        else
        {
            loraparameters->setHPW(channel_count / 52);
        }
        loraparameters->setFHSSCR(lorafhsstransmission->getFHSSCR());
        loraparameters->setFHSSBW(lorafhsstransmission->getFHSSBW());
    }
    return receptionResult;
}

const IReceptionDecision *LoRaDimensionalReceiver::computeReceptionDecision(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference, const ISnir *snir) const
{
    const BandListening *bandListening = check_and_cast<const BandListening *>(listening);
    const NarrowbandReceptionBase *narrowbandReception = check_and_cast<const NarrowbandReceptionBase *>(reception);
    bool GatewayRxPossible = false;
    if(iAmGateway)
    {
        Hz GWminFreq = bandListening->getCenterFrequency() - bandListening->getBandwidth()/2;
        Hz GWmaxFreq = bandListening->getCenterFrequency() + bandListening->getBandwidth()/2;
        Hz TxminFreq = narrowbandReception->getCenterFrequency() - narrowbandReception->getBandwidth()/2;
        Hz TxMaxFreq = narrowbandReception->getCenterFrequency() + narrowbandReception->getBandwidth()/2;
        GatewayRxPossible = (GWminFreq <= TxminFreq) && (GWmaxFreq >= TxMaxFreq);
    }

    if (GatewayRxPossible || ((bandListening->getCenterFrequency() == narrowbandReception->getCenterFrequency() && bandListening->getBandwidth() == narrowbandReception->getBandwidth())))
    {
        auto isReceptionPossible = computeIsReceptionPossible(listening, reception, part);
        auto isReceptionAttempted = isReceptionPossible && computeIsReceptionAttempted(listening, reception, part, interference);
        auto isReceptionSuccessful = isReceptionAttempted && computeIsReceptionSuccessful(listening, reception, part, interference, snir);
        return new ReceptionDecision(reception, part, isReceptionPossible, isReceptionAttempted, isReceptionSuccessful);
    }
    else
    {
        return new ReceptionDecision(reception, part, false, false, false);
    }
}




} // namespace physicallayer

} // namespace labscim

