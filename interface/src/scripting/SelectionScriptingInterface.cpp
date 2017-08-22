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

SelectionScriptingInterface::SelectionScriptingInterface(AbstractViewStateInterface* viewState) {
    _viewState = viewState;
}

//
// START HANDLING ENTITIES
//
render::ItemID getItemIDFromEntityID(const EntityItemID& entityID) {
    auto entityTree = qApp->getEntities()->getTree();
    entityTree->withReadLock([&] {
        auto entityItem = entityTree->findEntityByEntityItemID(entityID);
        if (entityItem != NULL) {
            auto renderableInterface = entityItem->getRenderableInterface();
            if (renderableInterface != NULL) {
                return renderableInterface->getMetaRenderItemID();
            }
        }
        return render::Item::INVALID_ITEM_ID;
    });
    return render::Item::INVALID_ITEM_ID;
}
bool SelectionScriptingInterface::addToSelectedItemsList(const render::Selection::Name& listName, const EntityItemID& entityID) {
    render::ItemID itemID = getItemIDFromEntityID(entityID);
    if (itemID != render::Item::INVALID_ITEM_ID) {
        return addToSelectedItemsList(listName, itemID);
    }
    return false;
}
bool SelectionScriptingInterface::removeFromSelectedItemsList(const render::Selection::Name& listName, const EntityItemID& entityID) {
    render::ItemID itemID = getItemIDFromEntityID(entityID);
    if (itemID != render::Item::INVALID_ITEM_ID) {
        return removeFromSelectedItemsList(listName, itemID);
    }
    return false;
}
//
// END HANDLING ENTITIES
//

//
// START HANDLING OVERLAYS
//
render::ItemID getItemIDFromOverlayID(const OverlayID& overlayID) {
    auto& overlays = qApp->getOverlays();
    auto overlay = overlays.getOverlay(overlayID);
    if (overlay != NULL) {
        auto itemID = overlay->getRenderItemID();
        if (itemID != render::Item::INVALID_ITEM_ID) {
            return overlay->getRenderItemID();
        }
    }
    return render::Item::INVALID_ITEM_ID;
}
bool SelectionScriptingInterface::addToSelectedItemsList(const render::Selection::Name& listName, const OverlayID& overlayID) {
    render::ItemID itemID = getItemIDFromOverlayID(overlayID);
    if (itemID != render::Item::INVALID_ITEM_ID) {
        return addToSelectedItemsList(listName, itemID);
    }
    return false;
}
bool SelectionScriptingInterface::removeFromSelectedItemsList(const render::Selection::Name& listName, const OverlayID& overlayID) {
    render::ItemID itemID = getItemIDFromOverlayID(overlayID);
    if (itemID != render::Item::INVALID_ITEM_ID) {
        return removeFromSelectedItemsList(listName, itemID);
    }
    return false;
}
//
// END HANDLING OVERLAYS
//

//
// START HANDLING GENERIC ITEMS
//
bool SelectionScriptingInterface::addToSelectedItemsList(const render::Selection::Name& listName, render::ItemID idToAdd) {
    if (_selectedItemsListMap.contains(listName)) {
        auto currentList = _selectedItemsListMap.take(listName);
        currentList.push_back(idToAdd); // TODO: Ensure thread safety
        _selectedItemsListMap.insert(listName, currentList);
        updateRendererSelectedList(listName);
        return true;
    } else {
        _selectedItemsListMap.insert(listName, render::ItemIDs());
        return addToSelectedItemsList(listName, idToAdd);
    }
}
bool SelectionScriptingInterface::removeFromSelectedItemsList(const render::Selection::Name& listName, render::ItemID idToRemove) {
    auto hashItr = _selectedItemsListMap.find(listName);
    if (hashItr != _selectedItemsListMap.end()) {
        auto currentList = _selectedItemsListMap.take(listName);
        auto listItr = std::find(currentList.begin(), currentList.end(), idToRemove); // TODO: Ensure thread safety
        if (listItr == currentList.end()) {
            return false;
        } else {
            currentList.erase(listItr);
            _selectedItemsListMap.insert(listName, currentList);
            updateRendererSelectedList(listName);
            return true;
        }
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
            render::Selection selection(listName, _selectedItemsListMap.value(listName));
            transaction.resetSelection(selection);

            scene->enqueueTransaction(transaction);
        } else {
            qWarning() << "List of ItemIDs doesn't exist in _selectedItemsListMap";
        }
    } else {
        qWarning() << "SelectionScriptingInterface::updateRendererSelectedList(), Unexpected null scene, possibly during application shutdown";
    }
}
