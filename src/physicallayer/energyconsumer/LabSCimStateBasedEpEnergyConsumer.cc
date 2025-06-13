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

#include "inet/common/ModuleAccess.h"
#include "inet/physicallayer/wireless/common/energyconsumer/StateBasedEpEnergyConsumer.h"
#include "LabSCimStateBasedEpEnergyConsumer.h"
#include "inet/physicallayer/wireless/common/base/packetlevel/FlatTransmitterBase.h"

namespace labscim {

namespace physicallayer {

using namespace inet;
using namespace inet::power;
using namespace omnetpp;

Define_Module(LabSCimStateBasedEpEnergyConsumer);

void LabSCimStateBasedEpEnergyConsumer::initialize(int stage)
{
    StateBasedEpEnergyConsumer::initialize(stage);
    if (stage == INITSTAGE_LOCAL)
    {
         transmitterTransmittingPowerConsumptionIncreasePerWatt = par("transmitterTransmittingPowerConsumptionIncreasePerWatt").doubleValue();

         transmitterTransmittingPreamblePowerConsumptionIncreasePerWatt = par("transmitterTransmittingPreamblePowerConsumptionIncreasePerWatt").doubleValue();

         transmitterTransmittingHeaderPowerConsumptionIncreasePerWatt = par("transmitterTransmittingHeaderPowerConsumptionIncreasePerWatt").doubleValue();

         transmitterTransmittingDataPowerConsumptionIncreasePerWatt = par("transmitterTransmittingDataPowerConsumptionIncreasePerWatt").doubleValue();
    }
}

W LabSCimStateBasedEpEnergyConsumer::InterpolatePower(W PowerAt0, W Power, double IncreasePerWatt) const
{
    return Power*IncreasePerWatt+PowerAt0;
}


W LabSCimStateBasedEpEnergyConsumer::computePowerConsumption() const
{
    IRadio::RadioMode radioMode = radio->getRadioMode();
    if (radioMode == IRadio::RADIO_MODE_OFF)
        return offPowerConsumption;
    else if (radioMode == IRadio::RADIO_MODE_SLEEP)
        return sleepPowerConsumption;
    else if (radioMode == IRadio::RADIO_MODE_SWITCHING)
        return switchingPowerConsumption;
    W powerConsumption = W(0);
    IRadio::ReceptionState receptionState = radio->getReceptionState();
    IRadio::TransmissionState transmissionState = radio->getTransmissionState();
    if (radioMode == IRadio::RADIO_MODE_RECEIVER || radioMode == IRadio::RADIO_MODE_TRANSCEIVER) {
        switch (receptionState) {
            case IRadio::RECEPTION_STATE_IDLE:
                powerConsumption += receiverIdlePowerConsumption;
                break;
            case IRadio::RECEPTION_STATE_BUSY:
                powerConsumption += receiverBusyPowerConsumption;
                break;
            case IRadio::RECEPTION_STATE_RECEIVING: {
                auto part = radio->getReceivedSignalPart();
                switch (part) {
                    case IRadioSignal::SIGNAL_PART_NONE:
                        break;
                    case IRadioSignal::SIGNAL_PART_WHOLE:
                        powerConsumption += receiverReceivingPowerConsumption;
                        break;
                    case IRadioSignal::SIGNAL_PART_PREAMBLE:
                        powerConsumption += receiverReceivingPreamblePowerConsumption;
                        break;
                    case IRadioSignal::SIGNAL_PART_HEADER:
                        powerConsumption += receiverReceivingHeaderPowerConsumption;
                        break;
                    case IRadioSignal::SIGNAL_PART_DATA:
                        powerConsumption += receiverReceivingDataPowerConsumption;
                        break;
                    default:
                        throw cRuntimeError("Unknown received signal part");
                }
                break;
            }
            case IRadio::RECEPTION_STATE_UNDEFINED:
                break;
            default:
                throw cRuntimeError("Unknown radio reception state");
        }
    }
    if (radioMode == IRadio::RADIO_MODE_TRANSMITTER || radioMode == IRadio::RADIO_MODE_TRANSCEIVER) {
        switch (transmissionState) {
            case IRadio::TRANSMISSION_STATE_IDLE:
                powerConsumption += transmitterIdlePowerConsumption;
                break;
            case IRadio::TRANSMISSION_STATE_TRANSMITTING:
            {
                const FlatTransmitterBase* FlatTransmitter = check_and_cast<const FlatTransmitterBase*>(radio->getTransmitter());

                if(FlatTransmitter)
                {
                    auto part = radio->getTransmittedSignalPart();
                    W txpower = FlatTransmitter->getPower();
                    switch (part) {
                    case IRadioSignal::SIGNAL_PART_NONE:
                        break;
                    case IRadioSignal::SIGNAL_PART_WHOLE:
                        powerConsumption += InterpolatePower(transmitterTransmittingPowerConsumption, txpower, transmitterTransmittingPowerConsumptionIncreasePerWatt);
                        break;
                    case IRadioSignal::SIGNAL_PART_PREAMBLE:
                        powerConsumption += InterpolatePower(transmitterTransmittingPreamblePowerConsumption, txpower, transmitterTransmittingPreamblePowerConsumptionIncreasePerWatt);
                        break;
                    case IRadioSignal::SIGNAL_PART_HEADER:
                        powerConsumption += InterpolatePower(transmitterTransmittingHeaderPowerConsumption, txpower, transmitterTransmittingHeaderPowerConsumptionIncreasePerWatt);
                        break;
                    case IRadioSignal::SIGNAL_PART_DATA:
                        powerConsumption += InterpolatePower(transmitterTransmittingDataPowerConsumption, txpower, transmitterTransmittingDataPowerConsumptionIncreasePerWatt);
                        break;
                    default:
                        throw cRuntimeError("Unknown transmitted signal part");
                    }
                }
                else
                {
                    throw cRuntimeError("LabSCim Energy Consumer Expects a FlatTransmitter");
                }
                break;
            }
            case IRadio::TRANSMISSION_STATE_UNDEFINED:
                break;
            default:
                throw cRuntimeError("Unknown radio transmission state");
        }
    }
    return powerConsumption;
}

} // namespace physicallayer

} // namespace labscim

