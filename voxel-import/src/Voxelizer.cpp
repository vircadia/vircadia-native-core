//
//  Voxelizer.cpp
//  skpimporter
//
//  Created by Stojce Slavkovski on 6/23/13.
//  Copyright (c) 2013 High Fidelity. All rights reserved.
//

#include "Voxelizer.h"
namespace voxelImport {
    
    using namespace std;
    using namespace glm;
    
    vector<vec3> Voxelizer::Voxelize(vector<vec3> vertices, float voxelSize) {
        
        vector<vec3> voxels;
        
        if (vertices.size() < 3) {
            throw;
        }
        
        vec3 aPoint = vertices[0];
        vec3 bPoint = vertices[1];
        vec3 cPoint = vertices[2];
        
        // first triangle
        Triangle* t = new Triangle(aPoint, bPoint, cPoint);
        triangles.push_back(*t);
        
        for (size_t i = 3; i < vertices.size(); i++) {
            bPoint = cPoint;
            t = new Triangle(aPoint, bPoint, vertices[i]);
            triangles.push_back(*t);
        }
        
        for (size_t i = 0; i < triangles.size(); i++) {
            vector<vec3> colliding = triangles[i].getCollidingVoxels(voxelSize);
            voxels.insert(voxels.end(), colliding.begin(), colliding.end());
        }
        
        return voxels;
    }
}