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

#include "InterfaceParentFinder.h"

SpatiallyNestableWeakPointer InterfaceParentFinder::find(QUuid parentID) const {
    SpatiallyNestableWeakPointer parent;

    // search entities
    EntityTreeRenderer* treeRenderer = qApp->getEntities();
    EntityTreePointer tree = treeRenderer->getTree();
    parent = tree->findEntityByEntityItemID(parentID);
    if (!parent.expired()) {
        return parent;
    }

    // search avatars
    QSharedPointer<AvatarManager> avatarManager = DependencyManager::get<AvatarManager>();
    return avatarManager->getAvatarBySessionID(parentID);
}
