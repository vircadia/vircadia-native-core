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

#include "AvatarCertifyBanner.h"

#include <QtGui/QDesktopServices>

#include "ui/TabletScriptingInterface.h"
#include "EntityTreeRenderer.h"

namespace {
    const QUrl AVATAR_THEFT_BANNER_IMAGE = PathUtils::resourcesUrl("images/AvatarTheftBanner.png");
    const QString AVATAR_THEFT_BANNER_SCRIPT { "/system/clickToAvatarApp.js" };
}

AvatarCertifyBanner::AvatarCertifyBanner(QQuickItem* parent) {
}

void AvatarCertifyBanner::show(const QUuid& avatarID) {
    if (!_active) {
        auto entityTreeRenderer = DependencyManager::get<EntityTreeRenderer>();
        EntityTreePointer entityTree = entityTreeRenderer->getTree();
        if (!entityTree) {
            return;
        }
        const bool tabletShown = DependencyManager::get<TabletScriptingInterface>()->property("tabletShown").toBool();
        const auto& position = tabletShown ? glm::vec3(0.0f, 0.0f, -1.8f) : glm::vec3(0.0f, 0.0f, -0.7f);
        const float scaleFactor = tabletShown ? 2.6f : 1.0f;

        EntityItemProperties entityProperties;
        entityProperties.setType(EntityTypes::Image);
        entityProperties.setEntityHostType(entity::HostType::LOCAL);
        entityProperties.setImageURL(AVATAR_THEFT_BANNER_IMAGE.toString());
        entityProperties.setName("hifi-avatar-notification-banner");
        entityProperties.setParentID(avatarID);
        entityProperties.setParentJointIndex(CAMERA_MATRIX_INDEX);
        entityProperties.setLocalPosition(position);
        entityProperties.setDimensions(glm::vec3(1.0f, 1.0f, 0.3f) * scaleFactor);
        entityProperties.setEmissive(true);
        entityProperties.setRenderLayer(tabletShown ? RenderLayer::WORLD : RenderLayer::FRONT);
        entityProperties.getGrab().setGrabbable(false);
        QString scriptPath = QUrl(PathUtils::defaultScriptsLocation("")).toString() + AVATAR_THEFT_BANNER_SCRIPT;
        entityProperties.setScript(scriptPath);
        entityProperties.setVisible(true);

        entityTree->withWriteLock([&] {
            auto entityTreeItem = entityTree->addEntity(_bannerID, entityProperties);
            entityTreeItem->setLocalPosition(position);
        });

        _active = true;
    }
}

void AvatarCertifyBanner::clear() {
    if (_active) {
        DependencyManager::get<EntityTreeRenderer>()->deleteEntity(_bannerID);
        _active = false;
    }
}
