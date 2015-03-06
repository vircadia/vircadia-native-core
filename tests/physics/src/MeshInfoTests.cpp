//
//  MeshInfoTests.cpp
//  tests/physics/src
//
//  Created by Virendra Singh on 2015.03.02
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>
#include <iomanip>
#include <MeshInfo.h>

#include "MeshInfoTests.h"
const double epsilon = 0.02;
void MeshInfoTests::testWithTetrahedron(){
    glm::vec3 p0(8.33220, -11.86875, 0.93355);
    glm::vec3 p1(0.75523, 5.00000, 16.37072);
    glm::vec3 p2(52.61236, 5.00000, -5.38580);
    glm::vec3 p3(2.00000, 5.00000, 3.00000);
    glm::vec3 centroid(15.92492, 0.782813, 3.72962);

	//translate the tetrahedron so that its apex is on origin
	glm::vec3 p11 = p1 - p0;
	glm::vec3 p22 = p2 - p0;
	glm::vec3 p33 = p3 - p0;
	vector<glm::vec3> vertices = { p11, p22, p33 };
	vector<int> triangles = { 0, 1, 2 };

    float volume = 1873.233236;
	float inertia_a = 43520.33257;
	float inertia_b = 194711.28938;
	float inertia_c = 191168.76173;
	float inertia_aa = 4417.66150;
	float inertia_bb = -46343.16662;
	float inertia_cc = 11996.20119;

	meshinfo::MeshInfo meshinfo(&vertices,&triangles);
	glm::vec3 tetCenterOfMass = meshinfo.getCentroid(p0, p1, p2, p3);
	glm::vec3 diff = centroid - tetCenterOfMass;
	vector<float> voumeAndInertia = meshinfo.computeMassProperties();
    std::cout << std::setprecision(12);
    //test if centroid is correct
	if (diff.x > epsilon || diff.y > epsilon || diff.z > epsilon){
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR : Centroid is incorrect : Expected = " << centroid.x << " " <<
			centroid.y << " " << centroid.z << ", actual = " << tetCenterOfMass.x << " " << tetCenterOfMass.y <<
			" " << tetCenterOfMass.z << std::endl;
    }

    //test if volume is correct
	if (abs(volume - voumeAndInertia.at(0)) > epsilon){
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR : Volume is incorrect : Expected = " << volume << " " <<
            ", actual = " << voumeAndInertia.at(0) << std::endl;
    }

    //test if moment of inertia with respect to x axis is correct
	if (abs(inertia_a - (voumeAndInertia.at(1))) > epsilon){
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR : Moment of inertia with respect to x axis is incorrect : Expected = " <<
            inertia_a << " " << ", actual = " << (voumeAndInertia.at(1)) << std::endl;
    }

    //test if moment of inertia with respect to y axis is correct
	if (abs(inertia_b - (voumeAndInertia.at(2))) > epsilon){
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR : Moment of inertia with respect to y axis is incorrect : Expected = " <<
            inertia_b << " " << ", actual = " << (voumeAndInertia.at(2)) << std::endl;
    }

    //test if moment of inertia with respect to z axis is correct
	if (abs(inertia_c - (voumeAndInertia.at(3))) > epsilon){
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR : Moment of inertia with respect to z axis is incorrect : Expected = " <<
            inertia_c << " " << ", actual = " << (voumeAndInertia.at(3)) << std::endl;
    }

    //test if product of inertia with respect to x axis is correct
	if (abs(inertia_aa - (voumeAndInertia.at(4))) > epsilon){
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR : Product of inertia with respect to x axis is incorrect : Expected = " <<
            inertia_aa << " " << ", actual = " << (voumeAndInertia.at(4)) << std::endl;
    }

    //test if product of inertia with respect to y axis is correct
	if (abs(inertia_bb - (voumeAndInertia.at(5))) > epsilon){
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR : Product of inertia with respect to y axis is incorrect : Expected = " <<
            inertia_bb << " " << ", actual = " << (voumeAndInertia.at(5)) << std::endl;
    }

    //test if product of inertia with respect to z axis is correct
	if (abs(inertia_cc - (voumeAndInertia.at(6))) > epsilon){
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR : Product of inertia with respect to z axis is incorrect : Expected = " <<
            inertia_cc << " " << ", actual = " << (voumeAndInertia.at(6)) << std::endl;
    }

}

void MeshInfoTests::testWithCube(){
	glm::vec3 p0(1.0, -1.0, -1.0);
	glm::vec3 p1(1.0, -1.0, 1.0);
	glm::vec3 p2(-1.0, -1.0, 1.0);
	glm::vec3 p3(-1.0, -1.0, -1.0);
	glm::vec3 p4(1.0, 1.0, -1.0);
	glm::vec3 p5(1.0, 1.0, 1.0);
	glm::vec3 p6(-1.0, 1.0, 1.0);
	glm::vec3 p7(-1.0, 1.0, -1.0);
	vector<glm::vec3> vertices;
    vertices.push_back(p0);
    vertices.push_back(p1);
    vertices.push_back(p2);
    vertices.push_back(p3);
    vertices.push_back(p4);
    vertices.push_back(p5);
    vertices.push_back(p6);
    vertices.push_back(p7);
    std::cout << std::setprecision(10);
    vector<int> triangles = { 0, 1, 2, 0, 2, 3, 4, 7, 6, 4, 6, 5, 0, 4, 5, 0, 5, 1, 1, 5, 6, 1, 6, 2, 2, 6, 
        7, 2, 7, 3, 4, 0, 3, 4, 3, 7 };
    glm::vec3 centerOfMass(0.0, 0.0, 0.0);
    double volume = 8.0;
    double side = 2.0;
    double inertia = (volume * side * side) / 6.0; //inertia of a unit cube is (mass * side * side) /6

    //test with origin as reference point
	meshinfo::MeshInfo massProp1(&vertices, &triangles);
    vector<float> volumeAndInertia1 = massProp1.computeMassProperties();
	if (abs(centerOfMass.x - massProp1.getMeshCentroid().x) > epsilon || abs(centerOfMass.y - massProp1.getMeshCentroid().y) > epsilon ||
		abs(centerOfMass.z - massProp1.getMeshCentroid().z) > epsilon){
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR : Center of mass is incorrect : Expected = " << centerOfMass.x << " " <<
			centerOfMass.y << " " << centerOfMass.z << ", actual = " << massProp1.getMeshCentroid().x << " " <<
			massProp1.getMeshCentroid().y << " " << massProp1.getMeshCentroid().z << std::endl;
    }

	if (abs(volume - volumeAndInertia1.at(0)) > epsilon){
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR : Volume is incorrect : Expected = " << volume <<
            ", actual = " << volumeAndInertia1.at(0) << std::endl;
    }

	if (abs(inertia - (volumeAndInertia1.at(1))) > epsilon || abs(inertia - (volumeAndInertia1.at(2))) > epsilon ||
		abs(inertia - (volumeAndInertia1.at(3))) > epsilon){
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR : Moment of inertia is incorrect : Expected = " << inertia << " " <<
            inertia << " " << inertia << ", actual = " << (volumeAndInertia1.at(1)) << " " << (volumeAndInertia1.at(2)) <<
            " " << (volumeAndInertia1.at(3)) << std::endl;
    }
}

void MeshInfoTests::testWithUnitCube()
{
	glm::vec3 p0(0, 0, 1);
	glm::vec3 p1(1, 0, 1);
	glm::vec3 p2(0, 1, 1);
	glm::vec3 p3(1, 1, 1);
	glm::vec3 p4(0, 0, 0);
	glm::vec3 p5(1, 0, 0);
	glm::vec3 p6(0, 1, 0);
	glm::vec3 p7(1, 1, 0);
	vector<glm::vec3> vertices;
    vertices.push_back(p0);
    vertices.push_back(p1);
    vertices.push_back(p2);
    vertices.push_back(p3);
    vertices.push_back(p4);
    vertices.push_back(p5);
    vertices.push_back(p6);
    vertices.push_back(p7);
    vector<int> triangles = { 0, 1, 2, 1, 3, 2, 2, 3, 7, 2, 7, 6, 1, 7, 3, 1, 5, 7, 6, 7, 4, 7, 5, 4, 0, 4, 1,
        1, 4, 5, 2, 6, 4, 0, 2, 4 };
    glm::vec3 centerOfMass(0.5, 0.5, 0.5);
    double volume = 1.0;
    double side = 1.0;
    double inertia = (volume * side * side) / 6.0; //inertia of a unit cube is (mass * side * side) /6
    std::cout << std::setprecision(10);

    //test with origin as reference point
    meshinfo::MeshInfo massProp1(&vertices, &triangles);
    vector<float> volumeAndInertia1 = massProp1.computeMassProperties();
	if (abs(centerOfMass.x - massProp1.getMeshCentroid().x) > epsilon || abs(centerOfMass.y - massProp1.getMeshCentroid().y) > epsilon ||
		abs(centerOfMass.z - massProp1.getMeshCentroid().z) > epsilon){
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR : Center of mass is incorrect : Expected = " << centerOfMass.x <<
			" " << centerOfMass.y << " " << centerOfMass.z << ", actual = " << massProp1.getMeshCentroid().x << " " <<
			massProp1.getMeshCentroid().y << " " << massProp1.getMeshCentroid().z << std::endl;
    }

	if (abs(volume - volumeAndInertia1.at(0)) > epsilon){
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR : Volume is incorrect : Expected = " << volume <<
            ", actual = " << volumeAndInertia1.at(0) << std::endl;
    }

	if (abs(inertia - (volumeAndInertia1.at(1))) > epsilon || abs(inertia - (volumeAndInertia1.at(2))) > epsilon ||
		abs(inertia - (volumeAndInertia1.at(3))) > epsilon){
        std::cout << __FILE__ << ":" << __LINE__ << " ERROR : Moment of inertia is incorrect : Expected = " << inertia << " " <<
            inertia << " " << inertia << ", actual = " << (volumeAndInertia1.at(1)) << " " << (volumeAndInertia1.at(2)) <<
            " " << (volumeAndInertia1.at(3)) << std::endl;
    }
}
void MeshInfoTests::runAllTests(){
    testWithTetrahedron();
    testWithUnitCube();
    testWithCube();
}