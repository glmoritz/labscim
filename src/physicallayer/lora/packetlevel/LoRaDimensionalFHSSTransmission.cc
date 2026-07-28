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

#include "inet/common/Units.h"
#include "LoRaDimensionalFHSSTransmission.h"
#include "../../../common/lr_fhss_v1_base_types.h"

using namespace inet;
using namespace inet::physicallayer;
using namespace inet::math;

namespace labscim {

namespace physicallayer {

LoRaDimensionalFHSSTransmission::LoRaDimensionalFHSSTransmission(const IRadio *transmitter, const Packet *packet, const simtime_t startTime, const simtime_t endTime, const simtime_t preambleDuration, const simtime_t headerDuration, const simtime_t dataDuration, const Coord startPosition, const Coord endPosition, const Quaternion startOrientation, const Quaternion endOrientation, const ITransmissionAnalogModel *analogModel, b headerLength, b dataLength, const IModulation *modulation, Hz bandwidth, bps bitrate, const std::vector<labscim::physicallayer::LoRaFHSSHopEntry>& HopSequence, lr_fhss_v1_bw_t BWIndex, lr_fhss_v1_grid_t Grid, lr_fhss_v1_cr_t CR ) :
    ApskTransmission(transmitter, packet, startTime, endTime, preambleDuration, headerDuration, dataDuration, startPosition, endPosition, startOrientation, endOrientation, nullptr, nullptr, nullptr, nullptr, analogModel, headerLength, dataLength, modulation, bandwidth, -1, bitrate, NAN),
    mHopTable(HopSequence),
    FHSSBwIndex(BWIndex),
    FHSSGrid(Grid),
    FHSSCR(CR)
{
}

std::ostream& LoRaDimensionalFHSSTransmission::printToStream(std::ostream& stream, int level, int evFlags) const
{
    stream << "LoRaDimensionalFHSSTransmission";
    //if (level <= PRINT_LEVEL_DEBUG)
    //    stream << ", LoRaSF = " << LoRaSF << ", LoRaCR = 4/" << LoRaCR+4 << ", ";
    return ApskTransmission::printToStream(stream, level);
}

} // namespace physicallayer

} // namespace labscim

