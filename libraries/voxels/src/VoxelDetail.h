//
//  VoxelDetail.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 1/29/2014
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__VoxelDetail__
#define __hifi__VoxelDetail__

#include <QtScript/QScriptEngine>

#include <AABox.h>
#include <SharedUtil.h>
#include "VoxelConstants.h"

struct VoxelDetail {
	float x;
	float y;
	float z;
	float s;
	unsigned char red;
	unsigned char green;
	unsigned char blue;
};

Q_DECLARE_METATYPE(VoxelDetail)

void registerVoxelMetaTypes(QScriptEngine* engine);

QScriptValue voxelDetailToScriptValue(QScriptEngine* engine, const VoxelDetail& color);
void voxelDetailFromScriptValue(const QScriptValue &object, VoxelDetail& color);

class RayToVoxelIntersectionResult {
public:
    RayToVoxelIntersectionResult();
    bool intersects;
    VoxelDetail voxel;
    float distance;
    BoxFace face;
    glm::vec3 intersection;
};

Q_DECLARE_METATYPE(RayToVoxelIntersectionResult)

QScriptValue rayToVoxelIntersectionResultToScriptValue(QScriptEngine* engine, const RayToVoxelIntersectionResult& results);
void rayToVoxelIntersectionResultFromScriptValue(const QScriptValue& object, RayToVoxelIntersectionResult& results);

#endif /* defined(__hifi__VoxelDetail__) */