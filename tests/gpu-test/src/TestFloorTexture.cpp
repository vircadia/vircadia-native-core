//
//  Created by Bradley Austin Davis on 2016/05/16
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TestFloorTexture.h"

struct Vertex {
    vec4 position;
    vec4 texture;
    vec4 normal;
    vec4 color;
};
static const uint TEXTURE_OFFSET = offsetof(Vertex, texture);
static const uint NORMAL_OFFSET = offsetof(Vertex, normal);
static const uint POSITION_OFFSET = offsetof(Vertex, position);
static const uint COLOR_OFFSET = offsetof(Vertex, color); 
static const gpu::Element POSITION_ELEMENT { gpu::VEC3, gpu::FLOAT, gpu::XYZ };
static const gpu::Element TEXTURE_ELEMENT { gpu::VEC2, gpu::FLOAT, gpu::UV };
static const gpu::Element NORMAL_ELEMENT { gpu::VEC3, gpu::FLOAT, gpu::XYZ };
static const gpu::Element COLOR_ELEMENT { gpu::VEC4, gpu::FLOAT, gpu::RGBA };

FloorTextureTest::FloorTextureTest() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    std::vector<Vertex> vertices;
    const int MINX = -1000;
    const int MAXX = 1000;

    vertices.push_back({ 
        vec4(MAXX, 0, MAXX, 1),
        vec4(MAXX, MAXX, 0, 0),
        vec4(0, 1, 0, 1),
        vec4(1),
    });

    vertices.push_back({
        vec4(MAXX, 0, MINX, 1),
        vec4(MAXX, 0, 0, 0),
        vec4(0, 1, 0, 1),
        vec4(1),
    });

    vertices.push_back({
        vec4(MINX, 0, MINX, 1),
        vec4(0, 0, 0, 0),
        vec4(0, 1, 0, 1),
        vec4(1),
    });

    vertices.push_back({
        vec4(MINX, 0, MAXX, 1),
        vec4(0, MAXX, 0, 0),
        vec4(0, 1, 0, 1),
        vec4(1),
    });

    vertexBuffer->append(vertices);
    indexBuffer->append(std::vector<uint16_t>({ 0, 1, 2, 2, 3, 0 }));
    texture = DependencyManager::get<TextureCache>()->getImageTexture("C:/Users/bdavis/Git/openvr/samples/bin/cube_texture.png");
    //texture = DependencyManager::get<TextureCache>()->getImageTexture("H:/test.png");
    //texture = DependencyManager::get<TextureCache>()->getImageTexture("H:/crate_blue.fbm/lambert8SG_Normal_OpenGL.png");
    vertexFormat->setAttribute(gpu::Stream::POSITION, 0, POSITION_ELEMENT, POSITION_OFFSET);
    vertexFormat->setAttribute(gpu::Stream::TEXCOORD, 0, TEXTURE_ELEMENT, TEXTURE_OFFSET);
    vertexFormat->setAttribute(gpu::Stream::COLOR, 0, COLOR_ELEMENT, COLOR_OFFSET);
    vertexFormat->setAttribute(gpu::Stream::NORMAL, 0, NORMAL_ELEMENT, NORMAL_OFFSET);
}

void FloorTextureTest::renderTest(size_t testId, RenderArgs* args) {
    gpu::Batch& batch = *(args->_batch);
    auto geometryCache = DependencyManager::get<GeometryCache>();
    static auto start = usecTimestampNow();
    auto now = usecTimestampNow();
    if ((now - start) > USECS_PER_SECOND * 1) {
        start = now;
        texture->incremementMinMip();
    }

    geometryCache->bindSimpleProgram(batch, true, false, true, true);
    batch.setInputBuffer(0, vertexBuffer, 0, sizeof(Vertex));
    batch.setInputFormat(vertexFormat);
    batch.setIndexBuffer(gpu::UINT16, indexBuffer, 0);
    batch.setResourceTexture(0, texture);
    batch.setModelTransform(glm::translate(glm::mat4(), vec3(0, -0.1, 0)));
    batch.drawIndexed(gpu::TRIANGLES, 6, 0);
}
