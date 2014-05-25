//
//  VoxelDetail.h
//  libraries/voxels/src
//
//  Created by Brad Hefta-Gaub on 1/29/2014.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VoxelDetail_h
#define hifi_VoxelDetail_h

#include <QtScript/QScriptEngine>

#include <AACube.h>
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
    bool accurate;
    VoxelDetail voxel;
    float distance;
    BoxFace face;
    glm::vec3 intersection;
};

Q_DECLARE_METATYPE(RayToVoxelIntersectionResult)

QScriptValue rayToVoxelIntersectionResultToScriptValue(QScriptEngine* engine, const RayToVoxelIntersectionResult& results);
void rayToVoxelIntersectionResultFromScriptValue(const QScriptValue& object, RayToVoxelIntersectionResult& results);

#endif // hifi_VoxelDetail_h
