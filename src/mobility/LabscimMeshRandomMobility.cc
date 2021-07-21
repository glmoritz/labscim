/*
 * Copyright (C) 2014 Florian Meier <florian.meier@koalo.de>
 *
 * Based on:
 * Copyright (C) 2006 Isabel Dietrich <isabel.dietrich@informatik.uni-erlangen.de>
 * Copyright (C) 2013 OpenSim Ltd
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

#include "LabscimMeshRandomMobility.h"

namespace labscim {

Define_Module(LabscimMeshRandomMobility);

std::list<const Coord*> LabscimMeshRandomMobility::mPoints;

void LabscimMeshRandomMobility::setInitialPosition()
{
    minimumDistance = par("minimumDistance");
    maximumDistance = par("maximumDistance");
    numNeighbors = par("numNeighbors");


    bool fixedNode =  par("fixedNode");
    unsigned int index = subjectModule->getIndex();
    Coord newcoord;

    lastPosition.x = par("initialX");
    lastPosition.y = par("initialY");
    lastPosition.z = par("initialZ");

    newcoord.x = par("initialX");
    newcoord.y = par("initialY");
    newcoord.z = par("initialZ");



    if(!fixedNode)
    {
        int32_t neighbors = std::min(static_cast<int32_t>(mPoints.size()),numNeighbors);
        uint32_t tries=0;
        bool too_near = false;
        bool relax = false;
        uint32_t num_neigh = 0;
        do
        {
            uint32_t item = intuniform(0,std::distance(mPoints.begin(), mPoints.end()) - 1);
            std::list<const Coord*>::iterator rnd_it = mPoints.begin();
            std::advance(rnd_it, item);

            double angle = uniform(0,2*M_PI);
            double module = uniform(minimumDistance,maximumDistance);
            newcoord.x = (*rnd_it)->x + module*cos(angle);
            newcoord.y = (*rnd_it)->y + module*sin(angle);

            too_near = false;
            num_neigh = 0;

            for(std::list<const Coord*>::iterator it = mPoints.begin(); it != mPoints.end(); it++)
            {
                if((*it)!=(const Coord*)&lastPosition)
                {
                    double distance = sqrt((newcoord.x - (*it)->x)*(newcoord.x - (*it)->x) + (newcoord.y - (*it)->y)*(newcoord.y - (*it)->y));
                    if(distance < minimumDistance)
                    {
                        too_near = true;
                        break;
                    }
                    if(distance < maximumDistance)
                    {
                        num_neigh++;
                        if(num_neigh>=neighbors)
                        {
                            break;
                        }
                    }
                }
            }
            tries++;
            if(tries>20)
            {
                relax = true;
            }
            else
            {
                relax = false;
            }
        }while( ( (too_near&&!relax) || ((num_neigh<neighbors)&&!relax) || (newcoord.x < constraintAreaMin.x) ||  (newcoord.y < constraintAreaMin.y) ||  (newcoord.x > constraintAreaMax.y) || (newcoord.y > constraintAreaMax.y) ) && (tries<50) );
    }

    lastPosition.x = newcoord.x;
    lastPosition.y = newcoord.y;
    lastPosition.z = newcoord.z;
    mPoints.push_back((const Coord*)&lastPosition);

    recordScalar("x", lastPosition.x);
    recordScalar("y", lastPosition.y);
    recordScalar("z", lastPosition.z);
}

} // namespace labscim

