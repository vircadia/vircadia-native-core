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

#include "InterfaceParentFinder.h"

#include <QtCore/QSharedPointer>

#include <Application.h>
#include <AvatarData.h>
#include <avatar/AvatarManager.h>
#include <EntityTree.h>
#include <EntityTreeRenderer.h>

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

    success = false;
    return parent;
}
