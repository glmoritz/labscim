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
#include <boost/geometry/algorithms/within.hpp>


#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/strong_components.hpp>

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
using  polygon = bg::model::polygon<point>;
using  multipoint = bg::model::multi_point<point>;

using namespace boost;

namespace labscim {




Define_Module(LabscimMeshRandomMobility);


std::map<uint32_t, std::vector<Coord>> LabscimMeshRandomMobility::mPoints;
std::map<uint32_t, uint32_t> LabscimMeshRandomMobility::mGoodPoints;
std::map<uint32_t, uint64_t> LabscimMeshRandomMobility::mUsedPoints;

uint32_t LabscimMeshRandomMobility::maxContext=0;


void LabscimMeshRandomMobility::setInitialPosition()
{
    //this code is extremely inneficient (memorywise and computingwise)
    //TODO: there is an elegant way of doing that?

    // create a typedef for the Graph type
    typedef adjacency_list<vecS, vecS, directedS> Graph;
    polygon boundary_poly;

    bool fixedNode =  par("fixedNode");
    uint32_t context = par("context");
    double initialZ = par("initialZ");
    bool boundary_set = false;

    minimumDistance = par("minimumDistance");
    maximumDistance = par("maximumDistance");
    numNeighbors = par("numNeighbors");
    numPoints = par("numPoints");

    auto boundaryPolygonX = check_and_cast<cValueArray *>(par("boundaryPolygonX").objectValue());
    auto boundaryPolygonY = check_and_cast<cValueArray *>(par("boundaryPolygonY").objectValue());

    if(boundaryPolygonX->size()==boundaryPolygonY->size()&&boundaryPolygonX->size()>2)
    {
        for (int i = 0; i < boundaryPolygonX->size(); i++) {
            float x = (boundaryPolygonX->get(i)).doubleValue();
            float y = (boundaryPolygonY->get(i)).doubleValue();
            point p(x,y);
            bg::append(boundary_poly.outer(), p);
        }
        boundary_set = true;
    }

    if(mPoints.find(context) == mPoints.end())
    {
        std::vector<Coord> l;
        l.reserve(numPoints);
        mPoints.insert(std::make_pair(context, l));
    }

    if(mPoints[context].empty())
    {
        uint32_t loop_tries = 0;
        bool points_ok = false;
        mUsedPoints[context] = 0;

        while((!points_ok)&&(loop_tries<5))
        {

            std::vector<pointI> ok_points;
            uint64_t index = 0;
            uint64_t tries = 0;

            while(ok_points.size() < numPoints)
            {
                uint64_t points_to_add = numPoints - ok_points.size();

                if(tries>40)
                {
                    //giving up
                    mGoodPoints[context] =ok_points.size();
                    break;
                }

                //create rtree by packing (faster queries)
                for ( uint64_t i = 0 ; i < points_to_add ; ++i )
                {
                    ok_points.push_back(pointI(point(uniform(constraintAreaMin.x,constraintAreaMax.x),uniform(constraintAreaMin.y,constraintAreaMax.y)), index++));
                }

                bgi::rtree<pointI, bgi::quadratic<16> > rtree(ok_points);

                //now check if every point meets the mobility criteria
                ok_points.clear();
                for(auto const& c: rtree)
                {
                    bool failed = false;
                    point p = std::get<0>(c);

                    //first check if the point is within region boundary
                    if(boundary_set)
                    {
                        if(!boost::geometry::within(p, boundary_poly))
                        {
                            failed = true;
                        }
                    }

                    if(!failed)
                    {
                        std::vector<pointI> result_n;
                        rtree.query(bgi::nearest(p, numNeighbors+1), std::back_inserter(result_n));
                        for(auto neighbor=result_n.begin(); neighbor!=result_n.end(); ++neighbor)
                        {
                            double clearance = bg::distance(std::get<0>(c), std::get<0>(*neighbor));
                            if( (clearance>0) && ( (clearance<minimumDistance) || (clearance>maximumDistance) ) )
                            {
                                failed = true;
                                break;
                            }
                        }
                    }

                    if(!failed)
                    {
                        ok_points.push_back(c);
                    }
                }
                tries++;
            }

            bgi::rtree<pointI, bgi::quadratic<16> > rtree(ok_points);

            if(ok_points.size() < numPoints)
            {
                //try to complete ok points by individually adding points to rtree
                for(uint32_t k=0;k<numPoints*4;k++)
                {
                    pointI p_try = pointI(point(uniform(constraintAreaMin.x,constraintAreaMax.x),uniform(constraintAreaMin.y,constraintAreaMax.y)), index++);

                    std::vector<pointI> result_n;
                    rtree.query(bgi::nearest(std::get<0>(p_try), numNeighbors), std::back_inserter(result_n));
                    bool failed = false;
                    for(auto neighbor=result_n.begin(); neighbor!=result_n.end(); ++neighbor)
                    {
                        double clearance = bg::distance(std::get<0>(p_try), std::get<0>(*neighbor));
                        if( (clearance<minimumDistance) || (clearance>maximumDistance)  )
                        {
                            failed = true;
                            break;
                        }
                    }
                    if(!failed)
                    {
                        //we found a good point
                        ok_points.push_back(p_try);
                        rtree.insert(p_try);
                    }
                    if(ok_points.size() == numPoints)
                    {
                        break;
                    }
                }
            }
            mGoodPoints[context] = ok_points.size();

            //now create a coordinate list and the connectivity graph
            Graph connectivity(numPoints);
            std::vector<Coord> coords;

            std::map<uint32_t, uint32_t> edge_map;
            std::map<uint32_t, pointI> edge_points;

            uint32_t current_index = 0;
            for(auto p=ok_points.begin(); p!=ok_points.end(); ++p)
            {
                point allocated = std::get<0>(*p);
                std::vector<pointI> result_n;

                point box_upper_left = point(bg::get<0>(allocated)-maximumDistance,bg::get<1>(allocated)-maximumDistance);
                point box_lower_right = point(bg::get<0>(allocated)+maximumDistance,bg::get<1>(allocated)+maximumDistance);
                box query_box(box_upper_left,box_lower_right);
                rtree.query(bgi::within(query_box), std::back_inserter(result_n));
                for(auto neighbor=result_n.begin(); neighbor!=result_n.end(); ++neighbor)
                {
                    double clearance = bg::distance(std::get<0>(*p), std::get<0>(*neighbor));
                    if( (clearance>minimumDistance) && (clearance<maximumDistance)  )
                    {
                        uint32_t index1 = std::get<1>(*p);
                        uint32_t index2 = std::get<1>(*neighbor);

                        if(edge_map.find(index1) == edge_map.end())
                        {
                            edge_map.insert(std::make_pair(index1, current_index++));
                            edge_points.insert(std::make_pair(edge_map[index1], *p));
                        }

                        if(edge_map.find(index2) == edge_map.end())
                        {
                            edge_map.insert(std::make_pair(index2, current_index++));
                        }

                        uint32_t mapped_index1 = edge_map[index1];
                        uint32_t mapped_index2 = edge_map[index2];

                        add_edge(mapped_index1, mapped_index2, connectivity);
                    }
                }
                coords.push_back(Coord(bg::get<0>(allocated),bg::get<1>(allocated),initialZ));
            }

            //check if the created graph is strongly connected
            //check: https://www.boost.org/doc/libs/1_65_0/libs/graph/example/strong_components.cpp

            typedef graph_traits<adjacency_list<vecS, vecS, directedS> >::vertex_descriptor Vertex;

            std::vector<int32_t> component(num_vertices(connectivity)), discover_time(num_vertices(connectivity));
            std::vector<default_color_type> color(num_vertices(connectivity));
            std::vector<Vertex> root(num_vertices(connectivity));

            int32_t num = strong_components(connectivity, make_iterator_property_map(component.begin(), get(vertex_index, connectivity)),
                    root_map(make_iterator_property_map(root.begin(), get(vertex_index, connectivity))).
                    color_map(make_iterator_property_map(color.begin(), get(vertex_index, connectivity))).
                    discover_time_map(make_iterator_property_map(discover_time.begin(), get(vertex_index, connectivity))));

            if(num==1) //only one connected subgraph was formed
            {
                points_ok = true;
            }
            else
            {
                loop_tries++;
                if(loop_tries == 5)
                {
                    //we couldn't find a good topology - what should I do?
//                    std::vector<multipoint> point_geo_vector;
//                    std::vector<polygon> geo_hulls;
//                    std::vector<int32_t>::size_type i;
//
//                    for(uint32_t j=0;j<num;j++)
//                    {
//                        multipoint mp;
//                        polygon gh;
//                        point_geo_vector.push_back(mp);
//                        point_geo_vector.append(gh);
//                    }
//
//                    for (i = 0; i != component.size(); ++i)
//                    {
//                        bg::append(point_geo_vector[component[i]], bg::get<0>(edge_points[i]));
//                    }
//                    for(uint32_t j=0;j<num;j++)
//                    {
//
//                        bg::convex_hull(point_geo_vector[j], geo_hulls[j]);
//                    }

                }
            }
            //randomize coords
            int32_t size = coords.size();
            for (int32_t i = 0; i < size - 1; i++) {
                uint32_t j = i + intuniformexcl(0, size-i);
                std::swap(coords[i],coords[j]);
            }
            mPoints[context] = coords;
        }
        if(loop_tries == 5)
        {
            mGoodPoints[context] = 0;
        }
    }

    uint64_t index = mUsedPoints[context]++;

    if(!fixedNode)
    {
        if(index < mPoints[context].size())
        {
            lastPosition = mPoints[context][index];
            recordScalar("PositionViolation", index<mGoodPoints[context]?0:1);
        }
        else
        {
            lastPosition.x = uniform(constraintAreaMin.x,constraintAreaMax.x);
            lastPosition.y = uniform(constraintAreaMin.y,constraintAreaMax.y);
            recordScalar("PositionViolation", 1);
        }
        lastPosition.z = initialZ;
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
                recordScalar("PositionViolation", 2);
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
        recordScalar("PositionViolation", 3);
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

