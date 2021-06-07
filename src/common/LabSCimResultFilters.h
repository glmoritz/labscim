//
// Copyright (C) 2011 Zoltan Bojthe
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

#ifndef __LABSCIM_RESULTFILTERS_H
#define __LABSCIM_RESULTFILTERS_H

#include "inet/common/INETDefs.h"

using namespace inet;
using namespace inet::utils::filters;

namespace labscim {

namespace utils {

namespace filters {

/**
 * Filter that expects a Packet and outputs its MeanSNR in dB from snirIndication tag if exists .
 */
class INET_API MeanSnirFromSnirIndFilter : public cObjectResultFilter
{
  public:
    virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t, cObject *object, cObject *details) override;
};

} // namespace filters

} // namespace utils

} // namespace labscim

#endif // ifndef __LABSCIM_RESULTFILTERS_H

