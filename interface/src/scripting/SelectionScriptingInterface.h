
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
    std::vector<QUuid> _avatarIDs;
    std::vector<EntityItemID> _entityIDs;
    std::vector<OverlayID> _overlayIDs;
};

class SelectionScriptingInterface : public QObject, public Dependency {
    Q_OBJECT

public:
    SelectionScriptingInterface(AbstractViewStateInterface* viewState);

    Q_INVOKABLE bool removeListFromMap(const render::Selection::Name& listName);

    Q_INVOKABLE bool addToSelectedItemsList(const render::Selection::Name& listName, const QUuid& avatarSessionID);
    Q_INVOKABLE bool removeFromSelectedItemsList(const render::Selection::Name& listName, const QUuid& avatarSessionID);

    Q_INVOKABLE bool addToSelectedItemsList(const render::Selection::Name& listName, const EntityItemID& entityID);
    Q_INVOKABLE bool removeFromSelectedItemsList(const render::Selection::Name& listName, const EntityItemID& entityID);
    
    Q_INVOKABLE bool addToSelectedItemsList(const render::Selection::Name& listName, const OverlayID& overlayID);
    Q_INVOKABLE bool removeFromSelectedItemsList(const render::Selection::Name& listName, const OverlayID& overlayID);

private:
    AbstractViewStateInterface* _viewState;
    QMap<render::Selection::Name, GameplayObjects> _selectedItemsListMap;

    template <class T> bool addToGameplayObjects(const render::Selection::Name& listName, T idToAdd);
    template <class T> bool removeFromGameplayObjects(const render::Selection::Name& listName, T idToRemove);

    void updateRendererSelectedList(const render::Selection::Name& listName);
};

#endif // hifi_SelectionScriptingInterface_h
