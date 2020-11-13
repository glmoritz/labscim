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
#include "LoRaRadio.h"
#include "LoRaDimensionalTransmitter.h"
#include "LoRaDimensionalReceiver.h"
#include "LoRaRadioControlInfo_m.h"

using namespace inet;
using namespace inet::physicallayer;

namespace labscim {
namespace physicallayer {

Define_Module(LoRaRadio);

LoRaRadio::LoRaRadio() :
    FlatRadioBase()
{
}

void LoRaRadio::initialize(int stage)
{
    FlatRadioBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL)
    {
        iAmGateway = par("iAmGateway").boolValue();
        LoRaSF = par("LoRaSF");
        LoRaCR = par("LoRaCR");

        LoRaTransmitter = check_and_cast<LoRaDimensionalTransmitter *>(getSubmodule("transmitter"));
        LoRaReceiver = check_and_cast<LoRaDimensionalReceiver *>(getSubmodule("receiver"));

    }
}

void LoRaRadio::handleUpperCommand(cMessage *message)
{
    if (message->getKind() == RADIO_C_CONFIGURE)
    {
        ConfigureLoRaRadioCommand* configureCommand =  check_and_cast<ConfigureLoRaRadioCommand *>(message->getControlInfo());
        if (configureCommand->getLoRaSF() != -1)
        {
            setLoRaSF(configureCommand->getLoRaSF());
        }
        if (configureCommand->getLoRaCR() != -1)
        {
            setLoRaCR(configureCommand->getLoRaCR());
        }
    }
    else
        throw cRuntimeError("Unsupported command");

    FlatRadioBase::handleUpperCommand(message);
}


void LoRaRadio::setLoRaSF(int LoRaSF)
{
    this->LoRaSF = LoRaSF;
    LoRaTransmitter->setLoRaSF(LoRaSF);
    LoRaReceiver->setLoRaSF(LoRaSF);
}

void LoRaRadio::setLoRaCR(int LoRaCR)
{
    this->LoRaCR = LoRaCR;
    LoRaTransmitter->setLoRaCR(LoRaCR);
    LoRaReceiver->setLoRaCR(LoRaCR);
}

} // namespace physicallayer
} // namespace inet

