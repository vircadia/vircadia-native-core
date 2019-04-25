//
//  AvatarCertifyBanner.h
//  interface/src/ui
//
//  Created by Simon Walton, April 2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtGui/QDesktopServices>

#include "OffscreenQmlElement.h"
#include "EntityTreeRenderer.h"
#include "AvatarCertifyBanner.h"

static const QUrl AVATAR_THEFT_DIALOG = PathUtils::qmlUrl("AvatarTheftSettings.qml");
static const QUrl AVATAR_THEFT_BANNER_IMAGE = PathUtils::resourcesUrl("images/AvatarTheftBanner.png");

AvatarCertifyBanner::AvatarCertifyBanner(QQuickItem* parent) {

}

AvatarCertifyBanner::~AvatarCertifyBanner()
{ }

void AvatarCertifyBanner::show(const QUuid& avatarID, int jointIndex) {
    if (!_active) {
        auto entityTreeRenderer = DependencyManager::get<EntityTreeRenderer>();
        EntityTreePointer entityTree = entityTreeRenderer->getTree();
        if (!entityTree) {
            return;
        }
        glm::vec3 position { 0.0f, 0.0f, -0.7f };
        EntityItemProperties entityProperties;
        entityProperties.setType(EntityTypes::Image);
        entityProperties.setImageURL(AVATAR_THEFT_BANNER_IMAGE.toString());
        entityProperties.setName("hifi-avatar-theft-banner");
        entityProperties.setParentID(avatarID);
        entityProperties.setParentJointIndex(CAMERA_MATRIX_INDEX);
        entityProperties.setLocalPosition(position);
        entityProperties.setDimensions({ 1.0f, 1.0f, 0.3f });
        entityProperties.setRenderLayer(RenderLayer::WORLD);
        entityProperties.getGrab().setGrabbable(false);
        entityProperties.setVisible(true);

        entityTree->withWriteLock([&] {
            auto entityTreeItem = entityTree->addEntity(_bannerID, entityProperties);
            entityTreeItem->setLocalPosition(position);  // ?!
        });

        _active = true;
    }
}

void AvatarCertifyBanner::clear() {
    if (_active) {
        auto entityTreeRenderer = DependencyManager::get<EntityTreeRenderer>();
        EntityTreePointer entityTree = entityTreeRenderer->getTree();
        if (!entityTree) {
            return;
        }

        entityTree->withWriteLock([&] {
            entityTree->deleteEntity(_bannerID);
        });

        _active = false;
    }
}
