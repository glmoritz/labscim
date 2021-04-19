//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 2003 Ahmet Sekercioglu
// Copyright (C) 2003-2015 Andras Varga
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//

#include <string.h>
#include <omnetpp.h>
#include "LabscimRadioRecorder.h"
#include "inet/physicallayer/contract/packetlevel/IRadio.h"
#include "inet/physicallayer/base/packetlevel/FlatTransmissionBase.h"
#include "inet/physicallayer/base/packetlevel/FlatReceptionBase.h"
#include "inet/physicallayer/base/packetlevel/FlatRadioBase.h"
#include "inet/physicallayer/base/packetlevel/FlatReceiverBase.h"
#include "inet/physicallayer/contract/packetlevel/SignalTag_m.h"
#include "inet/linklayer/base/MacProtocolBase.h"
#include "inet/common/Simsignals.h"
#include "inet/common/packet/Message.h"

using namespace omnetpp;
using namespace inet;
using namespace inet::physicallayer;
using namespace std;

namespace labscim {

namespace physicallayer {

// The module class needs to be registered with OMNeT++
Define_Module(LabscimRadioRecorder);

void LabscimRadioRecorder::initialize(int stage)
{
    if(stage == INITSTAGE_LOCAL)
    {
        std::string name = par("LogName");
        LogFile.open(name);
    }
    else if(stage == INITSTAGE_LAST)
    {
        getSimulation()->getSystemModule()->subscribe(IRadio::radioModeChangedSignal, this);
        getSimulation()->getSystemModule()->subscribe(IRadio::transmissionEndedSignal, this);
        getSimulation()->getSystemModule()->subscribe(IRadio::receptionEndedSignal, this);
        getSimulation()->getSystemModule()->subscribe(IRadio::receptionStateChangedSignal, this);
        getSimulation()->getSystemModule()->subscribe(IRadio::transmissionStateChangedSignal, this);
        getSimulation()->getSystemModule()->subscribe(IRadio::receptionStartedSignal, this);
        getSimulation()->getSystemModule()->subscribe(packetSentToUpperSignal, this);


    }
}

LabscimRadioRecorder::~LabscimRadioRecorder()
{
    if(LogFile.is_open())
    {
        LogFile.close();
    }
}

void LabscimRadioRecorder::receiveSignal(cComponent *source, simsignal_t signalID, long l, cObject *details)
{

    if(signalID == IRadio::radioModeChangedSignal)
    {
        FlatRadioBase* radio = check_and_cast<FlatRadioBase *>(source);

        LogFile << signalID << " ," << simTime().dbl() <<",IRadio::radioModeChangedSignal, " <<  source->getFullPath().c_str();
        LogFile << "," << l;
        if(l == IRadio::RadioMode::RADIO_MODE_RECEIVER || l == IRadio::RadioMode::RADIO_MODE_TRANSCEIVER)
        {
            const FlatReceiverBase* receiver = check_and_cast<const FlatReceiverBase *>(radio->getReceiver());
            if(receiver!=nullptr)
            {
                LogFile << "," << receiver->getCenterFrequency().get() << "," << receiver->getBandwidth().get() << "\n";
            }
        }
        else
        {
            LogFile << "\n";
        }

    }
    else if(signalID == IRadio::receptionStateChangedSignal)
    {

        FlatRadioBase* radio = check_and_cast<FlatRadioBase *>(source);

        LogFile << signalID << " ," << simTime().dbl() <<",IRadio::receptionStateChangedSignal, " << source->getFullPath().c_str();
        LogFile << "," << l;
        if(l == IRadio::ReceptionState::RECEPTION_STATE_RECEIVING)
        {
            const FlatReceiverBase* receiver = check_and_cast<const FlatReceiverBase *>(radio->getReceiver());

            if(receiver!=nullptr)
            {
                LogFile << "," << receiver->getCenterFrequency().get() << "," << receiver->getBandwidth().get() << "\n";
            }
        }
        else
        {
            LogFile << "\n";
        }

    }
    else if(signalID == IRadio::transmissionStateChangedSignal)
    {
        LogFile << signalID << " ," << simTime().dbl() <<",IRadio::transmissionStateChangedSignal, " << source->getFullPath().c_str();
        LogFile << "," << l << "\n";

    }
}

void LabscimRadioRecorder::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    if(signalID == IRadio::transmissionEndedSignal)
    {
        const FlatTransmissionBase* transmission = check_and_cast<const FlatTransmissionBase *>(obj);
        if(transmission!=nullptr)
        {
            LogFile << signalID << " ," << simTime().dbl() <<",IRadio::transmissionEndedSignal, " << source->getFullPath().c_str();
            LogFile << "," << transmission->getStartTime() << "," << transmission->getEndTime() << "," << transmission->getCenterFrequency().get() << "," << transmission->getBandwidth().get() << "\n";
        }
    }
    else if(signalID == IRadio::receptionStartedSignal)
    {
        const FlatReceptionBase* reception = check_and_cast<const FlatReceptionBase *>(obj);
        if(reception!=nullptr)
        {
            const FlatTransmissionBase* transmission = check_and_cast<const FlatTransmissionBase *>(reception->getTransmission());
            if(transmission!=nullptr)
            {
                LogFile << signalID << " ," << simTime().dbl() <<",IRadio::receptionStartedSignal, " << source->getFullPath().c_str();
                LogFile << "," << reception->getStartTime() << "," << reception->getCenterFrequency().get() << "," << reception->getBandwidth().get() << "\n";
            }
        }
    }
    else if(signalID == IRadio::receptionEndedSignal)
    {
        const FlatReceptionBase* reception = check_and_cast<const FlatReceptionBase *>(obj);
        if(reception!=nullptr)
        {
            const FlatTransmissionBase* transmission = check_and_cast<const FlatTransmissionBase *>(reception->getTransmission());
            if(transmission!=nullptr)
            {
                LogFile << signalID << " ," << simTime().dbl() <<",IRadio::receptionEndedSignal, " << source->getFullPath().c_str();
                LogFile << "," << reception->getStartTime() << "," << reception->getEndTime() << "," << reception->getCenterFrequency().get() << "," << reception->getBandwidth().get() << "\n";
            }
        }
    }
    else if(signalID == packetSentToUpperSignal)
    {
        const FlatRadioBase* radio = check_and_cast<const FlatRadioBase *>(source);

        if(radio!=nullptr)
        {
            const Packet* packet = check_and_cast<const Packet *>(obj);
            if(packet!=nullptr)
            {
                int64_t frequency=0,bandwidth=0;
                float power=0,snr=0;

                if (packet->findTag<SignalBandInd>() != nullptr) {
                    auto signalBandInd = packet->getTag<SignalBandInd>();
                    frequency = signalBandInd->getCenterFrequency().get();
                    bandwidth = signalBandInd->getBandwidth().get();
                }

                if (packet->findTag<SignalPowerInd>() != nullptr) {
                    auto signalPowerInd = packet->getTag<SignalPowerInd>();
                    power = math::mW2dBmW(signalPowerInd->getPower().get());
                }

                if (packet->findTag<SnirInd>() != nullptr)
                {
                    auto snir = packet->getTag<SnirInd>();
                    double snr = math::mW2dBmW(snir->getAverageSnir());
                }
                LogFile << signalID << " ," << simTime().dbl() <<",packetSentToUpperSignal, " << source->getFullPath().c_str();
                LogFile << "," << packet->getArrivalTime().dbl() << "," << frequency << "," << bandwidth << "," << power << "," << snr << "\n";
            }
        }
    }
}

} // namespace physicallayer
} // namespace labscim

