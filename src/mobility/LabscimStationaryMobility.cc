/*
 * Copyright (C) 2021 Guilherme Luiz Moritz <moritz@utfpr.edu.br>
 *
 * Based on:
 * Copyright (C) 2006 Isabel Dietrich <isabel.dietrich@informatik.uni-erlangen.de>
 * Copyright (C) 2013 OpenSim Ltd
 * Copyright (C) 2014 Florian Meier <florian.meier@koalo.de>
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

#include "LabscimStationaryMobility.h"
#include "inet/common/geometry/common/GeographicCoordinateSystem.h"

namespace labscim {

Define_Module(LabscimStationaryMobility);






void LabscimStationaryMobility::setInitialPosition()
{
    if(hasPar("InitByCartesianCoord"))
    {
        if (par("InitByCartesianCoord"))
        {
            updateDisplayStringFromMobilityState();
            //just ignore the coordinate system
            lastPosition.x = par("initialX");
            lastPosition.y = par("initialY");
            lastPosition.z = par("initialZ");
            EV_DEBUG << "position initialized from initialX/Y/Z parameters: " << lastPosition << endl;
            if (par("updateDisplayString"))
                updateDisplayStringFromMobilityState();
            recordScalar("x", lastPosition.x);
            recordScalar("y", lastPosition.y);
            recordScalar("z", lastPosition.z);
            return;
        }
    }
    StationaryMobility::setInitialPosition();
}



} // namespace labscim

