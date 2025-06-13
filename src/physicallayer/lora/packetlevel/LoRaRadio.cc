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

#include "inet/common/ProtocolTag_m.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/packet/chunk/BitCountChunk.h"
#include "inet/physicallayer/wireless/apsk/bitlevel/ApskEncoder.h"
#include "inet/physicallayer/wireless/apsk/bitlevel/ApskLayeredTransmitter.h"
#include "inet/physicallayer/wireless/apsk/packetlevel/ApskPhyHeader_m.h"
#include "inet/physicallayer/wireless/apsk/packetlevel/ApskRadio.h"
#include "inet/physicallayer/wireless/common/base/packetlevel/FlatTransmitterBase.h"
#include "inet/physicallayer/wireless/common/medium/RadioMedium.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/Simsignals.h"
#include "LoRaRadio.h"
#include "LoRaDimensionalTransmitter.h"
#include "LoRaDimensionalReceiver.h"
#include "LoRaRadioControlInfo_m.h"
#include "../../../common/labscim_sx126x.h"
#include "LoRaFHSSHopEntry.h"

using namespace inet;
using namespace inet::physicallayer;

namespace labscim {
namespace physicallayer {

Define_Module(LoRaRadio);

simsignal_t LoRaRadio::loraradio_datarate_changed = cComponent::registerSignal("loraradio_datarate_changed");

LoRaRadio::LoRaRadio() :
            FlatRadioBase(),
            mConfiguringRadio(false)
{
}

void LoRaRadio::initialize(int stage)
{
    FlatRadioBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL)
    {
        LoRaSF = par("LoRaSF");
        LoRaCR = par("LoRaCR");
        iAmGateway = par("iAmGateway");
        LoRaTransmitter = check_and_cast<LoRaDimensionalTransmitter *>(getSubmodule("transmitter"));
        LoRaTransmitter->setIamGateway(iAmGateway);
        LoRaReceiver = check_and_cast<LoRaDimensionalReceiver *>(getSubmodule("receiver"));
        LoRaReceiver->setIamGateway(iAmGateway);
    }
}

void LoRaRadio::setLoRaModulationMode(sx126x_pkt_types_e mode)
{
    LoRaTransmitter->setLoRaModulationMode(mode);
}

sx126x_pkt_types_e LoRaRadio::getLoRaModulationMode()
{
    return LoRaTransmitter->getLoRaModulationMode();
}

Hz LoRaRadio::FHSSBandwidthToHz(lr_fhss_v1_bw_t bw)
{
    switch(bw)
    {
    case LR_FHSS_V1_BW_39063_HZ:
        return Hz(39063);
    case LR_FHSS_V1_BW_85938_HZ:
        return Hz(85938);
    case LR_FHSS_V1_BW_136719_HZ:
        return Hz(136719);
    case LR_FHSS_V1_BW_183594_HZ:
        return Hz(183594);
    case LR_FHSS_V1_BW_335938_HZ:
        return Hz(335938);
    case LR_FHSS_V1_BW_386719_HZ:
        return Hz(386719);
    case LR_FHSS_V1_BW_722656_HZ:
        return Hz(722656);
    case LR_FHSS_V1_BW_773438_HZ:
        return Hz(773438);
    case LR_FHSS_V1_BW_1523438_HZ:
        return Hz(1523438);
    case LR_FHSS_V1_BW_1574219_HZ:
    default:
        return Hz(1574219);
    }
}


void LoRaRadio::handleUpperCommand(cMessage *message)
{
    mConfiguringRadio = true;
    bool datarate_changed=false;

    if (message->getKind() == RADIO_C_CONFIGURE)
    {
        ConfigureLoRaRadioCommand* configureCommand =  dynamic_cast<ConfigureLoRaRadioCommand *>(message->getControlInfo());

        if(configureCommand!=nullptr)
        {
            if (configureCommand->getLoRaSF() != -1)
            {
                if(getLoRaSF()!=configureCommand->getLoRaSF())
                {
                    setLoRaSF(configureCommand->getLoRaSF());
                    datarate_changed = true;
                }
            }
            if (configureCommand->getLoRaCR() != -1)
            {
                if(getLoRaCR()!=configureCommand->getLoRaCR())
                {
                    setLoRaCR(configureCommand->getLoRaCR());
                    datarate_changed = true;
                }
            }

            if (configureCommand->getFHSSCR() != -1)
            {
                if(getFHSSCR()!=configureCommand->getFHSSCR())
                {
                    setFHSSCR((lr_fhss_v1_cr_t)configureCommand->getFHSSCR());
                    //datarate_changed = true; //we should set, but we are not setting by now
                }
            }

            if (configureCommand->getFHSSBW() != -1)
            {
                if(LoRaTransmitter->getFHSSBW()!=configureCommand->getFHSSBW())
                {
                    LoRaTransmitter->setFHSSBW((lr_fhss_v1_bw_t)configureCommand->getFHSSBW());
                    //datarate_changed = true; //we should set, but we are not setting by now
                }
                configureCommand->setBandwidth(FHSSBandwidthToHz((lr_fhss_v1_bw_t)configureCommand->getFHSSBW()));
            }

            if (configureCommand->getFHSSGrid() != -1)
            {
                if(LoRaTransmitter->getFHSSGrid()!=configureCommand->getFHSSGrid())
                {
                    LoRaTransmitter->setFHSSGrid((lr_fhss_v1_grid_t)configureCommand->getFHSSGrid());
                }
            }


            NarrowbandTransmitterBase *narrowbandTransmitter = const_cast<NarrowbandTransmitterBase *>(check_and_cast<const NarrowbandTransmitterBase *>(transmitter));

            if(!std::isnan(configureCommand->getBandwidth().get()))
            {
                if(configureCommand->getBandwidth()!=narrowbandTransmitter->getBandwidth())
                {
                    datarate_changed = true;
                }
            }

            if(configureCommand->getPreamble_length()!=-1)
            {
                LoRaTransmitter->setPreamble_length(configureCommand->getPreamble_length());
            }

            if(configureCommand->getCRC_enabled()!=-1)
            {
                LoRaTransmitter->setLoRaCRC_enabled(configureCommand->getCRC_enabled());
            }

            if(configureCommand->getHeader_enabled()!=-1)
            {
                LoRaTransmitter->setLoRaHeader_enabled(configureCommand->getHeader_enabled());
            }
        }
    }
    FlatRadioBase::handleUpperCommand(message);
    if(datarate_changed)
    {
        emit(loraradio_datarate_changed, LoRaTransmitter->getPacketDataRate().get());
    }
    mConfiguringRadio = false;
}

bool LoRaRadio::compareArrivals(cMessage* i1, cMessage* i2)
{
    return (i1->getArrivalTime() < i2->getArrivalTime());
}

void LoRaRadio::setHoppingSequence(std::vector<LoRaFHSSHopEntry>& HopTable)
{
        LoRaTransmitter->setHoppingSequence(HopTable);
}

void LoRaRadio::startReception(cMessage *timer, IRadioSignal::SignalPart part)
{
    if(iAmGateway)
    {
        auto signal = static_cast<WirelessSignal *>(timer->getControlInfo());
        auto arrival = signal->getArrival();
        auto reception = signal->getReception();
        // TODO: should be this, but it breaks fingerprints: if (receptionTimer == nullptr && isReceiverMode(radioMode) && arrival->getStartTime(part) == simTime()) {
        if (isReceiverMode(radioMode) && arrival->getStartTime(part) == simTime()) {
            auto transmission = signal->getTransmission();
            auto isReceptionAttempted = medium->isReceptionAttempted(this, transmission, part);
            EV_INFO << "Reception started: " << (isReceptionAttempted ? "\x1b[1mattempting\x1b[0m" : "\x1b[1mnot attempting\x1b[0m") << " " << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
            if (isReceptionAttempted)
            {
                concurrentReceptions.push_back(timer);
                emit(receptionStartedSignal, check_and_cast<const cObject *>(reception));
            }
        }
        else
            EV_INFO << "Reception started: \x1b[1mignoring\x1b[0m " << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
        timer->setKind(part);
        scheduleAt(arrival->getEndTime(part), timer);
        updateReceptionTimer();
        updateTransceiverState();
        updateTransceiverPart();

        // TODO: move to radio medium
        check_and_cast<RadioMedium *>(medium.get())->emit(IRadioMedium::signalArrivalStartedSignal, check_and_cast<const cObject *>(reception));
    }
    else
    {
        FlatRadioBase::startReception(timer, part);
    }
}


void LoRaRadio::continueReception(cMessage *timer)
{
    if(iAmGateway)
    {
        auto previousPart = (IRadioSignal::SignalPart)timer->getKind();
        auto nextPart = (IRadioSignal::SignalPart)(previousPart + 1);
        auto signal = static_cast<WirelessSignal *>(timer->getControlInfo());
        auto arrival = signal->getArrival();
        auto reception = signal->getReception();
        std::list<cMessage *>::iterator it = std::find(concurrentReceptions.begin(), concurrentReceptions.end(), timer);

        if((it != concurrentReceptions.end()) && ( timer == *it && isReceiverMode(radioMode) && arrival->getEndTime(previousPart) == simTime()))
        {
            auto transmission = signal->getTransmission();
            bool isReceptionSuccessful = medium->isReceptionSuccessful(this, transmission, previousPart);
            EV_INFO << "Reception ended: " << (isReceptionSuccessful ? "\x1b[1msuccessfully\x1b[0m" : "\x1b[1munsuccessfully\x1b[0m") << " for " << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(previousPart) << " as " << reception << endl;
            if (!isReceptionSuccessful)
            {
                concurrentReceptions.remove(timer);
            }
            auto isReceptionAttempted = medium->isReceptionAttempted(this, transmission, nextPart);
            EV_INFO << "Reception started: " << (isReceptionAttempted ? "\x1b[1mattempting\x1b[0m" : "\x1b[1mnot attempting\x1b[0m") << " " << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(nextPart) << " as " << reception << endl;
            if (!isReceptionAttempted)
            {
                concurrentReceptions.remove(timer);
            }
            // TODO: FIXME: see handling packets with incorrect PHY headers in the TODO file
        }
        else
        {
            EV_INFO << "Reception ended: \x1b[1mignoring\x1b[0m " << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(previousPart) << " as " << reception << endl;
            EV_INFO << "Reception started: \x1b[1mignoring\x1b[0m " << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(nextPart) << " as " << reception << endl;
        }
        timer->setKind(nextPart);
        scheduleAt(arrival->getEndTime(nextPart), timer);
        updateReceptionTimer();
        updateTransceiverState();
        updateTransceiverPart();
    }
    else
    {
        FlatRadioBase::continueReception(timer);
    }
}


void LoRaRadio::endReception(cMessage *timer)
{
    if(iAmGateway)
    {
        auto part = (IRadioSignal::SignalPart)timer->getKind();
        auto signal = static_cast<WirelessSignal *>(timer->getControlInfo());
        auto arrival = signal->getArrival();
        auto reception = signal->getReception();
        std::list<cMessage *>::iterator it;

        if (timer == receptionTimer && isReceiverMode(radioMode) && arrival->getEndTime() == simTime()) {
            auto transmission = signal->getTransmission();
            // TODO: this would draw twice from the random number generator in isReceptionSuccessful: auto isReceptionSuccessful = medium->isReceptionSuccessful(this, transmission, part);
            auto isReceptionSuccessful = medium->getReceptionDecision(this, signal->getListening(), transmission, part)->isReceptionSuccessful();
            EV_INFO << "Reception ended: " << (isReceptionSuccessful ? "\x1b[1msuccessfully\x1b[0m" : "\x1b[1munsuccessfully\x1b[0m") << " for " << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
            auto macFrame = medium->receivePacket(this, signal);
            take(macFrame);
            // TODO: FIXME: see handling packets with incorrect PHY headers in the TODO file
            decapsulate(macFrame);
            sendUp(macFrame);
            receptionTimer = nullptr;
            emit(receptionEndedSignal, check_and_cast<const cObject *>(reception));
        }
        else
            EV_INFO << "Reception ended: \x1b[1mignoring\x1b[0m " << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;

        concurrentReceptions.remove(timer);
        updateReceptionTimer();
        updateTransceiverState();
        updateTransceiverPart();
        delete timer;
        // TODO: move to radio medium
        check_and_cast<RadioMedium *>(medium.get())->emit(IRadioMedium::signalArrivalEndedSignal, check_and_cast<const cObject *>(reception));
    }
    else
    {
        FlatRadioBase::endReception(timer);
    }
}


void LoRaRadio::abortReception(cMessage *timer)
{
    if(iAmGateway)
    {
        std::list<cMessage *>::iterator it;
        for (it=concurrentReceptions.begin(); it!=concurrentReceptions.end(); it++)
        {
            auto timer = *it;
            auto signal = static_cast<WirelessSignal *>(timer->getControlInfo());
            auto part = (IRadioSignal::SignalPart)timer->getKind();
            auto reception = signal->getReception();
            EV_INFO << "Reception \x1b[1maborted\x1b[0m: for " << (IWirelessSignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
        }
        concurrentReceptions.clear();
        updateReceptionTimer();
        updateTransceiverState();
        updateTransceiverPart();
    }
    else
    {
        FlatRadioBase::abortReception(timer);
    }
}

void LoRaRadio::updateReceptionTimer()
{
    if(iAmGateway)
    {
        if(concurrentReceptions.empty())
        {
            receptionTimer = nullptr;
        }
        else
        {
            concurrentReceptions.sort(LoRaRadio::compareArrivals);
            receptionTimer = concurrentReceptions.front();
        }
    }
}

void LoRaRadio::setLoRaSF(int LoRaSF)
{
    //TODO: abort transmission and reception?
    this->LoRaSF = LoRaSF;
    LoRaTransmitter->setLoRaSF(LoRaSF);
    LoRaReceiver->setLoRaSF(LoRaSF);
    if(!mConfiguringRadio)
        emit(loraradio_datarate_changed, LoRaTransmitter->getPacketDataRate().get());
}

void LoRaRadio::setLoRaCR(int LoRaCR)
{
    this->LoRaCR = LoRaCR;
    LoRaTransmitter->setLoRaCR(LoRaCR);
    LoRaReceiver->setLoRaCR(LoRaCR);
    if(!mConfiguringRadio)
        emit(loraradio_datarate_changed, LoRaTransmitter->getPacketDataRate().get());
}

void LoRaRadio::setFHSSCR(lr_fhss_v1_cr_t FHSSCR)
{
    LoRaTransmitter->setFHSSCR(FHSSCR);
    //if(!mConfiguringRadio) //TODO: how this can work for FHSS?
    //    emit(loraradio_datarate_changed, LoRaTransmitter->getPacketDataRate().get());
}

lr_fhss_v1_cr_t LoRaRadio::getFHSSCR() const
{
    return LoRaTransmitter->getFHSSCR();
}


bps LoRaRadio::getPacketDataRate(const Packet *packet) const
{
    return LoRaTransmitter->getPacketDataRate(packet);
}

bps LoRaRadio::getPacketDataRate() const
{
    return LoRaTransmitter->getPacketDataRate();
}


simtime_t LoRaRadio::getPacketRadioTimeOnAir( const Packet *packet )
{
    return LoRaTransmitter->getPacketRadioTimeOnAir(packet);
}


} // namespace physicallayer
} // namespace inet

