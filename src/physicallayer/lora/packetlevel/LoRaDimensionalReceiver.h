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

#ifndef __LABSCIM_LORADIMENSIONALRECEIVER_H
#define __LABSCIM_LORADIMENSIONALRECEIVER_H

#include "inet/physicallayer/base/packetlevel/FlatReceiverBase.h"
#include "LoRaDimensionalReception.h"
#include "inet/common/Units.h"


using namespace inet;
using namespace inet::physicallayer;

namespace labscim {

namespace physicallayer {

class INET_API LoRaDimensionalReceiver : public FlatReceiverBase
{
   private:
    const int nonOrthDelta[6][6] = {
           {1, -8, -9, -9, -9, -9},
           {-11, 1, -11, -12, -13, -13},
           {-15, -13, 1, -13, -14, -15},
           {-19, -18, -17, 1, -17, -18},
           {-22, -22, -21, -20, 1, -20},
           {-25, -25, -25, -24, -23, 1}
        };


  protected:
    W minInterferencePower;
    W snirNonLoRaThreshold;
    bool iAmGateway;

    int LoRaSF;
    int LoRaCR;

    W getSensitivity(const LoRaDimensionalReception *reception) const;

  public:
    LoRaDimensionalReceiver();

    void initialize(int stage) override;

    virtual std::ostream& printToStream(std::ostream& stream, int level) const override;

    virtual W getMinInterferencePower() const override { return minInterferencePower; }

    virtual bool computeIsReceptionPossible(const IListening *listening, const ITransmission *transmission) const override;
    virtual bool computeIsReceptionPossible(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part) const override;
    virtual bool computeIsReceptionAttempted(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference) const override;
    virtual bool computeIsReceptionSuccessful(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference, const ISnir *snir) const override;
    virtual const IReceptionDecision* computeReceptionDecision(const IListening *listening, const IReception *reception, IRadioSignal::SignalPart part, const IInterference *interference, const ISnir *snir) const override;

    virtual const IListening* createListening(const IRadio *radio, const simtime_t startTime, const simtime_t endTime, const Coord startPosition, const Coord endPosition) const override;
    virtual const IReceptionResult *computeReceptionResult(const IListening *listening, const IReception *reception, const IInterference *interference, const ISnir *snir, const std::vector<const IReceptionDecision *> *decisions) const override;


    virtual int getLoRaSF() const { return LoRaSF; }

    virtual int getLoRaCR() const { return LoRaCR; }

    virtual void setLoRaSF(int LoRaSF) {this->LoRaSF = LoRaSF;}; //TODO: set failure to the receiving packet?

    virtual void setLoRaCR(int LoRaCR) {this->LoRaCR = LoRaCR;}; //TODO: set failure to the receiving packet?
};

} // namespace physicallayer

} // namespace labscim

#endif // ifndef __LABSCIM_LORADIMENSIONALRECEIVER_H

