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
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/rtree.hpp>

// to store queries results
#include <vector>

// just for output
#include <iostream>
#include <boost/foreach.hpp>

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

using  point  = bg::model::point <double, 2, bg::cs::cartesian>;
using  pointI = std::pair<point, uint64_t>;
using  box = bg::model::box<point>;

namespace labscim {

Define_Module(LabscimMeshRandomMobility);


std::map<uint32_t, std::vector<Coord>> LabscimMeshRandomMobility::mPoints;
uint32_t LabscimMeshRandomMobility::maxContext=0;

void LabscimMeshRandomMobility::setInitialPosition()
{
    //this code is extremely inneficient (memorywise and computingwise)
    //TODO: there is an elegant way of doing that?

    minimumDistance = par("minimumDistance");
    maximumDistance = par("maximumDistance");
    numNeighbors = par("numNeighbors");
    numPoints = par("numPoints");

    bool fixedNode =  par("fixedNode");
    unsigned int index = subjectModule->getIndex();
    uint32_t context = par("context");
    double initialZ = par("initialZ");


    if(mPoints.find(context) == mPoints.end())
    {
        std::vector<Coord> l;
        l.reserve(numPoints);
        mPoints.insert(std::make_pair(context, l));
    }


    if(mPoints[context].empty())
    {
        std::vector<pointI> ok_points;
        uint64_t index = 0;
        uint64_t tries = 0;

        while(ok_points.size() < numPoints)
        {
            uint64_t points_to_add = numPoints - ok_points.size();

            //create rtree by packing (faster queries)
            for ( uint64_t i = 0 ; i < points_to_add ; ++i )
            {
                ok_points.push_back(pointI(point(uniform(constraintAreaMin.x,constraintAreaMax.x),uniform(constraintAreaMin.y,constraintAreaMax.y)), index++));
            }
            if(tries>40)
            {
                //giving up
                break;
            }
            bgi::rtree<pointI, bgi::quadratic<16> > rtree(ok_points);

            //now check if every point meets the mobility criteria
            ok_points.clear();
            for(auto const& c: rtree)
            {
                std::vector<pointI> result_n;
                rtree.query(bgi::nearest(std::get<0>(c), numNeighbors+1), std::back_inserter(result_n));
                uint64_t failed = false;

                for(auto neighbor=result_n.begin(); neighbor!=result_n.end(); ++neighbor)
                {
                    double clearance = bg::distance(std::get<0>(c), std::get<0>(*neighbor));
                    if( (clearance>0) && ( (clearance<minimumDistance) || (clearance>maximumDistance) ) )
                    {
                        failed = true;
                        break;
                    }
                }
                if(!failed)
                {
                    ok_points.push_back(c);
                }
            }
            tries++;
        }
        std::vector<Coord> coords;
        for(auto p=ok_points.begin(); p!=ok_points.end(); ++p)
        {
            point allocated = std::get<0>(*p);
            coords.push_back(Coord(bg::get<0>(allocated),bg::get<1>(allocated),initialZ));
        }
        mPoints[context] = coords;
    }

    if(!fixedNode)
    {
        if(index < mPoints[context].size())
        {
            lastPosition = mPoints[context][index];
        }
        else
        {
            lastPosition.x = uniform(constraintAreaMin.x,constraintAreaMax.x);
            lastPosition.y = uniform(constraintAreaMin.y,constraintAreaMax.y);
            lastPosition.z = par("initialZ");
        }
        EV_DEBUG << "position initialized randomly to form a mesh: index: (" << index << ") " << lastPosition << endl;
        if (par("updateDisplayString"))
            updateDisplayStringFromMobilityState();
    }
    else
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
        recordScalar("x", lastPosition.x);
        recordScalar("y", lastPosition.y);
        recordScalar("z", lastPosition.z);
        return;
    }
    recordScalar("x", lastPosition.x);
    recordScalar("y", lastPosition.y);
    recordScalar("z", lastPosition.z);
}

} // namespace labscim

