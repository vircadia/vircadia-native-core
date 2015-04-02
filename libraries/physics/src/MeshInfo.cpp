//
//  MeshInfo.cpp
//  libraries/physics/src
//
//  Created by Virendra Singh 2015.02.28
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MeshInfo.h"
#include <iostream>
using namespace meshinfo;

//class to compute volume, mass, center of mass, and inertia tensor of a mesh.
//origin is the default reference point for generating the tetrahedron from each triangle of the mesh.

MeshInfo::MeshInfo(vector<Vertex> *vertices, vector<int> *triangles) :\
    _vertices(vertices),
    _centerOfMass(Vertex(0.0, 0.0, 0.0)),
    _triangles(triangles)
{
    
}

MeshInfo::~MeshInfo(){

    _vertices = NULL;
    _triangles = NULL;

}

inline float MeshInfo::getVolume(const Vertex p1, const Vertex p2, const Vertex p3, const Vertex p4) const{
    glm::mat4 tet = { glm::vec4(p1.x, p2.x, p3.x, p4.x), glm::vec4(p1.y, p2.y, p3.y, p4.y), glm::vec4(p1.z, p2.z, p3.z, p4.z),
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) };
    return glm::determinant(tet) / 6.0f;
}

Vertex MeshInfo::getMeshCentroid() const{
    return _centerOfMass;
}

vector<float> MeshInfo::computeMassProperties(){
    vector<float> volumeAndInertia = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    Vertex origin(0.0, 0.0, 0.0);
    glm::mat3 identity;
    float meshVolume = 0.0f;
    glm::mat3 globalInertiaTensors(0.0);

    for (unsigned int i = 0; i < _triangles->size(); i += 3){
        Vertex p1 = _vertices->at(_triangles->at(i));
        Vertex p2 = _vertices->at(_triangles->at(i + 1));
        Vertex p3 = _vertices->at(_triangles->at(i + 2));
		float volume = getVolume(p1, p2, p3, origin);
        Vertex com = 0.25f * (p1 + p2 + p3);   
        meshVolume += volume;
        _centerOfMass += com * volume;

        //centroid is used for calculating inertia tensor relative to center of mass.
        // translate the tetrahedron to its center of mass using P = P - centroid
        Vertex p0 = origin - com;
        p1 = p1 - com;
        p2 = p2 - com;
        p3 = p3 - com;

        //Calculate inertia tensor based on Tonon's Formulae given in the paper mentioned below.
        //http://docsdrive.com/pdfs/sciencepublications/jmssp/2005/8-11.pdf
        //Explicit exact formulas for the 3-D tetrahedron inertia tensor in terms of its vertex coordinates - F.Tonon

        float i11 = (volume * 0.1f) * (
            p0.y*p0.y + p0.y*p1.y + p0.y*p2.y + p0.y*p3.y +
            p1.y*p1.y + p1.y*p2.y + p1.y*p3.y +
            p2.y*p2.y + p2.y*p3.y +
            p3.y*p3.y +
            p0.z*p0.z + p0.z*p1.z + p0.z*p2.z + p0.z*p3.z +
            p1.z*p1.z + p1.z*p2.z + p1.z*p3.z +
            p2.z*p2.z + p2.z*p3.z +
            p3.z*p3.z);

		float i22 = (volume * 0.1f) * (
            p0.x*p0.x + p0.x*p1.x + p0.x*p2.x + p0.x*p3.x +
            p1.x*p1.x + p1.x*p2.x + p1.x*p3.x +
            p2.x*p2.x + p2.x*p3.x +
            p3.x*p3.x +
            p0.z*p0.z + p0.z*p1.z + p0.z*p2.z + p0.z*p3.z +
            p1.z*p1.z + p1.z*p2.z + p1.z*p3.z +
            p2.z*p2.z + p2.z*p3.z +
            p3.z*p3.z);

		float i33 = (volume * 0.1f) * (
            p0.x*p0.x + p0.x*p1.x + p0.x*p2.x + p0.x*p3.x +
            p1.x*p1.x + p1.x*p2.x + p1.x*p3.x +
            p2.x*p2.x + p2.x*p3.x +
            p3.x*p3.x +
            p0.y*p0.y + p0.y*p1.y + p0.y*p2.y + p0.y*p3.y +
            p1.y*p1.y + p1.y*p2.y + p1.y*p3.y +
            p2.y*p2.y + p2.y*p3.y +
            p3.y*p3.y);

        float i23 = -(volume * 0.05f) * (2.0f * (p0.y*p0.z + p1.y*p1.z + p2.y*p2.z + p3.y*p3.z) +
            p0.y*p1.z + p0.y*p2.z + p0.y*p3.z +
            p1.y*p0.z + p1.y*p2.z + p1.y*p3.z +
            p2.y*p0.z + p2.y*p1.z + p2.y*p3.z +
            p3.y*p0.z + p3.y*p1.z + p3.y*p2.z);

		float i21 = -(volume * 0.05f) * (2.0f * (p0.x*p0.z + p1.x*p1.z + p2.x*p2.z + p3.x*p3.z) +
            p0.x*p1.z + p0.x*p2.z + p0.x*p3.z +
            p1.x*p0.z + p1.x*p2.z + p1.x*p3.z +
            p2.x*p0.z + p2.x*p1.z + p2.x*p3.z +
            p3.x*p0.z + p3.x*p1.z + p3.x*p2.z);

		float i31 = -(volume * 0.05f) * (2.0f * (p0.x*p0.y + p1.x*p1.y + p2.x*p2.y + p3.x*p3.y) +
            p0.x*p1.y + p0.x*p2.y + p0.x*p3.y +
            p1.x*p0.y + p1.x*p2.y + p1.x*p3.y +
            p2.x*p0.y + p2.x*p1.y + p2.x*p3.y +
            p3.x*p0.y + p3.x*p1.y + p3.x*p2.y);

        //3x3 of local inertia tensors of each tetrahedron. Inertia tensors are the diagonal elements
        //         |  I11 -I12 -I13 |     
        //     I = | -I21  I22 -I23 |
        //         | -I31 -I32  I33 |
        glm::mat3 localInertiaTensors = { Vertex(i11, i21, i31), Vertex(i21, i22, i23),
            Vertex(i31, i23, i33) };

        //Translate the inertia tensors from center of mass to origin
        //Parallel axis theorem J = I * m[(R.R)*Identity - RxR] where x is outer cross product
        globalInertiaTensors += localInertiaTensors + volume * ((glm::dot(com, com) * identity) -
        glm::outerProduct(com, com));
    }

    //Translate accumulated center of mass from each tetrahedron to mesh's center of mass using parallel axis theorem 
    if (meshVolume == 0){
        return volumeAndInertia;
    }
    _centerOfMass = (_centerOfMass / meshVolume);
    
    //Translate the inertia tensors from origin to mesh's center of mass.
    globalInertiaTensors = globalInertiaTensors - meshVolume * ((glm::dot(_centerOfMass, _centerOfMass) *
        identity) - glm::outerProduct(_centerOfMass, _centerOfMass));

    volumeAndInertia[0] = meshVolume;
    volumeAndInertia[1] = globalInertiaTensors[0][0]; //i11
    volumeAndInertia[2] = globalInertiaTensors[1][1]; //i22
    volumeAndInertia[3] = globalInertiaTensors[2][2]; //i33
    volumeAndInertia[4] = -globalInertiaTensors[2][1]; //i23 or i32
    volumeAndInertia[5] = -globalInertiaTensors[1][0]; //i21 or i12
    volumeAndInertia[6] = -globalInertiaTensors[2][0]; //i13 or i31
    return volumeAndInertia;
}