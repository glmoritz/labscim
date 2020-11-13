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

#include "inet/mobility/contract/IMobility.h"
#include "inet/physicallayer/analogmodel/packetlevel/DimensionalTransmission.h"
#include "inet/physicallayer/contract/packetlevel/RadioControlInfo_m.h"
#include "LoRaDimensionalTransmitter.h"
#include "LoRaDimensionalTransmission.h"
#include "LoRaTags_m.h"

using namespace inet;
using namespace inet::physicallayer;

namespace labscim {

namespace physicallayer {

Define_Module(LoRaDimensionalTransmitter);

LoRaDimensionalTransmitter::LoRaDimensionalTransmitter() :
    FlatTransmitterBase(),
    DimensionalTransmitterBase()
{
}

void LoRaDimensionalTransmitter::initialize(int stage)
{
    FlatTransmitterBase::initialize(stage);
    DimensionalTransmitterBase::initialize(stage);
}

std::ostream& LoRaDimensionalTransmitter::printToStream(std::ostream& stream, int level) const
{
    stream << "LoRaDimensionalTransmitter";
    DimensionalTransmitterBase::printToStream(stream, level);
    return DimensionalTransmitterBase::printToStream(stream, level);
}

const ITransmission *LoRaDimensionalTransmitter::createTransmission(const IRadio *transmitter, const Packet *packet, const simtime_t startTime) const
{

    //based on Semtech AN1200.13 - LoRa Modem Design Guide
    W transmissionPower = computeTransmissionPower(packet);
    Hz centerFrequency = computeCenterFrequency(packet);
    Hz bandWidth = computeBandwidth(packet);
    int RequestedLoraSF = computeLoRaSF(packet);
    int RequestedLoraCR = computeLoRaCR(packet);
    double LoraCRfraction = 4.0/(RequestedLoraCR+4.0);
    const bool CRC_enabled = true;
    const bool Header_enabled = true;
    const bool LowDataRate_optimization = false;

    bps transmissionBitrate = bps(RequestedLoraSF * bandWidth.get() * LoraCRfraction / (pow(2,RequestedLoraSF)) );

    int nPreamble = 8;
    simtime_t Tsym = pow(2, RequestedLoraSF) / bandWidth.get();
    simtime_t Tpreamble = (nPreamble + 4.25) * Tsym;

    int payloadBytes_PL = packet->getByteLength();

    int payloadSymbNb = 8 + std::max(ceil( (8*payloadBytes_PL - 4*RequestedLoraSF + 28 + CRC_enabled?16:0 - Header_enabled?0:20) / (4*(RequestedLoraSF-LowDataRate_optimization?2:0)) )*(RequestedLoraCR + 4), 0.0);

    simtime_t Tpayload = payloadSymbNb * Tsym;
    simtime_t Tpacket = Tpreamble+Tpayload;

    const simtime_t endTime = startTime + Tpacket;
    IMobility *mobility = transmitter->getAntenna()->getMobility();
    const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& powerFunction = createPowerFunction(startTime, endTime, centerFrequency, bandWidth, transmissionPower);
    const Coord startPosition = mobility->getCurrentPosition();
    const Coord endPosition = mobility->getCurrentPosition();
    const Quaternion startOrientation = mobility->getCurrentAngularPosition();
    const Quaternion endOrientation = mobility->getCurrentAngularPosition();
    return new LoRaDimensionalTransmission(transmitter, packet, startTime, endTime, Tpreamble, simtime_t::ZERO, Tpayload, startPosition, endPosition, startOrientation, endOrientation, modulation, b(0), packet->getTotalLength(), centerFrequency, bandWidth, transmissionBitrate, powerFunction,RequestedLoraSF,RequestedLoraCR);
}

int LoRaDimensionalTransmitter::computeLoRaSF(const Packet *packet) const
{
    auto LoRaParamsReq = const_cast<Packet *>(packet)->findTag<inet::LoRaParamsReq>();
    return LoRaParamsReq != nullptr ? LoRaParamsReq->getLoRaSF() : LoRaSF;
}

int LoRaDimensionalTransmitter::computeLoRaCR(const Packet *packet) const
{
    auto LoRaParamsReq = const_cast<Packet *>(packet)->findTag<inet::LoRaParamsReq>();
    return LoRaParamsReq != nullptr ? LoRaParamsReq->getLoRaCR() : LoRaCR;
}


} // namespace physicallayer

} // namespace labscim

