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

Tetrahedron::Tetrahedron(Vertex p1, Vertex p2, Vertex p3, Vertex p4) :\
_w(p1), 
_x(p2), 
_y(p3), 
_z(p4){

	computeVolumeAndInertia();
}
Tetrahedron::~Tetrahedron(){

}

Vertex Tetrahedron::getX(){
	return _x;
}

Vertex Tetrahedron::getY(){
	return _y;
}
Vertex Tetrahedron::getZ(){
	return _z;
}

Vertex Tetrahedron::getw(){
	return _w;
}

Vertex Tetrahedron::getCentroid(){
	Vertex com;
	com.x = (_x.x + _y.x + _z.x + _w.x) / 4.0f;
	com.y = (_x.y + _y.y + _z.y + _w.y) / 4.0f;
	com.z = (_x.z + _y.z + _z.z + _w.z) / 4.0f;
	return com;
}

vector<double> Tetrahedron::getVolumeAndInertia(){
	return _volumeAndInertia;
}

void Tetrahedron::computeVolumeAndInertia(){
	double A = glm::distance2(_w, _x);
	double B = glm::distance2(_w, _y);
	double C = glm::distance2(_x, _y);
	double a = glm::distance2(_y, _z);
	double b = glm::distance2(_x, _z);
	double c = glm::distance2(_w, _z);
	double squaredVol = (4 * a * b * c) - (a*glm::pow((b + c - A), 2.0)) - (b*glm::pow((c + a - B), 2.0)) -
		(c*glm::pow((a + b - C), 2.0)) + ((a + b - C)*(a + c - B)*(b + c - A));

	double volume = glm::sqrt(squaredVol);// volume of tetrahedron
	_volumeAndInertia.push_back(volume);

	//centroid is used for calculating inertia tensor relative to center of mass.
	// translatw the tetrahedron to its center of mass using parallel axis theorem
	Vertex com = getCentroid();
	Vertex p0 = _w - com;
	Vertex p1 = _x - com;
	Vertex p2 = _y - com;
	Vertex p3 = _z - com;

	//Calculate inertia tensor based on Tonon's Formulae given in the paper mentioned below.
	//http://docsdrive.com/pdfs/sciencepublications/jmssp/2005/8-11.pdf
	//Explicit exact formulas for the 3-D tetrahedron inertia tensor in terms of its vertex coordinates - F.Tonon

	double inertia_a = (volume * 6.0 / 60.0) * (
		p0.y*p0.y + p0.y*p1.y + p0.y*p2.y + p0.y*p3.y + 
		p1.y*p1.y + p1.y*p2.y + p1.y*p3.y + 
		p2.y*p2.y + p2.y*p3.y +
		p3.y*p3.y + 
		p0.z*p0.z + p0.z*p1.z + p0.z*p2.z + p0.z*p3.z + 
		p1.z*p1.z + p1.z*p2.z + p1.z*p3.z + 
		p2.z*p2.z + p2.z*p3.z + 
		p3.z*p3.z);
	_volumeAndInertia.push_back(inertia_a);

	double inertia_b = (volume * 6.0 / 60.0) * (
		p0.x*p0.x + p0.x*p1.x + p0.x*p2.x + p0.x*p3.x + 
		p1.x*p1.x + p1.x*p2.x +	p1.x*p3.x + 
		p2.x*p2.x + p2.x*p3.x + 
		p3.x*p3.x + 
		p0.z*p0.z + p0.z*p1.z + p0.z*p2.z + p0.z*p3.z + 
		p1.z*p1.z + p1.z*p2.z +	p1.z*p3.z + 
		p2.z*p2.z + p2.z*p3.z + 
		p3.z*p3.z);
	_volumeAndInertia.push_back(inertia_b);

	double inertia_c = (volume * 6.0 / 60.0) * (
		p0.x*p0.x + p0.x*p1.x + p0.x*p2.x + p0.x*p3.x + 
		p1.x*p1.x + p1.x*p2.x +	p1.x*p3.x + 
		p2.x*p2.x + p2.x*p3.x + 
		p3.x*p3.x + 
		p0.y*p0.y + p0.y*p1.y + p0.y*p2.y + p0.y*p3.y + 
		p1.y*p1.y + p1.y*p2.y + p1.y*p3.y + 
		p2.y*p2.y + p2.y*p3.y + 
		p3.y*p3.y);
	_volumeAndInertia.push_back(inertia_c);

	double inertia_aa = (volume * 6.0 / 60.0) * (2.0 * (p0.y*p0.z + p1.y*p1.z + p2.y*p2.z + p3.y*p3.z) +
		p0.y*p1.z + p0.y*p2.z + p0.y*p3.z +
		p1.y*p0.z + p1.y*p2.z + p1.y*p3.z + 
		p2.y*p0.z + p2.y*p1.z + p2.y*p3.z + 
		p3.y*p0.z +	p3.y*p1.z + p3.y*p2.z);
	_volumeAndInertia.push_back(inertia_aa);

	double inertia_bb = (volume * 6.0 / 60.0) * (2.0 * (p0.x*p0.z + p1.x*p1.z + p2.x*p2.z + p3.x*p3.z) + 
		p0.x*p1.z + p0.x*p2.z + p0.x*p3.z +
		p1.x*p0.z + p1.x*p2.z + p1.x*p3.z + 
		p2.x*p0.z + p2.x*p1.z + p2.x*p3.z +
		p3.x*p0.z + p3.x*p1.z + p3.x*p2.z);
	_volumeAndInertia.push_back(inertia_bb);

	double inertia_cc = (volume * 6.0 / 60.0) * (2.0 * (p0.x*p0.y + p1.x*p1.y + p2.x*p2.y + p3.x*p3.y) + 
		p0.x*p1.y + p0.x*p2.y + p0.x*p3.y + 
		p1.x*p0.y + p1.x*p2.y + p1.x*p3.y + 
		p2.x*p0.y + p2.x*p1.y + p2.x*p3.y +
		p3.x*p0.y + p3.x*p1.y + p3.x*p2.y);
	_volumeAndInertia.push_back(inertia_cc);	
}

//class to compute volume, mass, center of mass, and inertia tensor of a mesh.
//origin is the default reference point for generating the tetrahedron from each triangle of the mesh. We can provide another reference
//point by passing it as 3rd parameter to the constructor

MassProperties::MassProperties(vector<Vertex> *vertices, Triangle *triangles, Vertex referencepoint = glm::vec3(0.0,0.0,0.0)):\
_vertices(vertices), 
_triangles(triangles), 
_referencePoint(referencepoint),
_trianglesCount(0),
_tetrahedraCount(0),
_verticesCount(0){

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
	delete _vertices;
	delete _triangles;
}

void MassProperties::generateTetrahedra(){
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

int MassProperties::getTetrahedraCount() const{
	return _tetrahedra.size();
}

vector<Tetrahedron> MassProperties::getTetrahedra() const{
	return _tetrahedra;
}
vector<double> MassProperties::getVolumeAndInertia(){
	vector<double> volumeAndInertia;
	return volumeAndInertia;
}