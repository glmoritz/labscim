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

//# This mobility model places the nodes randomly on space but guarantees that each node have at least numNeighbors Neighbors within maximumDistance meters
//# and no node is closer to any other in at least minimumDistance
simple LabscimMeshRandomMobility extends StationaryMobility
{
    parameters:        
        double minimumDistance @unit(m) = default(1m);
        double maximumDistance @unit(m) = default(500m);
        int numPoints = default(50);
        int numNeighbors = default(2);
        
        int context = default(0); //the mobility will create several meshes, based on context
        
        bool fixedNode = default(false); //this node is not placed randomly. The position must be set with initialX/initialY if InitByCartesianCoord = true or by Geographic Coord otherwise 
        bool InitByCartesianCoord = default(false); //this node is placed by cartesian coordinates
        
        //the parameters below are used only if fixedNode = true and ignored if InitByCartesianCoord = true
        //double initialLatitude @unit(deg) = default(nan deg);
        //double initialLongitude @unit(deg) = default(nan deg);
        //double initialAltitude @unit(m) = default(0m);
        
        //the parameters below are used only if InitByCartesianCoord = true and ignored otherwise
        //double initialX @unit(m) = default(0m);
        //double initialY @unit(m) = default(0m);
        //double initialZ @unit(m) = default(0m);
        
        //double initialHeading @unit(deg) = default(0deg);
        //double initialElevation @unit(deg) = default(0deg);
        //double initialBank @unit(deg) = default(0deg);
        //bool initFromDisplayString = default(true); // enables one time initialization from the subject module's display string
        //bool updateFromDisplayString = default(true); // enables continuous update from the subject module's display string for dragging and rotating it
        
        
        
        

        @class(labscim::LabscimMeshRandomMobility);
}

