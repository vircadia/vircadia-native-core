//
//  MassProperties.cpp
//  libraries/physics/src
//
//  Created by Virendra Singh 2015.02.28
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MassProperties.h"
using namespace massproperties;

Tetrahedron::Tetrahedron(const Vertex p1, const Vertex p2, const Vertex p3, const Vertex p4) :\
_w(p1), 
_x(p2), 
_y(p3), 
_z(p4){
    computeVolume();
    computeInertia();
}
Tetrahedron::~Tetrahedron(){

}

Vertex Tetrahedron::getX() const{
    return _x;
}

Vertex Tetrahedron::getY() const{
    return _y;
}
Vertex Tetrahedron::getZ() const{
    return _z;
}

Vertex Tetrahedron::getw() const{
    return _w;
}

Vertex Tetrahedron::getCentroid() const{
    Vertex com;
    com.x = (_x.x + _y.x + _z.x + _w.x) / 4.0f;
    com.y = (_x.y + _y.y + _z.y + _w.y) / 4.0f;
    com.z = (_x.z + _y.z + _z.z + _w.z) / 4.0f;
    return com;
}

vector<double> Tetrahedron::getVolumeAndInertia() const{
    return _volumeAndInertia;
}

void Tetrahedron::computeVolume(){
    glm::mat4 tet = { glm::vec4(_x.x, _y.x, _z.x, _w.x), glm::vec4(_x.y, _y.y, _z.y, _w.y), glm::vec4(_x.z, _y.z, _z.z, _w.z), 
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) };
    _volume = glm::determinant(tet) / 6.0f;
    _volumeAndInertia.push_back(_volume);
}

void Tetrahedron::computeInertia(){
   
    //centroid is used for calculating inertia tensor relative to center of mass.
    // translate the tetrahedron to its center of mass using P = P - centroid
    Vertex com = getCentroid();
    Vertex p0 = _w - com;
    Vertex p1 = _x - com;
    Vertex p2 = _y - com;
    Vertex p3 = _z - com;

    //Calculate inertia tensor based on Tonon's Formulae given in the paper mentioned below.
    //http://docsdrive.com/pdfs/sciencepublications/jmssp/2005/8-11.pdf
    //Explicit exact formulas for the 3-D tetrahedron inertia tensor in terms of its vertex coordinates - F.Tonon

    double inertia_a = (_volume * 6.0 / 60.0) * (
        p0.y*p0.y + p0.y*p1.y + p0.y*p2.y + p0.y*p3.y + 
        p1.y*p1.y + p1.y*p2.y + p1.y*p3.y + 
        p2.y*p2.y + p2.y*p3.y +
        p3.y*p3.y + 
        p0.z*p0.z + p0.z*p1.z + p0.z*p2.z + p0.z*p3.z + 
        p1.z*p1.z + p1.z*p2.z + p1.z*p3.z + 
        p2.z*p2.z + p2.z*p3.z + 
        p3.z*p3.z);
    _volumeAndInertia.push_back(inertia_a);

    double inertia_b = (_volume * 6.0 / 60.0) * (
        p0.x*p0.x + p0.x*p1.x + p0.x*p2.x + p0.x*p3.x + 
        p1.x*p1.x + p1.x*p2.x +	p1.x*p3.x + 
        p2.x*p2.x + p2.x*p3.x + 
        p3.x*p3.x + 
        p0.z*p0.z + p0.z*p1.z + p0.z*p2.z + p0.z*p3.z + 
        p1.z*p1.z + p1.z*p2.z +	p1.z*p3.z + 
        p2.z*p2.z + p2.z*p3.z + 
        p3.z*p3.z);
    _volumeAndInertia.push_back(inertia_b);

    double inertia_c = (_volume * 6.0 / 60.0) * (
        p0.x*p0.x + p0.x*p1.x + p0.x*p2.x + p0.x*p3.x + 
        p1.x*p1.x + p1.x*p2.x +	p1.x*p3.x + 
        p2.x*p2.x + p2.x*p3.x + 
        p3.x*p3.x + 
        p0.y*p0.y + p0.y*p1.y + p0.y*p2.y + p0.y*p3.y + 
        p1.y*p1.y + p1.y*p2.y + p1.y*p3.y + 
        p2.y*p2.y + p2.y*p3.y + 
        p3.y*p3.y);
    _volumeAndInertia.push_back(inertia_c);

    double inertia_aa = (_volume * 6.0 / 120.0) * (2.0 * (p0.y*p0.z + p1.y*p1.z + p2.y*p2.z + p3.y*p3.z) +
        p0.y*p1.z + p0.y*p2.z + p0.y*p3.z +
        p1.y*p0.z + p1.y*p2.z + p1.y*p3.z + 
        p2.y*p0.z + p2.y*p1.z + p2.y*p3.z + 
        p3.y*p0.z +	p3.y*p1.z + p3.y*p2.z);
    _volumeAndInertia.push_back(inertia_aa);

    double inertia_bb = (_volume * 6.0 / 120.0) * (2.0 * (p0.x*p0.z + p1.x*p1.z + p2.x*p2.z + p3.x*p3.z) + 
        p0.x*p1.z + p0.x*p2.z + p0.x*p3.z +
        p1.x*p0.z + p1.x*p2.z + p1.x*p3.z + 
        p2.x*p0.z + p2.x*p1.z + p2.x*p3.z +
        p3.x*p0.z + p3.x*p1.z + p3.x*p2.z);
    _volumeAndInertia.push_back(inertia_bb);

    double inertia_cc = (_volume * 6.0 / 120.0) * (2.0 * (p0.x*p0.y + p1.x*p1.y + p2.x*p2.y + p3.x*p3.y) + 
        p0.x*p1.y + p0.x*p2.y + p0.x*p3.y + 
        p1.x*p0.y + p1.x*p2.y + p1.x*p3.y + 
        p2.x*p0.y + p2.x*p1.y + p2.x*p3.y +
        p3.x*p0.y + p3.x*p1.y + p3.x*p2.y);
    _volumeAndInertia.push_back(inertia_cc);	
}

//class to compute volume, mass, center of mass, and inertia tensor of a mesh.
//origin is the default reference point for generating the tetrahedron from each triangle of the mesh. We can provide 
//another reference point by passing it as 3rd parameter to the constructor

MassProperties::MassProperties(vector<Vertex> *vertices, Triangle *triangles, Vertex referencepoint = glm::vec3(0.0,0.0,0.0)):\
_vertices(vertices), 
_triangles(triangles), 
_referencePoint(referencepoint),
_trianglesCount(0),
_tetrahedraCount(0),
_verticesCount(0),
_centerOfMass(glm::vec3(0.0, 0.0, 0.0)){

    if (_triangles){
        _trianglesCount = _triangles->size() / 3;
    }

    if (_vertices){
        _verticesCount = _vertices->size();
    }
    generateTetrahedra();
}

MassProperties::~MassProperties(){
    if (_vertices){
        _vertices->clear();
    }
    if (_triangles){
        _triangles->clear();
    }	
}

void MassProperties::generateTetrahedra() {
    for (int i = 0; i < _trianglesCount * 3; i += 3){
        Vertex p1 = _vertices->at(_triangles->at(i));
        Vertex p2 = _vertices->at(_triangles->at(i + 1));
        Vertex p3 = _vertices->at(_triangles->at(i + 2));
        Tetrahedron t(_referencePoint, p1, p2, p3);
        _tetrahedra.push_back(t);
    }
}

int MassProperties::getTriangleCount() const{
    return _trianglesCount;
}

int MassProperties::getVerticesCount() const{
    return _verticesCount;
}

Vertex MassProperties::getCenterOfMass() const{
    return _centerOfMass;
}

int MassProperties::getTetrahedraCount() const{
    return _tetrahedra.size();
}

vector<Tetrahedron> MassProperties::getTetrahedra() const{
    return _tetrahedra;
}

vector<double> MassProperties::getMassProperties(){
    vector<double> volumeAndInertia;
    double volume = 0.0;
    glm::vec3 centerOfMass;
    glm::mat3 globalInertia(0.0);
    glm::mat3 globalProductInertia(0.0);

    //Translate accumulated center of mass from each tetrahedron to mesh center of mass using parallel axis theorem
    for(Tetrahedron tet : _tetrahedra){
        vector<double> tetMassProperties = tet.getVolumeAndInertia();
        volume += tetMassProperties.at(0); //volume
        centerOfMass += tet.getCentroid() * (float)tetMassProperties.at(0);
    }

    if (volume != 0){
        _centerOfMass = (centerOfMass / (float)volume);
    }

    //Translate the moment of inertia from each tetrahedron to mesh center of mass using parallel axis theorem
    for(Tetrahedron tet : _tetrahedra){
        vector<double> tetMassProperties = tet.getVolumeAndInertia();
        glm::mat3 identity;
        glm::vec3 diff = _centerOfMass - tet.getCentroid();
        float diffDot = glm::dot(diff, diff);
        glm::mat3 outerDiff = glm::outerProduct(diff, diff);

        //3x3 of local inertia tensors of each tetrahedron. Inertia tensors are the diagonal elements
        glm::mat3 localMomentInertia = { Vertex(tetMassProperties.at(1), 0.0f, 0.0f), Vertex(0.0f, tetMassProperties.at(2), 0.0f), 
            Vertex(0.0f, 0.0f, tetMassProperties.at(3)) };
        glm::mat3 localProductInertia = { Vertex(tetMassProperties.at(4), 0.0f, 0.0f), Vertex(0.0f, tetMassProperties.at(5), 0.0f), 
            Vertex(0.0f, 0.0f, tetMassProperties.at(6)) };

        //Parallel axis theorem J = I * m[(R.R)*Identity - RxR] where x is outer cross product
        globalInertia += localMomentInertia + (float)tetMassProperties.at(0) * ((diffDot*identity) - outerDiff);
        globalProductInertia += localProductInertia + (float)tetMassProperties.at(0) * ((diffDot * identity) - outerDiff);
    }
    volumeAndInertia.push_back(volume);
    volumeAndInertia.push_back(globalInertia[0][0]);
    volumeAndInertia.push_back(globalInertia[1][1]);
    volumeAndInertia.push_back(globalInertia[2][2]);
    volumeAndInertia.push_back(globalProductInertia[0][0]);
    volumeAndInertia.push_back(globalProductInertia[1][1]);
    volumeAndInertia.push_back(globalProductInertia[2][2]);
    return volumeAndInertia;
}