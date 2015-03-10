//
//  MeshInfo.h
//  libraries/physics/src
//
//  Created by Virendra Singh 2015.02.28
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_MeshInfo_h
#define hifi_MeshInfo_h
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
using namespace std;
namespace meshinfo{
    typedef glm::vec3 Vertex;
    class MeshInfo{
    private:
        inline float getVolume(const Vertex p1, const Vertex p2, const Vertex p3, const Vertex p4) const;
        vector<float> computeVolumeAndInertia(const Vertex p1, const Vertex p2, const Vertex p3, const Vertex p4) const;
    public:
        vector<Vertex> *_vertices;
        Vertex _centerOfMass;
        vector<int> *_triangles;
        MeshInfo(vector<Vertex> *vertices, vector<int> *triangles);
        ~MeshInfo();
        Vertex getMeshCentroid() const;
        vector<float> computeMassProperties();		
    };
}
#endif // hifi_MeshInfo_h