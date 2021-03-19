//
//  WorldBox.cpp
//
//  Created by Sam Gateau on 01/07/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "WorldBox.h"

#include "OctreeConstants.h"

render::ItemID WorldBoxRenderData::_item{ render::Item::INVALID_ITEM_ID };


namespace render {
    template <> const ItemKey payloadGetKey(const WorldBoxRenderData::Pointer& stuff) { return ItemKey::Builder::opaqueShape().withTagBits(ItemKey::TAG_BITS_0 | ItemKey::TAG_BITS_1); }
    template <> const Item::Bound payloadGetBound(const WorldBoxRenderData::Pointer& stuff, RenderArgs* args) { return Item::Bound(); }
    template <> void payloadRender(const WorldBoxRenderData::Pointer& stuff, RenderArgs* args) {
        if (Menu::getInstance()->isOptionChecked(MenuOption::WorldAxes)) {
            PerformanceTimer perfTimer("worldBox");

            auto& batch = *args->_batch;
            DependencyManager::get<GeometryCache>()->bindSimpleProgram(batch, false, false, false, false, true, args->_renderMethod == Args::RenderMethod::FORWARD);
            WorldBoxRenderData::renderWorldBox(args, batch);
        }
    }
}

void WorldBoxRenderData::renderWorldBox(RenderArgs* args, gpu::Batch& batch) {
    auto geometryCache = DependencyManager::get<GeometryCache>();

    //  Show center of world
    static const glm::vec3 RED(1.0f, 0.0f, 0.0f);
    static const glm::vec3 GREEN(0.0f, 1.0f, 0.0f);
    static const glm::vec3 BLUE(0.0f, 0.0f, 1.0f);
    static const glm::vec3 GREY(0.5f, 0.5f, 0.5f);
    static const glm::vec4 GREY4(0.5f, 0.5f, 0.5f, 1.0f);

    static const glm::vec4 DASHED_RED(1.0f, 0.0f, 0.0f, 1.0f);
    static const glm::vec4 DASHED_GREEN(0.0f, 1.0f, 0.0f, 1.0f);
    static const glm::vec4 DASHED_BLUE(0.0f, 0.0f, 1.0f, 1.0f);
    static const float DASH_LENGTH = 1.0f;
    static const float GAP_LENGTH = 1.0f;
    auto transform = Transform{};
    static std::array<int, 18> geometryIds;
    static std::once_flag initGeometryIds;
    std::call_once(initGeometryIds, [&] {
        for (size_t i = 0; i < geometryIds.size(); ++i) {
            geometryIds[i] = geometryCache->allocateID();
        }
    });

    batch.setModelTransform(transform);

    geometryCache->renderLine(batch, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(HALF_TREE_SCALE, 0.0f, 0.0f), RED, geometryIds[0]);
    geometryCache->renderDashedLine(batch, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-HALF_TREE_SCALE, 0.0f, 0.0f), DASHED_RED,
        DASH_LENGTH, GAP_LENGTH, geometryIds[1]);

    geometryCache->renderLine(batch, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, HALF_TREE_SCALE, 0.0f), GREEN, geometryIds[2]);
    geometryCache->renderDashedLine(batch, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -HALF_TREE_SCALE, 0.0f), DASHED_GREEN,
        DASH_LENGTH, GAP_LENGTH, geometryIds[3]);

    geometryCache->renderLine(batch, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, HALF_TREE_SCALE), BLUE, geometryIds[4]);
    geometryCache->renderDashedLine(batch, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -HALF_TREE_SCALE), DASHED_BLUE,
        DASH_LENGTH, GAP_LENGTH, geometryIds[5]);

    // X center boundaries
    geometryCache->renderLine(batch, glm::vec3(-HALF_TREE_SCALE, -HALF_TREE_SCALE, 0.0f),
        glm::vec3(HALF_TREE_SCALE, -HALF_TREE_SCALE, 0.0f), GREY,
        geometryIds[6]);
    geometryCache->renderLine(batch, glm::vec3(-HALF_TREE_SCALE, -HALF_TREE_SCALE, 0.0f),
        glm::vec3(-HALF_TREE_SCALE, HALF_TREE_SCALE, 0.0f), GREY,
        geometryIds[7]);
    geometryCache->renderLine(batch, glm::vec3(-HALF_TREE_SCALE, HALF_TREE_SCALE, 0.0f),
        glm::vec3(HALF_TREE_SCALE, HALF_TREE_SCALE, 0.0f), GREY,
        geometryIds[8]);
    geometryCache->renderLine(batch, glm::vec3(HALF_TREE_SCALE, -HALF_TREE_SCALE, 0.0f),
        glm::vec3(HALF_TREE_SCALE, HALF_TREE_SCALE, 0.0f), GREY,
        geometryIds[9]);

    // Z center boundaries
    geometryCache->renderLine(batch, glm::vec3(0.0f, -HALF_TREE_SCALE, -HALF_TREE_SCALE),
        glm::vec3(0.0f, -HALF_TREE_SCALE, HALF_TREE_SCALE), GREY,
        geometryIds[10]);
    geometryCache->renderLine(batch, glm::vec3(0.0f, -HALF_TREE_SCALE, -HALF_TREE_SCALE),
        glm::vec3(0.0f, HALF_TREE_SCALE, -HALF_TREE_SCALE), GREY,
        geometryIds[11]);
    geometryCache->renderLine(batch, glm::vec3(0.0f, HALF_TREE_SCALE, -HALF_TREE_SCALE),
        glm::vec3(0.0f, HALF_TREE_SCALE, HALF_TREE_SCALE), GREY,
        geometryIds[12]);
    geometryCache->renderLine(batch, glm::vec3(0.0f, -HALF_TREE_SCALE, HALF_TREE_SCALE),
        glm::vec3(0.0f, HALF_TREE_SCALE, HALF_TREE_SCALE), GREY,
        geometryIds[13]);

    // Center boundaries
    geometryCache->renderLine(batch, glm::vec3(-HALF_TREE_SCALE, 0.0f, -HALF_TREE_SCALE),
        glm::vec3(-HALF_TREE_SCALE, 0.0f, HALF_TREE_SCALE), GREY,
        geometryIds[14]);
    geometryCache->renderLine(batch, glm::vec3(-HALF_TREE_SCALE, 0.0f, -HALF_TREE_SCALE),
        glm::vec3(HALF_TREE_SCALE, 0.0f, -HALF_TREE_SCALE), GREY,
        geometryIds[15]);
    geometryCache->renderLine(batch, glm::vec3(HALF_TREE_SCALE, 0.0f, -HALF_TREE_SCALE),
        glm::vec3(HALF_TREE_SCALE, 0.0f, HALF_TREE_SCALE), GREY,
        geometryIds[16]);
    geometryCache->renderLine(batch, glm::vec3(-HALF_TREE_SCALE, 0.0f, HALF_TREE_SCALE),
        glm::vec3(HALF_TREE_SCALE, 0.0f, HALF_TREE_SCALE), GREY,
        geometryIds[17]);

    auto pipeline = geometryCache->getShapePipelinePointer(false, false, args->_renderMethod == render::Args::RenderMethod::FORWARD);
    geometryCache->renderWireCubeInstance(args, batch, GREY4, pipeline);

    //  Draw meter markers along the 3 axis to help with measuring things
    const float MARKER_DISTANCE = 1.0f;
    const float MARKER_RADIUS = 0.05f;

    transform = Transform().setScale(MARKER_RADIUS);
    batch.setModelTransform(transform);
    geometryCache->renderSolidSphereInstance(args, batch, RED, pipeline);

    transform = Transform().setTranslation(glm::vec3(MARKER_DISTANCE, 0.0f, 0.0f)).setScale(MARKER_RADIUS);
    batch.setModelTransform(transform);
    geometryCache->renderSolidSphereInstance(args, batch, RED, pipeline);

    transform = Transform().setTranslation(glm::vec3(0.0f, MARKER_DISTANCE, 0.0f)).setScale(MARKER_RADIUS);
    batch.setModelTransform(transform);
    geometryCache->renderSolidSphereInstance(args, batch, GREEN, pipeline);

    transform = Transform().setTranslation(glm::vec3(0.0f, 0.0f, MARKER_DISTANCE)).setScale(MARKER_RADIUS);
    batch.setModelTransform(transform);
    geometryCache->renderSolidSphereInstance(args, batch, BLUE, pipeline);

    transform = Transform().setTranslation(glm::vec3(MARKER_DISTANCE, 0.0f, MARKER_DISTANCE)).setScale(MARKER_RADIUS);
    batch.setModelTransform(transform);
    geometryCache->renderSolidSphereInstance(args, batch, GREY, pipeline);
}

