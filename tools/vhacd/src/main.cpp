//
//  main.cpp
//  tools/vhacd/src
//
//  Created by Virendra Singh on 2/20/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <VHACD.h>
#include <string>
#include <vector>
#include "VHACDUtil.h"

using namespace std;
using namespace VHACD;

int main(int argc, char * argv[])
{
	vector<int> triangles; // array of indexes
	vector<float> points;  // array of coordinates 
	vhacd::VHACDUtil vUtil;
	vhacd::LoadFBXResults fbx; //mesh data from loaded fbx file
	vhacd::ComputeResults results; // results after computing vhacd
	VHACD::IVHACD::Parameters params;
	vhacd::ProgressCallback pCallBack;
	

	QString fname = "F:/models/ship/Sample_Ship.fbx";

	//set parameters for V-HACD
	params.m_callback = &pCallBack; //progress callback
	params.m_resolution = 50000;
	params.m_depth = 10;
	params.m_concavity = 0.003;
	params.m_alpha = 0.05; // controls the bias toward clipping along symmetry planes
	params.m_pca = 1; // enable/disable normalizing the mesh before applying the convex decomposition
	params.m_mode = 1; // 0: voxel - based approximate convex decomposition, 1 : tetrahedron - based approximate convex decomposition
	params.m_maxNumVerticesPerCH = 128;
	params.m_minVolumePerCH = 0.0001; // controls the adaptive sampling of the generated convex - hulls

	// load the mesh 
	if (!vUtil.loadFBX(fname, &fbx))
	{
		cout << "Error in opening FBX file....";
		return 1;
	}

	if (!vUtil.computeVHACD(&fbx, params, &results))
	{
		cout << "Compute Failed...";
		return 1;
	}

	int totalVertices = 0;
	for (int i = 0; i < fbx.meshCount; i++)
	{
		totalVertices += fbx.perMeshVertices.at(i).count();
	}

	int totalTriangles = 0;
	for (int i = 0; i < fbx.meshCount; i++)
	{
		totalTriangles += fbx.perMeshTriangleIndices.at(i).count();
	}

	int totalHulls = 0;
	QVector<int> hullCounts = results.convexHullsCountList;
	for (int i = 0; i < results.meshCount; i++)
	{
		totalHulls += hullCounts.at(i);
	}
	cout << endl << "Summary of V-HACD Computation..................." << endl;
	cout << "File Path          : " << fname.toStdString() << endl;
	cout << "Number Of Meshes   : " << fbx.meshCount << endl;
	cout << "Processed Meshes   : " << results.meshCount << endl;
	cout << "Total vertices     : " << totalVertices << endl;
	cout << "Total Triangles    : " << totalTriangles << endl;
	cout << "Total Convex Hulls : " << totalHulls << endl;
	cout << endl << "Summary per convex hull ........................" << endl <<endl;
	for (int i = 0; i < results.meshCount; i++)
	{
		cout << "Mesh : " << i + 1 << endl;
		QVector<VHACD::IVHACD::ConvexHull> chList = results.convexHullList.at(i);
		cout << "\t" << "Number Of Hulls : " << chList.count() << endl;

		for (int j = 0; j < results.convexHullList.at(i).count(); j++)
		{

			cout << "\tHUll : " << j + 1 << endl;
			cout << "\t\tNumber Of Points    : " << chList.at(j).m_nPoints << endl;
			cout << "\t\tNumber Of Triangles : " << chList.at(j).m_nTriangles << endl;
		}
	}

	getchar();
	return 0;
}