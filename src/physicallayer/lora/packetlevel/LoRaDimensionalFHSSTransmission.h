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

#ifndef __LABSCIM_LORADIMENSIONALFHSSTRANSMISSION_H
#define __LABSCIM_LORADIMENSIONALFHSSTRANSMISSION_H

#include "inet/common/math/Functions.h"
#include "inet/physicallayer/wireless/common/analogmodel/packetlevel/DimensionalTransmission.h"
#include "LoRaFHSSHopEntry.h"
#include "../../../common/lr_fhss_v1_base_types.h"

using namespace inet;
using namespace inet::physicallayer;
using namespace inet::math;

namespace labscim {

namespace physicallayer {



class INET_API LoRaDimensionalFHSSTransmission : public DimensionalTransmission
{
  protected:
    const W TransmissionPower;
    const std::vector<labscim::physicallayer::LoRaFHSSHopEntry> mHopTable;
    const lr_fhss_v1_bw_t FHSSBwIndex;
    const lr_fhss_v1_grid_t FHSSGrid;
    const lr_fhss_v1_cr_t FHSSCR;


  public:
    LoRaDimensionalFHSSTransmission(const IRadio *transmitter, const Packet *packet, const simtime_t startTime, const simtime_t endTime, const simtime_t preambleDuration, const simtime_t headerDuration, const simtime_t dataDuration, const Coord startPosition, const Coord endPosition, const Quaternion startOrientation, const Quaternion endOrientation, const IModulation *modulation, b headerLength, b dataLength, Hz centerFrequency, Hz bandwidth, bps bitrate, const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& power, const std::vector<labscim::physicallayer::LoRaFHSSHopEntry>& HopSequence, lr_fhss_v1_bw_t BWIndex, lr_fhss_v1_grid_t Grid, lr_fhss_v1_cr_t CR );

    virtual std::ostream& printToStream(std::ostream& stream, int level, int evFlags = 0) const override;

    virtual W getLoRaTransmissionPower() const { return TransmissionPower; }

    virtual const std::vector<labscim::physicallayer::LoRaFHSSHopEntry>& getHopTable() const {return mHopTable;};

    virtual const lr_fhss_v1_bw_t getFHSSBW() const { return FHSSBwIndex;};

    virtual const lr_fhss_v1_grid_t getFHSSGrid() const {return FHSSGrid;};

    virtual const lr_fhss_v1_cr_t getFHSSCR() const {return FHSSCR;};

};

} // namespace physicallayer

} // namespace labscim

#endif // ifndef __LABSCIM_LORADIMENSIONALTRANSMISSION_H

