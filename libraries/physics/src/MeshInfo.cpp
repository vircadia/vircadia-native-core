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
#include <iostream>df
using namespace meshinfo;

//class to compute volume, mass, center of mass, and inertia tensor of a mesh.
//origin is the default reference point for generating the tetrahedron from each triangle of the mesh.

MeshInfo::MeshInfo(vector<Vertex> *vertices, vector<int> *triangles) :\
_vertices(vertices),
_triangles(triangles),
_centerOfMass(Vertex(0.0, 0.0, 0.0)){
}

MeshInfo::~MeshInfo(){

	_vertices = NULL;
	_triangles = NULL;

}

inline Vertex MeshInfo::getCentroid(const Vertex p1, const Vertex p2, const Vertex p3, const Vertex p4) const{
	Vertex com;
	com.x = (p1.x + p2.x + p3.x + p4.x) / 4.0f;
	com.y = (p2.y + p2.y + p3.y + p4.y) / 4.0f;
	com.z = (p2.z + p2.z + p3.z + p4.z) / 4.0f;
	return com;
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
	Vertex p0(0.0, 0.0, 0.0);
	float meshVolume = 0.0f;
	glm::mat3 globalMomentOfInertia(0.0);
	glm::mat3 globalProductOfInertia(0.0);

	//First we need need the center of mass of the mesh in order to translate the tetrahedron inertia to center of mass of the mesh.
	for (int i = 0; i < _triangles->size(); i += 3){
		Vertex p1 = _vertices->at(_triangles->at(i));
		Vertex p2 = _vertices->at(_triangles->at(i + 1));
		Vertex p3 = _vertices->at(_triangles->at(i + 2));
		float volume = getVolume(p1, p2, p3, p0);
		Vertex com = getCentroid(p0, p1, p2, p3);
		//Translate accumulated center of mass from each tetrahedron to mesh's center of mass using parallel axis theorem    
		meshVolume += volume;
		_centerOfMass += com * volume;
	}
	if (meshVolume == 0){
		return volumeAndInertia;
	}
	_centerOfMass = (_centerOfMass / (float)meshVolume);

	//Translate the moment of inertia from each tetrahedron to mesh's center of mass using parallel axis theorem
	for (int i = 0; i < _triangles->size(); i += 3){
		Vertex p1 = _vertices->at(_triangles->at(i));
		Vertex p2 = _vertices->at(_triangles->at(i + 1));
		Vertex p3 = _vertices->at(_triangles->at(i + 2));
		float volume = getVolume(p1, p2, p3, p0);
		Vertex com = getCentroid(p0, p1, p2, p3);
		glm::mat3 identity;
		Vertex diff = _centerOfMass - com;
		float diffDot = glm::dot(diff, diff);
		glm::mat3 outerDiff = glm::outerProduct(diff, diff);
		//centroid is used for calculating inertia tensor relative to center of mass.
		// translate the tetrahedron to its center of mass using P = P - centroid
		p0 = p0 - com;
		p1 = p1 - com;
		p2 = p2 - com;
		p3 = p3 - com;

		//Calculate inertia tensor based on Tonon's Formulae given in the paper mentioned below.
		//http://docsdrive.com/pdfs/sciencepublications/jmssp/2005/8-11.pdf
		//Explicit exact formulas for the 3-D tetrahedron inertia tensor in terms of its vertex coordinates - F.Tonon

		float inertia_a = (volume * 6.0 / 60.0) * (
			p0.y*p0.y + p0.y*p1.y + p0.y*p2.y + p0.y*p3.y +
			p1.y*p1.y + p1.y*p2.y + p1.y*p3.y +
			p2.y*p2.y + p2.y*p3.y +
			p3.y*p3.y +
			p0.z*p0.z + p0.z*p1.z + p0.z*p2.z + p0.z*p3.z +
			p1.z*p1.z + p1.z*p2.z + p1.z*p3.z +
			p2.z*p2.z + p2.z*p3.z +
			p3.z*p3.z);

		float inertia_b = (volume * 6.0 / 60.0) * (
			p0.x*p0.x + p0.x*p1.x + p0.x*p2.x + p0.x*p3.x +
			p1.x*p1.x + p1.x*p2.x + p1.x*p3.x +
			p2.x*p2.x + p2.x*p3.x +
			p3.x*p3.x +
			p0.z*p0.z + p0.z*p1.z + p0.z*p2.z + p0.z*p3.z +
			p1.z*p1.z + p1.z*p2.z + p1.z*p3.z +
			p2.z*p2.z + p2.z*p3.z +
			p3.z*p3.z);

		float inertia_c = (volume * 6.0 / 60.0) * (
			p0.x*p0.x + p0.x*p1.x + p0.x*p2.x + p0.x*p3.x +
			p1.x*p1.x + p1.x*p2.x + p1.x*p3.x +
			p2.x*p2.x + p2.x*p3.x +
			p3.x*p3.x +
			p0.y*p0.y + p0.y*p1.y + p0.y*p2.y + p0.y*p3.y +
			p1.y*p1.y + p1.y*p2.y + p1.y*p3.y +
			p2.y*p2.y + p2.y*p3.y +
			p3.y*p3.y);

		float inertia_aa = (volume * 6.0 / 120.0) * (2.0 * (p0.y*p0.z + p1.y*p1.z + p2.y*p2.z + p3.y*p3.z) +
			p0.y*p1.z + p0.y*p2.z + p0.y*p3.z +
			p1.y*p0.z + p1.y*p2.z + p1.y*p3.z +
			p2.y*p0.z + p2.y*p1.z + p2.y*p3.z +
			p3.y*p0.z + p3.y*p1.z + p3.y*p2.z);

		float inertia_bb = (volume * 6.0 / 120.0) * (2.0 * (p0.x*p0.z + p1.x*p1.z + p2.x*p2.z + p3.x*p3.z) +
			p0.x*p1.z + p0.x*p2.z + p0.x*p3.z +
			p1.x*p0.z + p1.x*p2.z + p1.x*p3.z +
			p2.x*p0.z + p2.x*p1.z + p2.x*p3.z +
			p3.x*p0.z + p3.x*p1.z + p3.x*p2.z);

		float inertia_cc = (volume * 6.0 / 120.0) * (2.0 * (p0.x*p0.y + p1.x*p1.y + p2.x*p2.y + p3.x*p3.y) +
			p0.x*p1.y + p0.x*p2.y + p0.x*p3.y +
			p1.x*p0.y + p1.x*p2.y + p1.x*p3.y +
			p2.x*p0.y + p2.x*p1.y + p2.x*p3.y +
			p3.x*p0.y + p3.x*p1.y + p3.x*p2.y);
		//3x3 of local inertia tensors of each tetrahedron. Inertia tensors are the diagonal elements
		glm::mat3 localMomentInertia = { Vertex(inertia_a, 0.0f, 0.0f), Vertex(0.0f, inertia_b, 0.0f),
			Vertex(0.0f, 0.0f, inertia_c) };
		glm::mat3 localProductInertia = { Vertex(inertia_aa, 0.0f, 0.0f), Vertex(0.0f, inertia_bb, 0.0f),
			Vertex(0.0f, 0.0f, inertia_cc) };

		//Parallel axis theorem J = I * m[(R.R)*Identity - RxR] where x is outer cross product
		globalMomentOfInertia += localMomentInertia + volume * ((diffDot*identity) - outerDiff);
		globalProductOfInertia += localProductInertia + volume * ((diffDot * identity) - outerDiff);
	}
	volumeAndInertia.push_back(meshVolume);
	volumeAndInertia.push_back(globalMomentOfInertia[0][0]);
	volumeAndInertia.push_back(globalMomentOfInertia[1][1]);
	volumeAndInertia.push_back(globalMomentOfInertia[2][2]);
	volumeAndInertia.push_back(globalProductOfInertia[0][0]);
	volumeAndInertia.push_back(globalProductOfInertia[1][1]);
	volumeAndInertia.push_back(globalProductOfInertia[2][2]);
	return volumeAndInertia;
}