//
//  Created by Bradley Austin Davis on 2016/05/16
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TestInstancedShapes.h"

gpu::Stream::FormatPointer& getInstancedSolidStreamFormat();

static const size_t TYPE_COUNT = 4;
static const size_t ITEM_COUNT = 50;
static const float SHAPE_INTERVAL = (PI * 2.0f) / ITEM_COUNT;
static const float ITEM_INTERVAL = SHAPE_INTERVAL / TYPE_COUNT;

static GeometryCache::Shape SHAPE[TYPE_COUNT] = {
    GeometryCache::Icosahedron,
    GeometryCache::Cube,
    GeometryCache::Sphere,
    GeometryCache::Tetrahedron,
};

const gpu::Element POSITION_ELEMENT { gpu::VEC3, gpu::FLOAT, gpu::XYZ };
const gpu::Element NORMAL_ELEMENT { gpu::VEC3, gpu::FLOAT, gpu::XYZ };
const gpu::Element COLOR_ELEMENT { gpu::VEC4, gpu::NUINT8, gpu::RGBA };


TestInstancedShapes::TestInstancedShapes() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    colorBuffer = std::make_shared<gpu::Buffer>();

    static const float ITEM_RADIUS = 20;
    static const vec3 ITEM_TRANSLATION { 0, 0, -ITEM_RADIUS };
    for (size_t i = 0; i < TYPE_COUNT; ++i) {
        GeometryCache::Shape shape = SHAPE[i];
        GeometryCache::ShapeData shapeData = geometryCache->_shapes[shape];
        //indirectCommand._count
        float startingInterval = ITEM_INTERVAL * i;
        std::vector<mat4> typeTransforms;
        for (size_t j = 0; j < ITEM_COUNT; ++j) {
            float theta = j * SHAPE_INTERVAL + startingInterval;
            auto transform = glm::rotate(mat4(), theta, Vectors::UP);
            transform = glm::rotate(transform, (randFloat() - 0.5f) * PI / 4.0f, Vectors::UNIT_X);
            transform = glm::translate(transform, ITEM_TRANSLATION);
            transform = glm::scale(transform, vec3(randFloat() / 2.0f + 0.5f));
            typeTransforms.push_back(transform);
            auto color = vec4 { randomColorValue(64), randomColorValue(64), randomColorValue(64), 255 };
            color /= 255.0f;
            colors.push_back(color);
            colorBuffer->append(toCompactColor(color));
        }
        transforms.push_back(typeTransforms);
    }
}

void TestInstancedShapes::renderTest(size_t testId, RenderArgs* args) {
    gpu::Batch& batch = *(args->_batch);
    auto geometryCache = DependencyManager::get<GeometryCache>();
    geometryCache->bindSimpleProgram(batch);
    batch.setInputFormat(getInstancedSolidStreamFormat());
    for (size_t i = 0; i < TYPE_COUNT; ++i) {
        GeometryCache::Shape shape = SHAPE[i];
        GeometryCache::ShapeData shapeData = geometryCache->_shapes[shape];
        
        std::string namedCall = __FUNCTION__ + std::to_string(i);

        //batch.addInstanceModelTransforms(transforms[i]);
        for (size_t j = 0; j < ITEM_COUNT; ++j) {
            batch.setModelTransform(transforms[i][j]);
            batch.setupNamedCalls(namedCall, [=](gpu::Batch& batch, gpu::Batch::NamedBatchData&) {
                batch.setInputBuffer(gpu::Stream::COLOR, gpu::BufferView(colorBuffer, i * ITEM_COUNT * 4, colorBuffer->getSize(), COLOR_ELEMENT));
                shapeData.drawInstances(batch, ITEM_COUNT);
            });
        }

        //for (size_t j = 0; j < ITEM_COUNT; ++j) {
        //    batch.setModelTransform(transforms[j + i * ITEM_COUNT]);
        //    shapeData.draw(batch);
        //}
    }
}

