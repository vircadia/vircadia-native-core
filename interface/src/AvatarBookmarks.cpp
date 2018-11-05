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

#include "AvatarBookmarks.h"

#include <QAction>
#include <QInputDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QQmlContext>
#include <QList>
#include <QtCore/QThread>

#include <shared/QtHelpers.h>
#include <Application.h>
#include <OffscreenUi.h>
#include <avatar/AvatarManager.h>
#include <EntityItemID.h>
#include <EntityTree.h>
#include <ModelEntityItem.h>
#include <PhysicalEntitySimulation.h>
#include <EntityEditPacketSender.h>
#include <VariantMapToScriptValue.h>

#include "MainWindow.h"
#include "InterfaceLogging.h"

#include "QVariantGLM.h"

#include <QtQuick/QQuickWindow>
#include <memory>

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
        entityTree->withWriteLock([&entityTree, id, &entityProperties, &success] {
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
                if (entityProperties.getDynamic()) {
                    // since we're creating a dynamic object we volunteer immediately to own its simulation
                    entity->upgradeScriptSimulationPriority(VOLUNTEER_SIMULATION_PRIORITY);
                }
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
    QDir directory(PathUtils::getAppDataPath());
    if (!directory.exists()) {
        directory.mkpath(".");
    }

    _bookmarksFilename = PathUtils::getAppDataPath() + "/" + AVATARBOOKMARKS_FILENAME;
    if(!QFile::exists(_bookmarksFilename)) {
        auto defaultBookmarksFilename = PathUtils::resourcesPath() + QString("avatar/bookmarks") + "/" + AVATARBOOKMARKS_FILENAME;
        if (!QFile::exists(defaultBookmarksFilename)) {
            qDebug() << defaultBookmarksFilename << "doesn't exist!";
        }

        if (!QFile::copy(defaultBookmarksFilename, _bookmarksFilename)) {
            qDebug() << "failed to copy" << defaultBookmarksFilename << "to" << _bookmarksFilename;
        } else {
            QFile bookmarksFile(_bookmarksFilename);
            bookmarksFile.setPermissions(bookmarksFile.permissions() | QFile::WriteUser);
        }
    }
    readFromFile();
}

void AvatarBookmarks::addBookmark(const QString& bookmarkName) {
    if (QThread::currentThread() != thread()) {
        BLOCKING_INVOKE_METHOD(this, "addBookmark", Q_ARG(QString, bookmarkName));
        return;
    }
    QVariantMap bookmark = getAvatarDataToBookmark();
    insert(bookmarkName, bookmark);

    emit bookmarkAdded(bookmarkName);
}

void AvatarBookmarks::saveBookmark(const QString& bookmarkName) {
    if (QThread::currentThread() != thread()) {
        BLOCKING_INVOKE_METHOD(this, "saveBookmark", Q_ARG(QString, bookmarkName));
        return;
    }
    if (contains(bookmarkName)) {
        QVariantMap bookmark = getAvatarDataToBookmark();
        insert(bookmarkName, bookmark);
    }
}

void AvatarBookmarks::removeBookmark(const QString& bookmarkName) {
    if (QThread::currentThread() != thread()) {
        BLOCKING_INVOKE_METHOD(this, "removeBookmark", Q_ARG(QString, bookmarkName));
        return;
    }

    remove(bookmarkName);
    emit bookmarkDeleted(bookmarkName);
}

void AvatarBookmarks::updateAvatarEntities(const QVariantList &avatarEntities) {
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    myAvatar->removeWearableAvatarEntities();
    addAvatarEntities(avatarEntities);
}

void AvatarBookmarks::loadBookmark(const QString& bookmarkName) {
    if (QThread::currentThread() != thread()) {
        BLOCKING_INVOKE_METHOD(this, "loadBookmark", Q_ARG(QString, bookmarkName));
        return;
    }

    auto bookmarkEntry = _bookmarks.find(bookmarkName);

    if (bookmarkEntry != _bookmarks.end()) {
        QVariantMap bookmark = bookmarkEntry.value().toMap();
        if (bookmark.empty()) { // compatibility with bookmarks like this: "Wooden Doll": "http://mpassets.highfidelity.com/7fe80a1e-f445-4800-9e89-40e677b03bee-v1/mannequin.fst?noDownload=false",
            auto avatarUrl = bookmarkEntry.value().toString();
            if (!avatarUrl.isEmpty()) {
                bookmark.insert(ENTRY_AVATAR_URL, avatarUrl);
            }
        }

        if (!bookmark.empty()) {
            auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
            auto treeRenderer = DependencyManager::get<EntityTreeRenderer>();
            EntityTreePointer entityTree = treeRenderer ? treeRenderer->getTree() : nullptr;
            myAvatar->removeWearableAvatarEntities();
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

            emit bookmarkLoaded(bookmarkName);
        }
    }
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

QVariantMap AvatarBookmarks::getBookmark(const QString &bookmarkName)
{
    if (QThread::currentThread() != thread()) {
        QVariantMap result;
        BLOCKING_INVOKE_METHOD(this, "getBookmark", Q_RETURN_ARG(QVariantMap, result), Q_ARG(QString, bookmarkName));
        return result;
    }

    QVariantMap bookmark;

    auto bookmarkEntry = _bookmarks.find(bookmarkName);

    if (bookmarkEntry != _bookmarks.end()) {
        bookmark = bookmarkEntry.value().toMap();
    }

    return bookmark;
}

QVariantMap AvatarBookmarks::getAvatarDataToBookmark() {
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();

    const QString& avatarUrl = myAvatar->getSkeletonModelURL().toString();
    const QVariant& avatarScale = myAvatar->getAvatarScale();

    // If Avatar attachments ever change, this is where to update them, when saving remember to also append to AVATAR_BOOKMARK_VERSION
    QVariantMap bookmark;
    bookmark.insert(ENTRY_VERSION, AVATAR_BOOKMARK_VERSION);
    bookmark.insert(ENTRY_AVATAR_URL, avatarUrl);
    bookmark.insert(ENTRY_AVATAR_SCALE, avatarScale);

    QScriptEngine scriptEngine;
    QVariantList wearableEntities;
    auto treeRenderer = DependencyManager::get<EntityTreeRenderer>();
    EntityTreePointer entityTree = treeRenderer ? treeRenderer->getTree() : nullptr;
    auto avatarEntities = myAvatar->getAvatarEntityData();
    for (auto entityID : avatarEntities.keys()) {
        auto entity = entityTree->findEntityByID(entityID);
        if (!entity || !isWearableEntity(entity)) {
            continue;
        }
        QVariantMap avatarEntityData;
        EncodeBitstreamParams params;
        auto desiredProperties = entity->getEntityProperties(params);
        desiredProperties += PROP_LOCAL_POSITION;
        desiredProperties += PROP_LOCAL_ROTATION;
        EntityItemProperties entityProperties = entity->getProperties(desiredProperties);
        QScriptValue scriptProperties = EntityItemPropertiesToScriptValue(&scriptEngine, entityProperties);
        avatarEntityData["properties"] = scriptProperties.toVariant();
        wearableEntities.append(QVariant(avatarEntityData));
    }
    bookmark.insert(ENTRY_AVATAR_ENTITIES, wearableEntities);
    return bookmark;
}
