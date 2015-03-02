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
		vector<double> _volumeAndInertia;
	public:
		Tetrahedron(Vertex p1, Vertex p2, Vertex p3, Vertex p4);
		~Tetrahedron();
		Vertex getX();
		Vertex getY();
		Vertex getZ();
		Vertex getw();
		Vertex getCentroid();
		void computeVolumeAndInertia();
		vector<double> getVolumeAndInertia();
	};

	class MassProperties{
	private:
		int _trianglesCount;
		int _tetrahedraCount;
		int _verticesCount;
		vector<Vertex> *_vertices;
		Vertex _referencePoint;
		Triangle *_triangles;
		vector<Tetrahedron> _tetrahedra;
		void generateTetrahedra();
	public:
		MassProperties(vector<Vertex> *vertices, Triangle *triangles, Vertex refewrencepoint);
		~MassProperties();
		int getTriangleCount() const;
		int getVerticesCount() const;
		int getTetrahedraCount() const;
		vector<Tetrahedron> getTetrahedra() const;
		vector<double> getVolumeAndInertia();
	};
}