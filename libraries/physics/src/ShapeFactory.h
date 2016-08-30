//
//  ShapeFactory.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2014.12.01
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ShapeFactory_h
#define hifi_ShapeFactory_h

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

#include <ShapeInfo.h>

// translates between ShapeInfo and btShape

namespace ShapeFactory {
    const btCollisionShape* createShapeFromInfo(const ShapeInfo& info);
    void deleteShape(const btCollisionShape* shape);

    //btTriangleIndexVertexArray* createStaticMeshArray(const ShapeInfo& info);
    //void deleteStaticMeshArray(btTriangleIndexVertexArray* dataArray);

    class StaticMeshShape : public btBvhTriangleMeshShape {
    public:
        StaticMeshShape() = delete;
        StaticMeshShape(btTriangleIndexVertexArray* dataArray);
        ~StaticMeshShape();

    private:
        // the StaticMeshShape owns its vertex/index data
        btTriangleIndexVertexArray* _dataArray;
    };
};

#endif // hifi_ShapeFactory_h
