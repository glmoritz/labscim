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

#ifndef __LABSCIM_LORARADIO_H
#define __LABSCIM_LORARADIO_H

#include "inet/physicallayer/wireless/common/base/packetlevel/ApskModulationBase.h"
#include "inet/physicallayer/wireless/common/base/packetlevel/FlatRadioBase.h"
#include "LoRaDimensionalReceiver.h"
#include "LoRaDimensionalTransmitter.h"
#include "../../../common/labscim_sx126x.h"
#include "../../../common/lr_fhss_v1_base_types.h"
#include "LoRaFHSSHopEntry.h"

using namespace inet;
using namespace inet::physicallayer;

namespace labscim {

namespace physicallayer {


class INET_API LoRaRadio : public FlatRadioBase
{
  protected:
    bool iAmGateway;
    int LoRaSF;
    int LoRaCR;

    LoRaDimensionalTransmitter* LoRaTransmitter;
    LoRaDimensionalReceiver* LoRaReceiver;

    std::list<cMessage *>concurrentReceptions;

  public:

    static simsignal_t loraradio_datarate_changed;

    LoRaRadio();

    void initialize(int stage) override;
    void handleUpperCommand(cMessage *message) override;

    virtual bps getPacketDataRate(const Packet *packet) const;
    virtual bps getPacketDataRate() const;

    virtual void startReception(cMessage *timer, IRadioSignal::SignalPart part) override;
    virtual void continueReception(cMessage *timer) override;
    virtual void endReception(cMessage *timer) override;

    virtual void abortReception(cMessage *timer) override;
    virtual void updateReceptionTimer();

    virtual simtime_t getPacketRadioTimeOnAir( const Packet *packet );

    virtual int getLoRaSF() const { return LoRaSF; }

    virtual int getLoRaCR() const { return LoRaCR; }

    virtual void setLoRaSF(int LoRaSF);

    virtual void setLoRaCR(int LoRaCR);

    virtual lr_fhss_v1_cr_t getFHSSCR() const;
    virtual void setFHSSCR(lr_fhss_v1_cr_t FHSSCR);

    virtual void setHoppingSequence(std::vector<LoRaFHSSHopEntry>& HopTable);

    virtual void setLoRaModulationMode(sx126x_pkt_types_e mode);
    virtual sx126x_pkt_types_e getLoRaModulationMode();

    static Hz FHSSBandwidthToHz(lr_fhss_v1_bw_t bw);

  private:
    static bool compareArrivals(cMessage* i1, cMessage* i2);
    bool mConfiguringRadio;

};

} // namespace physicallayer

} // namespace labscim

#endif // ifndef __LABSCIM_LORARADIO_H

