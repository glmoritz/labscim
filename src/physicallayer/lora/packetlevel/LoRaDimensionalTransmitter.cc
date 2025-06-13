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
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/DimensionalTransmission.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/RadioControlInfo_m.h"
#include "LoRaDimensionalTransmitter.h"
#include "LoRaDimensionalTransmission.h"
#include "LoRaDimensionalFHSSTransmission.h"
#include "LoRaTags_m.h"
#include "../../../common/labscim_sx126x.h"
#include "LoRaFHSSHopEntry.h"

using namespace inet;
using namespace inet::physicallayer;

namespace labscim {

namespace physicallayer {

Define_Module(LoRaDimensionalTransmitter);

LoRaDimensionalTransmitter::LoRaDimensionalTransmitter() :
            FlatTransmitterBase(),
            DimensionalTransmitterBase()
{
    CRC_enabled = true;
    Header_enabled = true;
    LowDataRate_optimization = false;
    Preamble_length = 8;
    ModulationMode = SX126X_PKT_TYPE_LORA;
    Payload_length = 222; //https://lora-developers.semtech.com/library/tech-papers-and-guides/the-book/packet-size-considerations/
}

void LoRaDimensionalTransmitter::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        LoRaSF = par("LoRaSF");
        LoRaCR = par("LoRaCR");
    }
    FlatTransmitterBase::initialize(stage);
    DimensionalTransmitterBase::initialize(stage);
}

std::ostream& LoRaDimensionalTransmitter::printToStream(std::ostream& stream, int level, int evFlags) const
{
    stream << "LoRaDimensionalTransmitter";
    DimensionalTransmitterBase::printToStream(stream, level);
    return DimensionalTransmitterBase::printToStream(stream, level);
}

const ITransmission *LoRaDimensionalTransmitter::createTransmission(const IRadio *transmitter, const Packet *packet, const simtime_t startTime) const
{
    W transmissionPower = computeTransmissionPower(packet);
    Hz centerFrequency = computeCenterFrequency(packet);
    Hz bandWidth = computeBandwidth(packet);
    IMobility *mobility = transmitter->getAntenna()->getMobility();
    const Coord startPosition = mobility->getCurrentPosition();
    const Coord endPosition = mobility->getCurrentPosition();
    const Quaternion startOrientation = mobility->getCurrentAngularPosition();
    const Quaternion endOrientation = mobility->getCurrentAngularPosition();

    switch(ModulationMode)
    {
    case SX126X_PKT_TYPE_LORA:
    {
        //based on Semtech AN1200.13 - LoRa Modem Design Guide
        int RequestedLoraSF = computeLoRaSF(packet);
        int RequestedLoraCR = computeLoRaCR(packet);
        double LoraCRfraction = 4.0/(RequestedLoraCR+4.0);

        bps transmissionBitrate = bps(RequestedLoraSF * bandWidth.get() * LoraCRfraction / (pow(2,RequestedLoraSF)) );
        simtime_t Tsym = pow(2, RequestedLoraSF) / bandWidth.get();
        simtime_t Tpreamble = (Preamble_length + 4.25) * Tsym;
        int payloadBytes_PL = packet->getByteLength();
        int payloadSymbNb = 8 + std::max( std::ceil( (8.0* payloadBytes_PL  - 4*RequestedLoraSF + 28 + (CRC_enabled?16:0) - (Header_enabled?0:20))  /  (4*(RequestedLoraSF-(LowDataRate_optimization?2:0))) )*(RequestedLoraCR + 4), 0.0);
        simtime_t Tpayload = payloadSymbNb * Tsym;
        simtime_t Tpacket = Tpayload + Tpreamble;
        const simtime_t endTime = startTime + Tpacket;
        const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& powerFunction = createPowerFunction(startTime, endTime, centerFrequency, bandWidth, transmissionPower);

        EV_DEBUG << "Start LoRa TX " << centerFrequency << ", BW" << bandWidth << ", SF " << RequestedLoraSF << ", CR 4/" << (RequestedLoraCR+4) << ", Power: " << math::mW2dBmW(transmissionPower.get()*1000) << " dBm" << endl;
        return new LoRaDimensionalTransmission(transmitter, packet, startTime, endTime, Tpreamble, simtime_t::ZERO, Tpayload, startPosition, endPosition, startOrientation, endOrientation, modulation, b(0), packet->getTotalLength(), centerFrequency, bandWidth, transmissionBitrate, powerFunction,RequestedLoraSF,RequestedLoraCR, transmissionPower,!iAmGateway);
    }
    case SX126X_PKT_TYPE_LR_FHSS:
    {
        bps transmissionBitrate = bps(488.28125);
        Hz BeamBandwidth = Hz(488.28125);
        std::vector<Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>> FHSSBands;

        simtime_t hopstart = startTime;
        simtime_t hopend;


        //create LR-FHSS spectrum
        for(const auto& hop : mHopTable)
        {
            hopend =  hopstart + hop.getDuration();
            const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& powerFunction = createPowerFunction(hopstart, hopend, hop.getCenterFrequency(), hop.getBandwidth(), transmissionPower);
            FHSSBands.push_back(powerFunction);
            hopstart = hopend;
        }
        const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& FHSSSpectrum = makeShared<SummedFunction<WpHz, Domain<simsec, Hz>>>(FHSSBands);
        return new LoRaDimensionalFHSSTransmission(transmitter, packet, startTime, hopend, simtime_t::ZERO, simtime_t::ZERO, hopend-startTime, startPosition, endPosition, startOrientation, endOrientation, modulation, b(0), packet->getTotalLength(), centerFrequency, bandWidth, transmissionBitrate, FHSSSpectrum, mHopTable, BW, FHSSGrid, FHSSCR );
    }
    case SX126X_PKT_TYPE_GFSK:
    default:
    {
        return nullptr;
    }
    }
}

bps LoRaDimensionalTransmitter::getPacketDataRate(const Packet *packet) const
{
    Hz bandWidth = computeBandwidth(packet);
    int RequestedLoraSF = computeLoRaSF(packet);
    int RequestedLoraCR = computeLoRaCR(packet);
    double LoraCRfraction = 4.0/(RequestedLoraCR+4.0);
    bps transmissionBitrate = bps(RequestedLoraSF * bandWidth.get() * LoraCRfraction / (pow(2,RequestedLoraSF)) );
    return bps(transmissionBitrate);
}

bps LoRaDimensionalTransmitter::getPacketDataRate() const
{
    double LoraCRfraction = 4.0/(LoRaCR+4.0);
    bps transmissionBitrate = bps(LoRaSF * bandwidth.get() * LoraCRfraction / (pow(2,LoRaSF)) );
    return bps(transmissionBitrate);
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

void LoRaDimensionalTransmitter::setHoppingSequence(std::vector<LoRaFHSSHopEntry>& HopTable)
{
    mHopTable.clear();
    mHopTable = HopTable;
}

void LoRaDimensionalTransmitter::setLoRaModulationMode(sx126x_pkt_types_e mode)
{
    if((mode!=SX126X_PKT_TYPE_LORA)&&(mode!=SX126X_PKT_TYPE_LR_FHSS))
    {
        throw cRuntimeError("Invalid Modulation Type (Unfortunately GFSK modulation is not implemented yet)");
    }
    ModulationMode = mode;
}

sx126x_pkt_types_e LoRaDimensionalTransmitter::getLoRaModulationMode()
{
    return ModulationMode;
}


simtime_t LoRaDimensionalTransmitter::getPacketRadioTimeOnAir( const Packet *packet )
{
    //based on Semtech AN1200.13 - LoRa Modem Design Guide
    Hz bandWidth = computeBandwidth(packet);
    int RequestedLoraSF = computeLoRaSF(packet);
    int RequestedLoraCR = computeLoRaCR(packet);
    double LoraCRfraction = 4.0/(RequestedLoraCR+4.0);
    bps transmissionBitrate = bps(RequestedLoraSF * bandWidth.get() * LoraCRfraction / (pow(2,RequestedLoraSF)) );
    simtime_t Tsym = pow(2, RequestedLoraSF) / bandWidth.get();
    simtime_t Tpreamble = (Preamble_length + 4.25) * Tsym;
    int payloadBytes_PL = packet->getByteLength();
    int payloadSymbNb = 8 + std::max( std::ceil( (8.0* payloadBytes_PL  - 4*RequestedLoraSF + 28 + (CRC_enabled?16:0) - (Header_enabled?0:20))  /  (4*(RequestedLoraSF-(LowDataRate_optimization?2:0))) )*(RequestedLoraCR + 4), 0.0);
    simtime_t Tpayload = payloadSymbNb * Tsym;
    return Tpayload + Tpreamble;
}

uint32_t LoRaDimensionalTransmitter::RadioGetLoRaBandwidthInHz( sx126x_lora_bw_e bw )
{
    uint32_t bandwidthInHz = 0;
    switch( bw )
    {
    case SX126X_LORA_BW_007:
        bandwidthInHz = 7812UL;
        break;
    case SX126X_LORA_BW_010:
        bandwidthInHz = 10417UL;
        break;
    case SX126X_LORA_BW_015:
        bandwidthInHz = 15625UL;
        break;
    case SX126X_LORA_BW_020:
        bandwidthInHz = 20833UL;
        break;
    case SX126X_LORA_BW_031:
        bandwidthInHz = 31250UL;
        break;
    case SX126X_LORA_BW_041:
        bandwidthInHz = 41667UL;
        break;
    case SX126X_LORA_BW_062:
        bandwidthInHz = 62500UL;
        break;
    case SX126X_LORA_BW_125:
        bandwidthInHz = 125000UL;
        break;
    case SX126X_LORA_BW_250:
        bandwidthInHz = 250000UL;
        break;
    case SX126X_LORA_BW_500:
        bandwidthInHz = 500000UL;
        break;
    }
    return bandwidthInHz;
}


} // namespace physicallayer

} // namespace labscim

