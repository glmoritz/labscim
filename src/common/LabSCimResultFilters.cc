//
// Copyright (C) 2011 OpenSim Ltd.
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
// @author Zoltan Bojthe
//


#include "inet/common/packet/Packet.h"
#include "inet/common/ResultFilters.h"
#include "inet/common/ResultRecorders.h"

#include "inet/physicallayer/base/packetlevel/FlatReceptionBase.h"
#include "inet/physicallayer/contract/packetlevel/SignalTag_m.h"
#include "LabSCimResultFilters.h"

namespace labscim {
namespace utils {
namespace filters {

using namespace inet;
using namespace inet::physicallayer;
using namespace inet::math;

Register_ResultFilter("meanSnirdB", labscim::utils::filters::MeanSnirFromSnirIndFilter);

void MeanSnirFromSnirIndFilter::receiveSignal(cResultFilter *prev, simtime_t_cref t, cObject *object, cObject *details)
{
#ifdef WITH_RADIO
    if (auto pk = dynamic_cast<Packet *>(object)) {
        auto tag = pk->findTag<SnirInd>();
        if (tag)
            fire(this, t, math::fraction2dB(tag->getAverageSnir()), details);
    }
#endif  // WITH_RADIO
}


} // namespace filters
} // namespace utils
} // namespace labscim

