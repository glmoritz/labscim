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

#include "inet/physicallayer/base/packetlevel/ApskModulationBase.h"
#include "inet/physicallayer/base/packetlevel/FlatRadioBase.h"
#include "LoRaDimensionalReceiver.h"
#include "LoRaDimensionalTransmitter.h"

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

  public:


    LoRaRadio();

    void initialize(int stage) override;
    void handleUpperCommand(cMessage *message) override;


    virtual int getLoRaSF() const { return LoRaSF; }

    virtual int getLoRaCR() const { return LoRaCR; }

    virtual void setLoRaSF(int LoRaSF);

    virtual void setLoRaCR(int LoRaCR);
};

} // namespace physicallayer

} // namespace labscim

#endif // ifndef __LABSCIM_LORARADIO_H

