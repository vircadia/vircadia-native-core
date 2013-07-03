//
//  Voxelizer.h
//  skpimporter
//
//  Created by Stojce Slavkovski on 6/23/13.
//  Copyright (c) 2013 High Fidelity. All rights reserved.
//

#ifndef __skpimporter__Voxelizer__
#define __skpimporter__Voxelizer__

#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include "Triangle.h"

namespace voxelImport {
    using namespace std;
    using namespace glm;
    
    class Voxelizer {
    private:
        vector<Triangle> triangles;
    public:
        vector<vec3> Voxelize(vector<vec3> vertices, float voxelSize);
    };
}

#endif /* defined(__skpimporter__Voxelizer__) */
