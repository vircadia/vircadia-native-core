//
//  SelectedWorkloadRenderer.cpp
//
//  Created by Andrew Meadows 2019.11.08
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "SelectedWorkloadRenderer.h"

#include <cstring>
#include <gpu/Context.h>

#include <workload/Space.h>

#include "Application.h"
#include "GameWorkloadRenderer.h"
#include "scripting/SelectionScriptingInterface.h"

void SelectedWorkloadRenderer::run(const workload::WorkloadContextPointer& runContext, Outputs& outputs) {
    auto gameWorkloadContext = std::dynamic_pointer_cast<GameWorkloadContext>(runContext);
    if (!gameWorkloadContext) {
        return;
    }
    auto space = gameWorkloadContext->_space;
    if (!space) {
        return;
    }

    render::Transaction transaction;
    auto scene = gameWorkloadContext->_scene;

    auto selection = DependencyManager::get<SelectionScriptingInterface>();
    // Note: the "DebugWorkloadSelection" name is a secret hard-coded C++ debug feature.
    // If you create such a named list using JS and the "Selection" API then it will be picked up here
    // and the workload proxies for corresponding entities will be rendered.
    GameplayObjects selectedObjects = selection->getList("DebugWorkloadSelection");

    if (!selectedObjects.getContainsData()) {
        // nothing to render
        // clear item if it exists and bail
        if (render::Item::isValidID(_spaceRenderItemID)) {
            transaction.updateItem<GameWorkloadRenderItem>(_spaceRenderItemID, [](GameWorkloadRenderItem& item) {
                item.setVisible(false);
            });
            scene->enqueueTransaction(transaction);
        }
        return;
    }

    std::vector<EntityItemID> entityIDs = selectedObjects.getEntityIDs();
    workload::indexed_container::Indices indices;
    indices.reserve(entityIDs.size());

    auto entityTreeRenderer = qApp->getEntities();
    auto entityTree = entityTreeRenderer->getTree();
    for (auto id : entityIDs) {
        EntityItemPointer entity = entityTree->findEntityByID(id);
        if (entity) {
            indices.push_back(entity->getSpaceIndex());
        }
    }

    workload::Proxy::Vector proxies;
    proxies.reserve(indices.size());
    space->copySelectedProxyValues(proxies, indices);

    if (!render::Item::isValidID(_spaceRenderItemID)) {
        _spaceRenderItemID = scene->allocateID();
        auto renderItem = std::make_shared<GameWorkloadRenderItem>();
        renderItem->editBound().setBox(glm::vec3(-16000.0f), 32000.0f);
        transaction.resetItem(_spaceRenderItemID, std::make_shared<GameWorkloadRenderItem::Payload>(renderItem));
    }

    bool showProxies = true;
    bool showViews = false;
    bool visible = true;
    workload::Views views(0);
    transaction.updateItem<GameWorkloadRenderItem>(_spaceRenderItemID, [visible, showProxies, proxies, showViews, views](GameWorkloadRenderItem& item) {
        item.setVisible(visible);
        item.showProxies(showProxies);
        item.setAllProxies(proxies);
        item.showViews(showViews);
        item.setAllViews(views);
    });
    scene->enqueueTransaction(transaction);
}
