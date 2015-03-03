//
//  MassProperties.h
//  libraries/physics/src
//
//  Created by Virendra Singh 2015.02.28
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_MassProperties_h
#define hifi_MassProperties_h

#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
using namespace std;
namespace massproperties{
    typedef glm::vec3 Vertex;
    typedef vector<int> Triangle;

    //Tetrahedron class containing the base triangle and the apex.
    class Tetrahedron{
    private:
        Vertex _w; //apex
        Vertex _x;
        Vertex _y;
        Vertex _z;
        double _volume;
        vector<double> _volumeAndInertia;
        void computeInertia();
        void computeVolume();
    public:
        Tetrahedron(const Vertex p1, const Vertex p2, const Vertex p3, const Vertex p4);
        ~Tetrahedron();
        Vertex getX() const;
        Vertex getY() const;
        Vertex getZ() const;
        Vertex getw() const;
        Vertex getCentroid() const;
        vector<double> getVolumeAndInertia() const;
    };

    class MassProperties{
    private:
        int _trianglesCount;
        int _tetrahedraCount;
        int _verticesCount;
        vector<Vertex> *_vertices;
        Vertex _referencePoint;
        Vertex _centerOfMass;
        Triangle *_triangles;
        vector<Tetrahedron> _tetrahedra;
        void generateTetrahedra();
    public:
        MassProperties(vector<Vertex> *vertices, Triangle *triangles, Vertex refewrencepoint);
        ~MassProperties();
        int getTriangleCount() const;
        int getVerticesCount() const;
        int getTetrahedraCount() const;
        Vertex getCenterOfMass() const;
        vector<Tetrahedron> getTetrahedra() const;
        vector<double> getMassProperties();		
    };
}
#endif // hifi_MassProperties_h