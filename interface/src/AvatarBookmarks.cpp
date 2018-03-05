//
//  AvatarBookmarks.cpp
//  interface/src
//
//  Created by Triplelexx on 23/03/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QAction>
#include <QInputDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QQmlContext>
#include <QList>

#include <Application.h>
#include <OffscreenUi.h>
#include <avatar/AvatarManager.h>
#include <EntityItemID.h>
#include <EntityTree.h>
#include <PhysicalEntitySimulation.h>
#include <EntityEditPacketSender.h>
#include <VariantMapToScriptValue.h>

#include "MainWindow.h"
#include "Menu.h"
#include "AvatarBookmarks.h"
#include "InterfaceLogging.h"

#include "QVariantGLM.h"

#include <QtQuick/QQuickWindow>


void addAvatarEntities(const QVariantList& avatarEntities) {
    auto nodeList = DependencyManager::get<NodeList>();
    const QUuid myNodeID = nodeList->getSessionUUID();
    EntityTreePointer entityTree = DependencyManager::get<EntityTreeRenderer>()->getTree();
    if (!entityTree) {
        return;
    }
    EntitySimulationPointer entitySimulation = entityTree->getSimulation();
    PhysicalEntitySimulationPointer physicalEntitySimulation = std::static_pointer_cast<PhysicalEntitySimulation>(entitySimulation);
    EntityEditPacketSender* entityPacketSender = physicalEntitySimulation->getPacketSender();
    QScriptEngine scriptEngine;
    for (int index = 0; index < avatarEntities.count(); index++) {
        const QVariantMap& avatarEntityProperties = avatarEntities.at(index).toMap();
        QVariant variantProperties = avatarEntityProperties["properties"];
        QVariantMap asMap = variantProperties.toMap();
        QScriptValue scriptProperties = variantMapToScriptValue(asMap, scriptEngine);
        EntityItemProperties entityProperties;
        EntityItemPropertiesFromScriptValueHonorReadOnly(scriptProperties, entityProperties);

        entityProperties.setParentID(myNodeID);
        entityProperties.setClientOnly(true);
        entityProperties.setOwningAvatarID(myNodeID);
        entityProperties.setSimulationOwner(myNodeID, AVATAR_ENTITY_SIMULATION_PRIORITY);
        entityProperties.markAllChanged();

        EntityItemID id = EntityItemID(QUuid::createUuid());
        bool success = true;
        entityTree->withWriteLock([&] {
            EntityItemPointer entity = entityTree->addEntity(id, entityProperties);
            if (entity) {
                if (entityProperties.queryAACubeRelatedPropertyChanged()) {
                    // due to parenting, the server may not know where something is in world-space, so include the bounding cube.
                    bool success;
                    AACube queryAACube = entity->getQueryAACube(success);
                    if (success) {
                        entityProperties.setQueryAACube(queryAACube);
                    }
                }

                entity->setLastBroadcast(usecTimestampNow());
                // since we're creating this object we will immediately volunteer to own its simulation
                entity->flagForOwnershipBid(VOLUNTEER_SIMULATION_PRIORITY);
                entityProperties.setLastEdited(entity->getLastEdited());
            } else {
                qCDebug(entities) << "AvatarEntitiesBookmark failed to add new Entity to local Octree";
                success = false;
            }
        });

        if (success) {
            entityPacketSender->queueEditEntityMessage(PacketType::EntityAdd, entityTree, id, entityProperties);
        }
    }
}

AvatarBookmarks::AvatarBookmarks() {
    _bookmarksFilename = PathUtils::getAppDataPath() + "/" + AVATARBOOKMARKS_FILENAME;
    readFromFile();
}

void AvatarBookmarks::readFromFile() {
    // migrate old avatarbookmarks.json, used to be in 'local' folder on windows
    QString oldConfigPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/" + AVATARBOOKMARKS_FILENAME;
    QFile oldConfig(oldConfigPath);

    // I imagine that in a year from now, this code for migrating (as well as the two lines above)
    // may be removed since all bookmarks should have been migrated by then
    // - Robbie Uvanni (6.8.2017)
    if (oldConfig.exists()) {
        if (QDir().rename(oldConfigPath, _bookmarksFilename)) {
            qCDebug(interfaceapp) << "Successfully migrated" << AVATARBOOKMARKS_FILENAME;
        } else {
            qCDebug(interfaceapp) << "Failed to migrate" << AVATARBOOKMARKS_FILENAME;
        }
    }

    Bookmarks::readFromFile();
}

void AvatarBookmarks::setupMenus(Menu* menubar, MenuWrapper* menu) {
    // Add menus/actions
    auto bookmarkAction = menubar->addActionToQMenuAndActionHash(menu, MenuOption::BookmarkAvatar);
    QObject::connect(bookmarkAction, SIGNAL(triggered()), this, SLOT(addBookmark()), Qt::QueuedConnection);
    _bookmarksMenu = menu->addMenu(MenuOption::AvatarBookmarks);
    _deleteBookmarksAction = menubar->addActionToQMenuAndActionHash(menu, MenuOption::DeleteAvatarBookmark);
    QObject::connect(_deleteBookmarksAction, SIGNAL(triggered()), this, SLOT(deleteBookmark()), Qt::QueuedConnection);

    for (auto it = _bookmarks.begin(); it != _bookmarks.end(); ++it) {
        addBookmarkToMenu(menubar, it.key(), it.value());
    }

    Bookmarks::sortActions(menubar, _bookmarksMenu);
}

void AvatarBookmarks::changeToBookmarkedAvatar() {
    QAction* action = qobject_cast<QAction*>(sender());
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();



    if (action->data().type() == QVariant::String) {
        // TODO: Phase this out eventually.
        // Legacy avatar bookmark.

        myAvatar->useFullAvatarURL(action->data().toString());
        qCDebug(interfaceapp) << " Using Legacy V1 Avatar Bookmark ";
    } else {

        const QMap<QString, QVariant> bookmark = action->data().toMap();
        // Not magic value. This is the current made version, and if it changes this interpreter should be updated to
        // handle the new one separately.
        // This is where the avatar bookmark entry is parsed. If adding new Value, make sure to have backward compatability with previous
        if (bookmark.value(ENTRY_VERSION) == 3) {
            myAvatar->removeAvatarEntities();
            const QString& avatarUrl = bookmark.value(ENTRY_AVATAR_URL, "").toString();
            myAvatar->useFullAvatarURL(avatarUrl);
            qCDebug(interfaceapp) << "Avatar On " << avatarUrl;
            const QList<QVariant>& attachments = bookmark.value(ENTRY_AVATAR_ATTACHMENTS, QList<QVariant>()).toList();

            qCDebug(interfaceapp) << "Attach " << attachments;
            myAvatar->setAttachmentsVariant(attachments);

            const float& qScale = bookmark.value(ENTRY_AVATAR_SCALE, 1.0f).toFloat();
            myAvatar->setAvatarScale(qScale);

            const QVariantList& avatarEntities = bookmark.value(ENTRY_AVATAR_ENTITIES, QVariantList()).toList();
            addAvatarEntities(avatarEntities);

        } else {
            qCDebug(interfaceapp) << " Bookmark entry does not match client version, make sure client has a handler for the new AvatarBookmark";
        }
    }

}

void AvatarBookmarks::addBookmark() {
    ModalDialogListener* dlg = OffscreenUi::getTextAsync(OffscreenUi::ICON_PLACEMARK, "Bookmark Avatar", "Name", QString());
    connect(dlg, &ModalDialogListener::response, this, [=] (QVariant response) {
        disconnect(dlg, &ModalDialogListener::response, this, nullptr);
        auto bookmarkName = response.toString();
        bookmarkName = bookmarkName.trimmed().replace(QRegExp("(\r\n|[\r\n\t\v ])+"), " ");
        if (bookmarkName.length() == 0) {
            return;
        }

        auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();

        const QString& avatarUrl = myAvatar->getSkeletonModelURL().toString();
        const QVariant& avatarScale = myAvatar->getAvatarScale();

        // If Avatar attachments ever change, this is where to update them, when saving remember to also append to AVATAR_BOOKMARK_VERSION
        QVariantMap bookmark;
        bookmark.insert(ENTRY_VERSION, AVATAR_BOOKMARK_VERSION);
        bookmark.insert(ENTRY_AVATAR_URL, avatarUrl);
        bookmark.insert(ENTRY_AVATAR_SCALE, avatarScale);
        bookmark.insert(ENTRY_AVATAR_ATTACHMENTS, myAvatar->getAttachmentsVariant());
        bookmark.insert(ENTRY_AVATAR_ENTITIES, myAvatar->getAvatarEntitiesVariant());

        Bookmarks::addBookmarkToFile(bookmarkName, bookmark);
    });

}

void AvatarBookmarks::addBookmarkToMenu(Menu* menubar, const QString& name, const QVariant& bookmark) {
    QAction* changeAction = _bookmarksMenu->newAction();
    changeAction->setData(bookmark);
    connect(changeAction, SIGNAL(triggered()), this, SLOT(changeToBookmarkedAvatar()));
    if (!_isMenuSorted) {
        menubar->addActionToQMenuAndActionHash(_bookmarksMenu, changeAction, name, 0, QAction::NoRole);
    } else {
        // TODO: this is aggressive but other alternatives have proved less fruitful so far.
        menubar->addActionToQMenuAndActionHash(_bookmarksMenu, changeAction, name, 0, QAction::NoRole);
        Bookmarks::sortActions(menubar, _bookmarksMenu);
    }
}
