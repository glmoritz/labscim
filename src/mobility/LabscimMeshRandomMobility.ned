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

import inet.mobility.base.MobilityBase;

//
// Places all hosts on concentric circles
//
//# This mobility model places the nodes randdomly on space but guarantees that each node have at least numNeighbors Neighbors within maximumDistance meters
//# and no node is closer to any other in at least minimumDistance
simple LabscimMeshRandomMobility extends MobilityBase
{
    parameters:        
        double minimumDistance @unit(m) = default(1m);
        double maximumDistance @unit(m) = default(500m);
        double initialX @unit(m) = default(0m);
        double initialY @unit(m) = default(0m);
        double initialZ @unit(m) = default(0m);
        
        int numNeighbors = default(2);
        bool fixedNode = default(false); //this node is not placed randomly. The position must be set with initialX/initialY parameters

        @class(labscim::LabscimMeshRandomMobility);
}

