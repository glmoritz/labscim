//
// Copyright (C) 2014 Florian Meier <florian.meier@tu-harburg.de>
// 
// Based on:
// Copyright (C) 2006 Isabel Dietrich <isabel.dietrich@informatik.uni-erlangen.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//

package labscim.mobility;

import inet.mobility.static.StationaryMobility;

//
// Places all hosts on concentric circles
//
//# This mobility model is like stationary mobility, but can initialize nodes by x-y-z positions even if there is a coordinate system
simple LabscimStationaryMobility extends StationaryMobility
{
    parameters:
        bool InitByCartesianCoord = default(false); //this node is placed by cartesian coordinates
        
        
        @class(labscim::LabscimStationaryMobility);
}

