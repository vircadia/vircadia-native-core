//
//  InterfaceParentFinder.cpp
//  interface/src/
//
//  Created by Seth Alves on 2015-10-21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <Application.h>
#include <EntityTree.h>
#include <EntityTreeRenderer.h>
#include <avatar/AvatarManager.h>
#include <AvatarData.h>

#include "InterfaceParentFinder.h"

SpatiallyNestableWeakPointer InterfaceParentFinder::find(QUuid parentID, bool& success, SpatialParentTree* entityTree) const {
    SpatiallyNestableWeakPointer parent;

    if (parentID.isNull()) {
        success = true;
        return parent;
    }

    // search entities
    if (entityTree) {
        parent = entityTree->findByID(parentID);
    } else {
        auto treeRenderer = qApp->getEntities();
        EntityTreePointer tree = treeRenderer ? treeRenderer->getTree() : nullptr;
        parent = tree ? tree->findEntityByEntityItemID(parentID) : nullptr;
    }
    if (!parent.expired()) {
        success = true;
        return parent;
    }

    // search avatars
    QSharedPointer<AvatarManager> avatarManager = DependencyManager::get<AvatarManager>();
    parent = avatarManager->getAvatarBySessionID(parentID);
    if (!parent.expired()) {
        success = true;
        return parent;
    }
    if (parentID == AVATAR_SELF_ID) {
        success = true;
        return avatarManager->getMyAvatar();
    }

    // search overlays
    auto& overlays = qApp->getOverlays();
    auto overlay = overlays.getOverlay(parentID);
    parent = std::dynamic_pointer_cast<SpatiallyNestable>(overlay); // this will return nullptr for non-3d overlays
    if (!parent.expired()) {
        success = true;
        return parent;
    }

    success = false;
    return parent;
}
