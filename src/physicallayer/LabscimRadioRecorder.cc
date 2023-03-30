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
#include <iomanip>
#include <iostream>
#include <omnetpp.h>
#include "LabscimRadioRecorder.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h"
#include "inet/physicallayer/wireless/common/base/packetlevel/FlatTransmissionBase.h"
#include "inet/physicallayer/wireless/common/base/packetlevel/FlatReceptionBase.h"
#include "inet/physicallayer/wireless/common/base/packetlevel/FlatRadioBase.h"
#include "inet/physicallayer/wireless/common/base/packetlevel/FlatReceiverBase.h"

#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/DimensionalReception.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/DimensionalTransmission.h"

#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"
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
        LogEnabled = par("EnableLog").boolValue();
        std::string name = par("LogName");
        std::string spectrumname = par("SpectrumLogName");

        mSpectrumRecorderUpperThreshold = WpHz(math::dBmWpMHz2WpHz(par("SpectrumRecorderUpperThreshold")));
        mSpectrumRecorderLowerThreshold = WpHz(math::dBmWpMHz2WpHz(par("SpectrumRecorderLowerThreshold")));


        if(LogEnabled)
        {
            LogFile.open(name);
            LogFile << std::fixed;
            SpectrumFile.open(spectrumname);
        }
    }
    else if(stage == INITSTAGE_LAST)
    {
        if(LogEnabled)
        {
            getSimulation()->getSystemModule()->subscribe(IRadio::radioModeChangedSignal, this);
            getSimulation()->getSystemModule()->subscribe(transmissionEndedSignal, this);
            getSimulation()->getSystemModule()->subscribe(receptionEndedSignal, this);
            getSimulation()->getSystemModule()->subscribe(IRadio::receptionStateChangedSignal, this);
            getSimulation()->getSystemModule()->subscribe(IRadio::transmissionStateChangedSignal, this);
            getSimulation()->getSystemModule()->subscribe(receptionStartedSignal, this);
            getSimulation()->getSystemModule()->subscribe(packetSentToUpperSignal, this);

            std::string radiopath = par("SpectrumRadioReceiverPath").str();
            if(radiopath.size() >= 2 )
            {
                radiopath = radiopath.substr(1, radiopath.size() - 2);
            }
            std::string targetradiopath = getSimulation()->getSystemModule()->getFullPath() + std::string(".") + radiopath;

            SpectrumRadioReceiver = dynamic_cast<inet::physicallayer::FlatRadioBase *>(getModuleByPath(targetradiopath.c_str()));
            if(SpectrumRadioReceiver)
            {
                SpectrumRadioReceiver->subscribe(receptionStartedSignal, this);
                SpectrumRadioReceiver->subscribe(transmissionStartedSignal, this);
            }
        }
    }
}

void LabscimRadioRecorder::writeConstantPowerValue(double tstart, double tend, double centerfrequency, double bandwidth, double power)
{
    SpectrumFile << std::fixed << setprecision(13);
    SpectrumFile << "POW, " <<  tstart << "," <<  tend << "," <<  centerfrequency << "," <<  bandwidth << "," <<  power << "\n";
}


void LabscimRadioRecorder::writeConstantValue(double x1, double x2, double y1, double y2, double v)
{
    SpectrumFile << "CP, " <<  x1 << "," <<  x2 << "," <<  y1 << "," <<  y2 << "," <<  v << "\n";
}

void LabscimRadioRecorder::writeLinearValue(double x1, double x2, double y1, double y2, double vl, double vu, int axis)
{
    SpectrumFile << "LP, " <<  x1 << "," <<  x2 << "," <<  y1 << "," <<  y2 << "," <<  vl << "," <<  vu << "," <<  axis << "\n";
}

void LabscimRadioRecorder::writeBilinearValue(double x1, double x2, double y1, double y2, double v11, double v21, double v12, double v22)
{
    SpectrumFile << "BP, " <<  x1 << "," <<  x2 << "," <<  y1 << "," <<  y2 << "," <<  v11 << "," <<  v21 << "," <<  v12 << "," <<  v22  << "\n";
}


LabscimRadioRecorder::~LabscimRadioRecorder()
{
    if(SpectrumFile.is_open())
    {
        auto l = spectrumPower.getDomain().getLower();
        auto u = spectrumPower.getDomain().getUpper();
        Interval<simsec, Hz> i(l, u, 0b1, 0b0, 0b0);

        spectrumPower.partition(i, [&] (const Interval<simsec, Hz>& j, const IFunction<WpHz, math::Domain<simsec, Hz>> *partitonPowerFunction)
        {
            auto lower = j.getLower();
            auto upper = j.getUpper();
            auto y1 = Hz(std::get<1>(lower));
            auto y2 = Hz(std::get<1>(upper));
            auto x1 = ((std::get<0>(lower)).get()).dbl();
            auto x2 = ((std::get<0>(upper)).get()).dbl();
            if (auto cf = dynamic_cast<const ConstantFunction<WpHz, math::Domain<simsec, Hz>> *>(partitonPowerFunction)) {
                auto v = cf->getConstantValue();
                if(v>mSpectrumRecorderUpperThreshold)
                {
                    v = mSpectrumRecorderUpperThreshold;
                }
                if(v>mSpectrumRecorderLowerThreshold)
                {
                    Hz Bandwidth = y2 - y1;
                    Hz Centerfrequency = y1 + Bandwidth/2;
                    writeConstantPowerValue(x1,x2,Centerfrequency.get(),Bandwidth.get(), wpHz2dBmWpMHz( v.get() ));
                }
            }
            else if (auto lf = dynamic_cast<const UnilinearFunction<WpHz, math::Domain<simsec, Hz>> *>(partitonPowerFunction)) {
                auto vl = lf->getRLower() == WpHz(0) ? -INFINITY : wpHz2dBmWpMHz(WpHz(lf->getRLower()).get());
                auto vu = lf->getRUpper() == WpHz(0) ? -INFINITY : wpHz2dBmWpMHz(WpHz(lf->getRUpper()).get());
                //TODO: interpolate the thresholds
                writeLinearValue(x1, x2, y1.get(), y2.get(), vl, vu, 2 - lf->getDimension());
            }
            else if (auto bf = dynamic_cast<const BilinearFunction<WpHz, math::Domain<simsec, Hz>> *>(partitonPowerFunction)) {
                auto v11 = bf->getRLowerLower() == WpHz(0) ? -INFINITY : wpHz2dBmWpMHz(WpHz(bf->getRLowerLower()).get());
                auto v21 = bf->getRLowerUpper() == WpHz(0) ? -INFINITY : wpHz2dBmWpMHz(WpHz(bf->getRLowerUpper()).get());
                auto v12 = bf->getRUpperLower() == WpHz(0) ? -INFINITY : wpHz2dBmWpMHz(WpHz(bf->getRUpperLower()).get());
                auto v22 = bf->getRUpperUpper() == WpHz(0) ? -INFINITY : wpHz2dBmWpMHz(WpHz(bf->getRUpperUpper()).get());
                //TODO: interpolate the thresholds
                writeBilinearValue(x1, x2, y1.get(), y2.get(), v11, v21, v12, v22);
            }
            else
                throw cRuntimeError("TODO");
        });
        SpectrumFile.close();
    }


    if(LogFile.is_open())
    {
        LogFile.close();
    }
}

void LabscimRadioRecorder::receiveSignal(cComponent *source, simsignal_t signalID, long l, cObject *details)
{

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
    LogFile.flush();
}

void LabscimRadioRecorder::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    if(dynamic_cast<inet::physicallayer::Radio *>(source)==SpectrumRadioReceiver)
    {
        if(signalID == receptionStartedSignal)
        {
            const DimensionalReception* reception = dynamic_cast<const DimensionalReception *>(obj);
            if(reception!=nullptr)
            {
                const auto receptionPowerFunction = reception->getPower();
                spectrumPower.addElement(receptionPowerFunction);
            }
        }
        else if(signalID == transmissionEndedSignal)
        {
            const DimensionalTransmission* transmission = check_and_cast<const DimensionalTransmission *>(obj);
            if(transmission!=nullptr)
            {
                const auto transmissionPowerFunction = transmission->getPower();
                spectrumPower.addElement(transmissionPowerFunction);
            }
        }
    }
    else
    {

        if(signalID == transmissionEndedSignal)
        {
            const FlatTransmissionBase* transmission = check_and_cast<const FlatTransmissionBase *>(obj);
            if(transmission!=nullptr)
            {
                LogFile << signalID << " ," << simTime().dbl() <<",IRadio::transmissionEndedSignal, " << source->getFullPath().c_str();
                LogFile << "," << transmission->getStartTime() << "," << transmission->getEndTime() << "," << transmission->getCenterFrequency().get() << "," << transmission->getBandwidth().get() << "\n";
            }
        }
        else if(signalID == receptionStartedSignal)
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
        else if(signalID == receptionEndedSignal)
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
                        power = math::mW2dBmW(signalPowerInd->getPower().get()*1000);
                    }

                    if (packet->findTag<SnirInd>() != nullptr)
                    {
                        auto snir = packet->getTag<SnirInd>();
                        double snr = math::fraction2dB(snir->getAverageSnir());
                    }
                    LogFile << signalID << " ," << simTime().dbl() <<",packetSentToUpperSignal, " << source->getFullPath().c_str();
                    LogFile << "," << packet->getArrivalTime().dbl() << "," << frequency << "," << bandwidth << "," << power << "," << snr << "\n";
                }
            }
        }
    }
    LogFile.flush();
}

} // namespace physicallayer
} // namespace labscim

