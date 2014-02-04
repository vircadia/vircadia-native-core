//
//  VoxelDetail.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 1/29/2014
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
#include "VoxelDetail.h"

void registerVoxelMetaTypes(QScriptEngine* engine) {
    qScriptRegisterMetaType(engine, voxelDetailToScriptValue, voxelDetailFromScriptValue);
}

QScriptValue voxelDetailToScriptValue(QScriptEngine* engine, const VoxelDetail& voxelDetail) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", voxelDetail.x * (float)TREE_SCALE);
    obj.setProperty("y", voxelDetail.y * (float)TREE_SCALE);
    obj.setProperty("z", voxelDetail.z * (float)TREE_SCALE);
    obj.setProperty("s", voxelDetail.s * (float)TREE_SCALE);
    obj.setProperty("red", voxelDetail.red);
    obj.setProperty("green", voxelDetail.green);
    obj.setProperty("blue", voxelDetail.blue);
    return obj;
}

void voxelDetailFromScriptValue(const QScriptValue &object, VoxelDetail& voxelDetail) {
    voxelDetail.x = object.property("x").toVariant().toFloat() / (float)TREE_SCALE;
    voxelDetail.y = object.property("y").toVariant().toFloat() / (float)TREE_SCALE;
    voxelDetail.z = object.property("z").toVariant().toFloat() / (float)TREE_SCALE;
    voxelDetail.s = object.property("s").toVariant().toFloat() / (float)TREE_SCALE;
    voxelDetail.red = object.property("red").toVariant().toInt();
    voxelDetail.green = object.property("green").toVariant().toInt();
    voxelDetail.blue = object.property("blue").toVariant().toInt();
}



