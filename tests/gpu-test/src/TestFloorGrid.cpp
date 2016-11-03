//
//  Created by Bradley Austin Davis on 2016/05/16
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TestFloorGrid.h"


TestFloorGrid::TestFloorGrid() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    // Render grid on xz plane (not the optimal way to do things, but w/e)
    // Note: GeometryCache::renderGrid will *not* work, as it is apparenly unaffected by batch rotations and renders xy only
    static const std::string GRID_INSTANCE = "Grid";
    static auto compactColor1 = toCompactColor(vec4 { 0.35f, 0.25f, 0.15f, 1.0f });
    static auto compactColor2 = toCompactColor(vec4 { 0.15f, 0.25f, 0.35f, 1.0f });
    static std::vector<glm::mat4> transforms;
    static gpu::BufferPointer colorBuffer;
    if (!transforms.empty()) {
        transforms.reserve(200);
        colorBuffer = std::make_shared<gpu::Buffer>();
        for (int i = 0; i < 100; ++i) {
            {
                glm::mat4 transform = glm::translate(mat4(), vec3(0, -1, -50 + i));
                transform = glm::scale(transform, vec3(100, 1, 1));
                transforms.push_back(transform);
                colorBuffer->append(compactColor1);
            }

            {
                glm::mat4 transform = glm::mat4_cast(quat(vec3(0, PI / 2.0f, 0)));
                transform = glm::translate(transform, vec3(0, -1, -50 + i));
                transform = glm::scale(transform, vec3(100, 1, 1));
                transforms.push_back(transform);
                colorBuffer->append(compactColor2);
            }
        }
    }

}

void TestFloorGrid::renderTest(size_t testId, RenderArgs* args) {
    //gpu::Batch& batch = *(args->_batch);
    //auto pipeline = geometryCache->getSimplePipeline();
    //for (auto& transform : transforms) {
    //    batch.setModelTransform(transform);
    //    batch.setupNamedCalls(GRID_INSTANCE, [=](gpu::Batch& batch, gpu::Batch::NamedBatchData& data) {
    //        batch.setPipeline(_pipeline);
    //        geometryCache->renderWireShapeInstances(batch, GeometryCache::Line, data.count(), colorBuffer);
    //    });
    //}
}
