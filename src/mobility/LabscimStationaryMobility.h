/*
 * Copyright (C) 2014 Florian Meier <florian.meier@koalo.de>
 *
 * Based on:
 * Copyright (C) 2006 Isabel Dietrich <isabel.dietrich@informatik.uni-erlangen.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */


#ifndef __INET_MESHRANDOMMOBILITY_H
#define __INET_MESHRANDOMMOBILITY_H

#include "inet/mobility/static/StationaryMobility.h"
#include "inet/common/geometry/common/Coord.h"

using namespace inet;

namespace labscim {

/**
 * @brief Mobility model which places all hosts randomly, but each node have at  least numNeighbors Neighbors within maximumDistance meters
 *
 * and no node is closer to any other in at least minimumDistance
 *
 * @ingroup mobility
 * @author Guilherme Luiz Moritz
 */
class INET_API LabscimStationaryMobility : public StationaryMobility
{
  protected:
    /** @brief Initializes the position according to the mobility model. */
    virtual void setInitialPosition() override;

  public:
    LabscimStationaryMobility() {};
    ~LabscimStationaryMobility() {};
};

}

#endif
