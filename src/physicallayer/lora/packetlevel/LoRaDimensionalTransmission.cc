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

#include "inet/physicallayer/analogmodel/packetlevel/DimensionalTransmission.h"
#include "inet/common/Units.h"
#include "LoRaDimensionalTransmission.h"

using namespace inet;
using namespace inet::physicallayer;
using namespace inet::math;

namespace labscim {

namespace physicallayer {

LoRaDimensionalTransmission::LoRaDimensionalTransmission(const IRadio *transmitter, const Packet *packet, const simtime_t startTime, const simtime_t endTime, const simtime_t preambleDuration, const simtime_t headerDuration, const simtime_t dataDuration, const Coord startPosition, const Coord endPosition, const Quaternion startOrientation, const Quaternion endOrientation, const IModulation *modulation, b headerLength, b dataLength, Hz centerFrequency, Hz bandwidth, bps bitrate, const Ptr<const IFunction<WpHz, Domain<simsec, Hz>>>& power, int LoRaSF, int LoRaCR) :
    DimensionalTransmission(transmitter, packet, startTime, endTime, preambleDuration, headerDuration, dataDuration, startPosition, endPosition, startOrientation, endOrientation, modulation, headerLength, dataLength, centerFrequency, bandwidth, bitrate, power),
    LoRaSF(LoRaSF),
    LoRaCR(LoRaCR)
{
}

std::ostream& LoRaDimensionalTransmission::printToStream(std::ostream& stream, int level) const
{
    stream << "LoRaDimensionalTransmission";
    if (level <= PRINT_LEVEL_DEBUG)
        stream << ", LoRaSF = " << LoRaSF << ", LoRaCR = 4/" << LoRaCR+4 << ", ";
    return DimensionalTransmission::printToStream(stream, level);
}

} // namespace physicallayer

} // namespace labscim

