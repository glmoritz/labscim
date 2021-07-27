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

#ifndef __LABSCIM_LORADIMENSIONALTRANSMITTER_H
#define __LABSCIM_LORADIMENSIONALTRANSMITTER_H

#include "inet/physicallayer/base/packetlevel/DimensionalTransmitterBase.h"
#include "inet/physicallayer/base/packetlevel/FlatTransmitterBase.h"

#include "../../../common/sx126x_labscim.h"

using namespace inet;
using namespace inet::physicallayer;

namespace labscim {

namespace physicallayer {

const RadioLoRaBandwidths_t Bandwidths[] = {LORA_BW_125, LORA_BW_250, LORA_BW_500};

typedef enum
{
    MODEM_FSK = 0,
    MODEM_LORA,
}RadioModems_t;

class INET_API LoRaDimensionalTransmitter : public FlatTransmitterBase, public DimensionalTransmitterBase
{
  public:
    LoRaDimensionalTransmitter();

    virtual void initialize(int stage) override;

    virtual std::ostream& printToStream(std::ostream& stream, int level) const override;
    virtual const ITransmission *createTransmission(const IRadio *radio, const Packet *packet, const simtime_t startTime) const override;

    virtual bool setIamGateway() const { return iAmGateway; }
    virtual void setIamGateway(bool IamGateway) {this->iAmGateway = IamGateway;};

    virtual int getLoRaSF() const { return LoRaSF; }
    virtual void setLoRaSF(int LoRaSF) {this->LoRaSF = LoRaSF;};

    virtual int getLoRaCR() const { return LoRaCR; }
    virtual void setLoRaCR(int LoRaCR) {this->LoRaCR = LoRaCR;};

    virtual bool getLoRaCRC_enabled() const { return CRC_enabled; }
    virtual void setLoRaCRC_enabled(bool CRC_enabled) {this->CRC_enabled = CRC_enabled;};

    virtual bool getLoRaHeader_enabled() const { return Header_enabled; }
    virtual void setLoRaHeader_enabled(bool Header_enabled) {this->Header_enabled = Header_enabled;};

    virtual bool getLoRaLowDataRate_optimization() const { return LowDataRate_optimization; }
    virtual void setLoRaLowDataRate_optimization(bool LowDataRate_optimization) {this->LowDataRate_optimization = LowDataRate_optimization;};

    virtual int getPreamble_length() const { return Preamble_length; }
    virtual void setPreamble_length(int Preamble_length) {this->Preamble_length = Preamble_length;};

    virtual int getPayload_length() const { return Payload_length; }
    virtual void setPayload_length(int Payload_length) {this->Payload_length = Payload_length;};

    virtual bps getPacketDataRate(const Packet *packet) const;
    virtual bps getPacketDataRate() const;

    simtime_t getPacketRadioTimeOnAir( const Packet *packet );
    static uint32_t RadioGetLoRaBandwidthInHz( RadioLoRaBandwidths_t bw );

  protected:

    virtual int computeLoRaSF(const Packet *packet) const;
    virtual int computeLoRaCR(const Packet *packet) const;
    int LoRaSF;
    int LoRaCR;
    bool CRC_enabled;
    bool Header_enabled;
    bool LowDataRate_optimization;
    int Preamble_length;
    int Payload_length;
  private:
         bool iAmGateway;
         simsignal_t LoRaTransmissionCreated;
};

} // namespace physicallayer

} // namespace labscim

#endif // ifndef __LABSCIM_LORADIMENSIONALTRANSMITTER_H

