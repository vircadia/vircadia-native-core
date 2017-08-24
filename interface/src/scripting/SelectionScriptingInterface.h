
//  SelectionScriptingInterface.h
//  interface/src/scripting
//
//  Created by Zach Fox on 2017-08-22.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SelectionScriptingInterface_h
#define hifi_SelectionScriptingInterface_h

#include <QtCore/QObject>
#include <QtCore/QMap>
#include <DependencyManager.h>

#include <AbstractViewStateInterface.h>

#include "RenderableEntityItem.h"
#include "ui/overlays/Overlay.h"
#include <avatar/AvatarManager.h>

class GameplayObjects {
public:
    GameplayObjects();

    bool getContainsData() { return containsData; }

    std::vector<QUuid> getAvatarIDs() { return _avatarIDs; }
    bool addToGameplayObjects(const QUuid& avatarID);
    bool removeFromGameplayObjects(const QUuid& avatarID);

    std::vector<EntityItemID> getEntityIDs() { return _entityIDs; }
    bool addToGameplayObjects(const EntityItemID& entityID);
    bool removeFromGameplayObjects(const EntityItemID& entityID);

    std::vector<OverlayID> getOverlayIDs() { return _overlayIDs; }
    bool addToGameplayObjects(const OverlayID& overlayID);
    bool removeFromGameplayObjects(const OverlayID& overlayID);

private:
    bool containsData { false };
    std::vector<QUuid> _avatarIDs;
    std::vector<EntityItemID> _entityIDs;
    std::vector<OverlayID> _overlayIDs;
};


class SelectionScriptingInterface : public QObject, public Dependency {
    Q_OBJECT

public:
    SelectionScriptingInterface();

    GameplayObjects getList(const QString& listName);

    Q_INVOKABLE void printList(const QString& listName);
    Q_INVOKABLE bool removeListFromMap(const QString& listName);

    Q_INVOKABLE bool addToSelectedItemsList(const QString& listName, const QString& itemType, const QUuid& id);
    Q_INVOKABLE bool removeFromSelectedItemsList(const QString& listName, const QString& itemType, const QUuid& id);

signals:
    void selectedItemsListChanged(const QString& listName);

private:
    QMap<QString, GameplayObjects> _selectedItemsListMap;

    template <class T> bool addToGameplayObjects(const QString& listName, T idToAdd);
    template <class T> bool removeFromGameplayObjects(const QString& listName, T idToRemove);
};


class SelectionToSceneHandler : public QObject {
    Q_OBJECT
public:
    SelectionToSceneHandler();
    void initialize(const QString& listName);

    void updateSceneFromSelectedList();

public slots:
    void selectedItemsListChanged(const QString& listName);

private:
    QString _listName { "" };
};

#endif // hifi_SelectionScriptingInterface_h
