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
#include "inet/physicallayer/apskradio/bitlevel/ApskEncoder.h"
#include "inet/physicallayer/apskradio/bitlevel/ApskLayeredTransmitter.h"
#include "inet/physicallayer/apskradio/packetlevel/ApskPhyHeader_m.h"
#include "inet/physicallayer/apskradio/packetlevel/ApskRadio.h"
#include "inet/physicallayer/base/packetlevel/FlatTransmitterBase.h"
#include "inet/physicallayer/common/packetlevel/RadioMedium.h"
#include "LoRaRadio.h"
#include "LoRaDimensionalTransmitter.h"
#include "LoRaDimensionalReceiver.h"
#include "LoRaRadioControlInfo_m.h"

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



void LoRaRadio::startReception(cMessage *timer, IRadioSignal::SignalPart part)
{
    if(iAmGateway)
    {
        auto signal = static_cast<Signal *>(timer->getControlInfo());
        auto arrival = signal->getArrival();
        auto reception = signal->getReception();
        // TODO: should be this, but it breaks fingerprints: if (receptionTimer == nullptr && isReceiverMode(radioMode) && arrival->getStartTime(part) == simTime()) {
        if (isReceiverMode(radioMode) && arrival->getStartTime(part) == simTime()) {
            auto transmission = signal->getTransmission();
            auto isReceptionAttempted = medium->isReceptionAttempted(this, transmission, part);
            EV_INFO << "Reception started: " << (isReceptionAttempted ? "\x1b[1mattempting\x1b[0m" : "\x1b[1mnot attempting\x1b[0m") << " " << (ISignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
            if (isReceptionAttempted)
            {
                concurrentReceptions.push_back(timer);
                emit(receptionStartedSignal, check_and_cast<const cObject *>(reception));
            }
        }
        else
            EV_INFO << "Reception started: \x1b[1mignoring\x1b[0m " << (ISignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
        timer->setKind(part);
        scheduleAt(arrival->getEndTime(part), timer);
        updateReceptionTimer();
        updateTransceiverState();
        updateTransceiverPart();

        // TODO: move to radio medium
        check_and_cast<RadioMedium *>(medium)->emit(IRadioMedium::signalArrivalStartedSignal, check_and_cast<const cObject *>(reception));
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
        auto signal = static_cast<Signal *>(timer->getControlInfo());
        auto arrival = signal->getArrival();
        auto reception = signal->getReception();
        std::list<cMessage *>::iterator it = std::find(concurrentReceptions.begin(), concurrentReceptions.end(), timer);

        if((it != concurrentReceptions.end()) && ( timer == *it && isReceiverMode(radioMode) && arrival->getEndTime(previousPart) == simTime()))
        {
            auto transmission = signal->getTransmission();
            bool isReceptionSuccessful = medium->isReceptionSuccessful(this, transmission, previousPart);
            EV_INFO << "Reception ended: " << (isReceptionSuccessful ? "\x1b[1msuccessfully\x1b[0m" : "\x1b[1munsuccessfully\x1b[0m") << " for " << (ISignal *)signal << " " << IRadioSignal::getSignalPartName(previousPart) << " as " << reception << endl;
            if (!isReceptionSuccessful)
            {
                concurrentReceptions.remove(timer);
            }
            auto isReceptionAttempted = medium->isReceptionAttempted(this, transmission, nextPart);
            EV_INFO << "Reception started: " << (isReceptionAttempted ? "\x1b[1mattempting\x1b[0m" : "\x1b[1mnot attempting\x1b[0m") << " " << (ISignal *)signal << " " << IRadioSignal::getSignalPartName(nextPart) << " as " << reception << endl;
            if (!isReceptionAttempted)
            {
                concurrentReceptions.remove(timer);
            }
            // TODO: FIXME: see handling packets with incorrect PHY headers in the TODO file
        }
        else
        {
            EV_INFO << "Reception ended: \x1b[1mignoring\x1b[0m " << (ISignal *)signal << " " << IRadioSignal::getSignalPartName(previousPart) << " as " << reception << endl;
            EV_INFO << "Reception started: \x1b[1mignoring\x1b[0m " << (ISignal *)signal << " " << IRadioSignal::getSignalPartName(nextPart) << " as " << reception << endl;
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
        auto signal = static_cast<Signal *>(timer->getControlInfo());
        auto arrival = signal->getArrival();
        auto reception = signal->getReception();
        std::list<cMessage *>::iterator it;

        if (timer == receptionTimer && isReceiverMode(radioMode) && arrival->getEndTime() == simTime()) {
            auto transmission = signal->getTransmission();
            // TODO: this would draw twice from the random number generator in isReceptionSuccessful: auto isReceptionSuccessful = medium->isReceptionSuccessful(this, transmission, part);
            auto isReceptionSuccessful = medium->getReceptionDecision(this, signal->getListening(), transmission, part)->isReceptionSuccessful();
            EV_INFO << "Reception ended: " << (isReceptionSuccessful ? "\x1b[1msuccessfully\x1b[0m" : "\x1b[1munsuccessfully\x1b[0m") << " for " << (ISignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
            auto macFrame = medium->receivePacket(this, signal);
            // TODO: FIXME: see handling packets with incorrect PHY headers in the TODO file
            decapsulate(macFrame);
            sendUp(macFrame);
            receptionTimer = nullptr;
            emit(receptionEndedSignal, check_and_cast<const cObject *>(reception));
        }
        else
            EV_INFO << "Reception ended: \x1b[1mignoring\x1b[0m " << (ISignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;

        concurrentReceptions.remove(timer);
        updateReceptionTimer();
        updateTransceiverState();
        updateTransceiverPart();
        delete timer;
        // TODO: move to radio medium
        check_and_cast<RadioMedium *>(medium)->emit(IRadioMedium::signalArrivalEndedSignal, check_and_cast<const cObject *>(reception));
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
            auto signal = static_cast<Signal *>(timer->getControlInfo());
            auto part = (IRadioSignal::SignalPart)timer->getKind();
            auto reception = signal->getReception();
            EV_INFO << "Reception \x1b[1maborted\x1b[0m: for " << (ISignal *)signal << " " << IRadioSignal::getSignalPartName(part) << " as " << reception << endl;
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

