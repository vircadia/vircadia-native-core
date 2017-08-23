//
//  SelectionScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Zach Fox on 2017-08-22.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SelectionScriptingInterface.h"
#include "Application.h"

GameplayObjects::GameplayObjects() {
}

bool GameplayObjects::addToGameplayObjects(const QUuid& avatarID) {
    _avatarIDs.push_back(avatarID);
    return true;
}
bool GameplayObjects::removeFromGameplayObjects(const QUuid& avatarID) {
    _avatarIDs.erase(std::remove(_avatarIDs.begin(), _avatarIDs.end(), avatarID), _avatarIDs.end());
    return true;
}

bool GameplayObjects::addToGameplayObjects(const EntityItemID& entityID) {
    _entityIDs.push_back(entityID);
    return true;
}
bool GameplayObjects::removeFromGameplayObjects(const EntityItemID& entityID) {
    _entityIDs.erase(std::remove(_entityIDs.begin(), _entityIDs.end(), entityID), _entityIDs.end());
    return true;
}

bool GameplayObjects::addToGameplayObjects(const OverlayID& overlayID) {
    _overlayIDs.push_back(overlayID);
    return true;
}
bool GameplayObjects::removeFromGameplayObjects(const OverlayID& overlayID) {
    _overlayIDs.erase(std::remove(_overlayIDs.begin(), _overlayIDs.end(), overlayID), _overlayIDs.end());
    return true;
}

SelectionScriptingInterface::SelectionScriptingInterface(AbstractViewStateInterface* viewState) {
    _viewState = viewState;
}

//
// START HANDLING AVATARS
//
bool SelectionScriptingInterface::addToSelectedItemsList(const render::Selection::Name& listName, const QUuid& avatarSessionID) {
    return addToGameplayObjects(listName, avatarSessionID);
}
bool SelectionScriptingInterface::removeFromSelectedItemsList(const render::Selection::Name& listName, const QUuid& avatarSessionID) {
    return addToGameplayObjects(listName, avatarSessionID);
}
//
// END HANDLING AVATARS
//

//
// START HANDLING ENTITIES
//
bool SelectionScriptingInterface::addToSelectedItemsList(const render::Selection::Name& listName, const EntityItemID& entityID) {
    return addToGameplayObjects(listName, entityID);
}
bool SelectionScriptingInterface::removeFromSelectedItemsList(const render::Selection::Name& listName, const EntityItemID& entityID) {
    return addToGameplayObjects(listName, entityID);
}
//
// END HANDLING ENTITIES
//

//
// START HANDLING OVERLAYS
//
bool SelectionScriptingInterface::addToSelectedItemsList(const render::Selection::Name& listName, const OverlayID& overlayID) {
    return addToGameplayObjects(listName, overlayID);
}
bool SelectionScriptingInterface::removeFromSelectedItemsList(const render::Selection::Name& listName, const OverlayID& overlayID) {
    return addToGameplayObjects(listName, overlayID);
}
//
// END HANDLING OVERLAYS
//

//
// START HANDLING GENERIC ITEMS
//
template <class T> bool SelectionScriptingInterface::addToGameplayObjects(const render::Selection::Name& listName, T idToAdd) {
    if (_selectedItemsListMap.contains(listName)) {
        auto currentList = _selectedItemsListMap.take(listName);
        currentList.addToGameplayObjects(idToAdd);
        _selectedItemsListMap.insert(listName, currentList);
        updateRendererSelectedList(listName);
        return true;
    } else {
        _selectedItemsListMap.insert(listName, GameplayObjects());
        return addToSelectedItemsList(listName, idToAdd);
    }
}
template <class T> bool SelectionScriptingInterface::removeFromGameplayObjects(const render::Selection::Name& listName, T idToRemove) {
    if (_selectedItemsListMap.contains(listName)) {
        auto currentList = _selectedItemsListMap.take(listName);
        currentList.removeFromGameplayObjects(idToRemove);
        _selectedItemsListMap.insert(listName, currentList);
        updateRendererSelectedList(listName);
        return true;
    } else {
        return false;
    }
}
//
// END HANDLING GENERIC ITEMS
//

bool SelectionScriptingInterface::removeListFromMap(const render::Selection::Name& listName) {
    if (_selectedItemsListMap.remove(listName)) {
        return true;
    } else {
        return false;
    }
}

void SelectionScriptingInterface::updateRendererSelectedList(const render::Selection::Name& listName) {
    auto scene = _viewState->getMain3DScene();
    if (scene) {
        render::Transaction transaction;

        if (_selectedItemsListMap.contains(listName)) {
            render::ItemIDs finalList;
            render::ItemID currentID;
            auto entityTree = qApp->getEntities()->getTree();
            auto& overlays = qApp->getOverlays();
            auto currentList = _selectedItemsListMap.value(listName);

            for (QUuid& currentAvatarID : currentList.getAvatarIDs()) {
                auto avatar = std::static_pointer_cast<Avatar>(DependencyManager::get<AvatarManager>()->getAvatarBySessionID(currentAvatarID));
                if (avatar) {
                    currentID = avatar->getRenderItemID();
                    if (currentID != render::Item::INVALID_ITEM_ID) {
                        finalList.push_back(currentID);
                    }
                }
            }

            for (EntityItemID& currentEntityID : currentList.getEntityIDs()) {
                entityTree->withReadLock([&] {
                    auto entityItem = entityTree->findEntityByEntityItemID(currentEntityID);
                    if (entityItem != NULL) {
                        auto renderableInterface = entityItem->getRenderableInterface();
                        if (renderableInterface != NULL) {
                            currentID = renderableInterface->getMetaRenderItemID();
                            if (currentID != render::Item::INVALID_ITEM_ID) {
                                finalList.push_back(currentID);
                            }
                        }
                    }
                });
            }

            for (OverlayID& currentOverlayID : currentList.getOverlayIDs()) {
                auto overlay = overlays.getOverlay(currentOverlayID);
                if (overlay != NULL) {
                    currentID = overlay->getRenderItemID();
                    if (currentID != render::Item::INVALID_ITEM_ID) {
                        finalList.push_back(currentID);
                    }
                }
            }

            render::Selection selection(listName, finalList);
            transaction.resetSelection(selection);

            scene->enqueueTransaction(transaction);
        } else {
            qWarning() << "List of GameplayObjects doesn't exist in _selectedItemsListMap";
        }
    } else {
        qWarning() << "SelectionScriptingInterface::updateRendererSelectedList(), Unexpected null scene, possibly during application shutdown";
    }
}
