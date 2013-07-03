//
//  Triangle.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 6/27/13.
//
//

#include "Triangle.h"
namespace voxelImport {
    
#define voxelOffset 2
    
    int divideCeiling(double a, double b) {
        return (int) (1 + ((a - 1) / b));
    }
    
    Triangle::Triangle(vec3 a, vec3 b, vec3 c) {
        _a = a;
        _b = b;
        _c = c;
        
        double minX = fmin(a.x, fmin(b.x, c.x));
        double maxX = fmax(a.x, fmax(b.x, c.x));
        double minY = fmin(a.y, fmin(b.y, c.y));
        double maxY = fmax(a.y, fmax(b.y, c.y));
        double minZ = fmin(a.z, fmin(b.z, c.z));
        double maxZ = fmax(a.z, fmax(b.z, c.z));
        
        vec3 min = vec3(minX, minY, minZ);
        vec3 max = vec3(maxX, maxY, maxZ);
        
        _boundingBox = new AABox(min, max);
    }
    
    vector<vec3> Triangle::getCollidingVoxels(float voxelSize) {
        vector<vec3> result;
        
        vec3 min = _boundingBox->getCorner();
        vec3 max = _boundingBox->getSize();
        
        vec3 minPoint = min - voxelSize * voxelOffset;
        vec3 size = (max + voxelSize * voxelOffset) - minPoint;
        
        vec3 deltaP = vec3(voxelSize, voxelSize, voxelSize);
        
        // create temp voxels inside of the box
        vec3 length = vec3(divideCeiling(size.x, deltaP.x), divideCeiling(size.y , deltaP.y), int(size.z / deltaP.z));
        
        // iterate over each voxel
        for (size_t iz = 0; iz <= length.z; iz++) {
            for (size_t iy = 0; iy <= length.y; iy++) {
                for (size_t ix = 0; ix <= length.x; ix++) {
                    
                    vec3 voxel = minPoint + vec3(ix, iy, iz) * deltaP;
                    // test if collides with triangle pane
                    if (this->isCollidingWithVoxel(voxel, deltaP)) {
                        // add it to result voxels
                        result.push_back(voxel);
                    }
                }
            }
        }
        
        return result;
    }
    
    bool Triangle::isCollidingWithVoxel(vec3 voxelCenter, vec3 delta) {
        
        vec3 v01 = _b - _a;
        vec3 v02 = _c - _a;
        
        vec3 crossVector = cross(v01, v02);
        vec3 faceNormal = normalize(crossVector);
        
        vec3 criticalPoint = vec3(faceNormal.x > 0 ? delta.x : 0, faceNormal.y > 0 ? delta.y : 0, faceNormal.z > 0 ? delta.z : 0);
        
        // (1)
        // Check whether minPoint + criticalPoint and the opposite box corner minPoint + (deltaP - criticalPoint)
        // are on different sides of the plane or one of them is on the plane.
        vec3 minPoint = criticalPoint - _a;
        float d1 = dot(faceNormal, minPoint);
        minPoint = delta - criticalPoint - _a;
        float d2 = dot(faceNormal, minPoint);
        
        bool isTrianglePlaneOverlappingVoxel = (dot(faceNormal, voxelCenter) + d1) * (dot(faceNormal, voxelCenter) + d2) <= 0;
        
        if (!isTrianglePlaneOverlappingVoxel) {
            return false;
        }
        
        vector<vec3> faceEdge(3);
        faceEdge[0] = _b - _a;
        faceEdge[1] = _c - _b;
        faceEdge[2] = _a - _c;
        
        for (size_t i =0; i < 3; i++) {
            
            vec3 v;
            
            if (i == 0) {
                v = _a;
            } else if (i == 1) {
                v = _b;
            } else {
                v = _c;
            }
            
            if (!testVoxelEdgeXYProjection(v, delta, voxelCenter, faceNormal, faceEdge[i])) {
                return false;
            }
            
            if (!testVoxelEdgeYZProjection(v, delta, voxelCenter, faceNormal, faceEdge[i])) {
                return false;
            }
            
            if (!testVoxelEdgeZXProjection(v, delta, voxelCenter, faceNormal, faceEdge[i])) {
                return false;
            }
        }
        
        return true;
    }
    
    bool Triangle::testVoxelEdgeXYProjection(vec3 triVertex, vec3 delta, vec3 voxelEvalPoint, vec3 faceNormal, vec3 faceEdge) {
        // XY Plane
        vec2 eNormal = vec2(-faceEdge.y, faceEdge.x);
        eNormal *= faceNormal.z >= 0.0f ? 1.0f : -1.0f;
        float Dxy = - dot(eNormal, vec2(triVertex.x, triVertex.y)) + fmax(0.0f, delta.x * eNormal.x) + fmax(0.0f, delta.y * eNormal.y);
        float test = dot(eNormal, vec2(voxelEvalPoint.x, voxelEvalPoint.y)) + Dxy;
        return test >= 0.0f;
    }
    
    bool Triangle::testVoxelEdgeYZProjection(vec3 triVertex, vec3 delta, vec3 voxelEvalPoint, vec3 faceNormal, vec3 faceEdge) {
        // YZ Plane
        vec2 eNormal = vec2(-faceEdge.z, faceEdge.y);
        eNormal *= faceNormal.x >= 0.0f ? 1.0f : -1.0f;
        double Dyz = - dot(eNormal, vec2(triVertex.y, triVertex.z)) + fmax(0.0f, delta.y * eNormal.x) + fmax(0.0f, delta.z * eNormal.y);
        double test = dot(eNormal, vec2(voxelEvalPoint.y, voxelEvalPoint.z)) + Dyz;
        return test >= 0.0f;
    }
    
    bool Triangle::testVoxelEdgeZXProjection(vec3 triVertex, vec3 delta, vec3 voxelEvalPoint, vec3 faceNormal, vec3 faceEdge) {
        // ZX Plane
        vec2 eNormal = vec2(-faceEdge.x, faceEdge.z);
        eNormal *= faceNormal.y >= 0.0f ? 1.0f : -1.0f;
        float Dzx = -dot(eNormal, vec2(triVertex.z, triVertex.x)) + fmax(0.0f, delta.z * eNormal.x) + fmax(0.0f, delta.x * eNormal.y);
        double test = dot(eNormal, vec2(voxelEvalPoint.z, voxelEvalPoint.x)) + Dzx;
        return test >= 0.0f;
    }
}
