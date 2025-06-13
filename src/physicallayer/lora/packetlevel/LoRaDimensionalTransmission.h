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

#ifndef __LABSCIM_LORADIMENSIONALTRANSMISSION_H
#define __LABSCIM_LORADIMENSIONALTRANSMISSION_H

#include "inet/common/math/Functions.h"
#include "inet/physicallayer/wireless/ieee80211/packetlevel/Ieee80211DimensionalTransmission.h"

using namespace inet;
using namespace inet::physicallayer;
using namespace inet::math;

namespace labscim {

namespace physicallayer {



class INET_API LoRaDimensionalTransmission : public DimensionalTransmission
{
  protected:
    const int LoRaSF;
    const int LoRaCR;
    const W TransmissionPower;
    const bool Uplink;


  public:
    LoRaDimensionalTransmission(const IRadio *transmitter, const Packet *packet, const simtime_t startTime, const simtime_t endTime, const simtime_t preambleDuration, const simtime_t headerDuration, const simtime_t dataDuration, const Coord startPosition, const Coord endPosition, const Quaternion startOrientation, const Quaternion endOrientation, const IModulation *modulation, b headerLength, b dataLength, Hz centerFrequency, Hz bandwidth, bps bitrate, const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& power, int LoRaSF, int LoRaCR, W LoRaTransmissionPower, bool IsUplink);

    virtual std::ostream& printToStream(std::ostream& stream, int level, int evFlags = 0) const override;

    virtual int getLoRaSF() const { return LoRaSF; }
    virtual int getLoRaCR() const { return LoRaCR; }
    virtual W getLoRaTransmissionPower() const { return TransmissionPower; }
    virtual bool getLoRaIamUplink() const { return Uplink; }
};

} // namespace physicallayer

} // namespace labscim

#endif // ifndef __LABSCIM_LORADIMENSIONALTRANSMISSION_H

