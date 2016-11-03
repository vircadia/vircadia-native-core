//
//  Created by Bradley Austin Davis on 2016/05/16
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TestShapes.h"

static const size_t TYPE_COUNT = 6;

static GeometryCache::Shape SHAPE[TYPE_COUNT] = {
    GeometryCache::Cube,
    GeometryCache::Tetrahedron,
    GeometryCache::Octahedron,
    GeometryCache::Dodecahedron,
    GeometryCache::Icosahedron,
    GeometryCache::Sphere,
};

void TestShapes::renderTest(size_t testId, RenderArgs* args) {
    gpu::Batch& batch = *(args->_batch);
    auto geometryCache = DependencyManager::get<GeometryCache>();
    geometryCache->bindSimpleProgram(batch);

    // Render unlit cube + sphere
    static auto startSecs = secTimestampNow();
    float seconds = secTimestampNow() - startSecs;
    seconds /= 4.0f;
    batch.setModelTransform(Transform());
    batch._glColor4f(0.8f, 0.25f, 0.25f, 1.0f);

    bool wire = (seconds - floorf(seconds) > 0.5f);
    int shapeIndex = ((int)seconds) % TYPE_COUNT;
    if (wire) {
        geometryCache->renderWireShape(batch, SHAPE[shapeIndex]);
    } else {
        geometryCache->renderShape(batch, SHAPE[shapeIndex]);
    }

    batch.setModelTransform(Transform().setScale(1.01f));
    batch._glColor4f(1, 1, 1, 1);
    geometryCache->renderWireCube(batch);
}



