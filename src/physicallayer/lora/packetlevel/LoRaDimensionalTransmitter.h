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

#include "inet/physicallayer/wireless/common/base/packetlevel/DimensionalTransmitterBase.h"
#include "inet/physicallayer/wireless/common/base/packetlevel/FlatTransmitterBase.h"

#include "../../../common/labscim_sx126x.h"
#include "../../../common/lr_fhss_v1_base_types.h"
#include "LoRaFHSSHopEntry.h"

using namespace inet;
using namespace inet::physicallayer;

namespace labscim {

namespace physicallayer {

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

    virtual std::ostream& printToStream(std::ostream& stream, int level, int evFlags = 0) const override;
    virtual const ITransmission *createTransmission(const IRadio *radio, const Packet *packet, const simtime_t startTime) const override;

    virtual bool setIamGateway() const { return iAmGateway; }
    virtual void setIamGateway(bool IamGateway) {this->iAmGateway = IamGateway;};

    virtual int getLoRaSF() const { return LoRaSF; }
    virtual void setLoRaSF(int LoRaSF) {this->LoRaSF = LoRaSF;};

    virtual int getLoRaCR() const { return LoRaCR; }
    virtual void setLoRaCR(int LoRaCR) {this->LoRaCR = LoRaCR;};

    virtual lr_fhss_v1_cr_t getFHSSCR() const { return FHSSCR; }
    virtual void setFHSSCR(lr_fhss_v1_cr_t FHSSCR) {this->FHSSCR = FHSSCR;};

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
    static uint32_t RadioGetLoRaBandwidthInHz( sx126x_lora_bw_e bw );

    virtual void setHoppingSequence(std::vector<LoRaFHSSHopEntry>& HopTable);

    virtual void setFHSSBW(lr_fhss_v1_bw_t BW) {this->BW = BW;};
    virtual lr_fhss_v1_bw_t getFHSSBW() {return BW;};

    virtual void setFHSSGrid(lr_fhss_v1_grid_t Grid) {this->FHSSGrid = Grid;};
    virtual lr_fhss_v1_grid_t getFHSSGrid() {return FHSSGrid;};

    virtual void setLoRaModulationMode(sx126x_pkt_types_e mode);
    virtual sx126x_pkt_types_e getLoRaModulationMode();

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
    lr_fhss_v1_cr_t FHSSCR;
    lr_fhss_v1_bw_t BW;
    lr_fhss_v1_grid_t FHSSGrid;
  private:
         bool iAmGateway;
         simsignal_t LoRaTransmissionCreated;
         std::vector<labscim::physicallayer::LoRaFHSSHopEntry> mHopTable;
         sx126x_pkt_types_e ModulationMode;
};

} // namespace physicallayer

} // namespace labscim

#endif // ifndef __LABSCIM_LORADIMENSIONALTRANSMITTER_H

