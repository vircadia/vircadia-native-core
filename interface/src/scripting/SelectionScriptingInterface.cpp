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
#include <QDebug>
#include "Application.h"

GameplayObjects::GameplayObjects() {
}

bool GameplayObjects::addToGameplayObjects(const QUuid& avatarID) {
    containsData = true;
    _avatarIDs.push_back(avatarID);
    return true;
}
bool GameplayObjects::removeFromGameplayObjects(const QUuid& avatarID) {
    _avatarIDs.erase(std::remove(_avatarIDs.begin(), _avatarIDs.end(), avatarID), _avatarIDs.end());
    return true;
}

bool GameplayObjects::addToGameplayObjects(const EntityItemID& entityID) {
    containsData = true;
    _entityIDs.push_back(entityID);
    return true;
}
bool GameplayObjects::removeFromGameplayObjects(const EntityItemID& entityID) {
    _entityIDs.erase(std::remove(_entityIDs.begin(), _entityIDs.end(), entityID), _entityIDs.end());
    return true;
}

bool GameplayObjects::addToGameplayObjects(const OverlayID& overlayID) {
    containsData = true;
    _overlayIDs.push_back(overlayID);
    return true;
}
bool GameplayObjects::removeFromGameplayObjects(const OverlayID& overlayID) {
    _overlayIDs.erase(std::remove(_overlayIDs.begin(), _overlayIDs.end(), overlayID), _overlayIDs.end());
    return true;
}


SelectionScriptingInterface::SelectionScriptingInterface() {
}

bool SelectionScriptingInterface::addToSelectedItemsList(const QString& listName, const QString& itemType, const QUuid& id) {
    if (itemType == "avatar") {
        return addToGameplayObjects(listName, (QUuid)id);
    } else if (itemType == "entity") {
        return addToGameplayObjects(listName, (EntityItemID)id);
    } else if (itemType == "overlay") {
        return addToGameplayObjects(listName, (OverlayID)id);
    }
    return false;
}
bool SelectionScriptingInterface::removeFromSelectedItemsList(const QString& listName, const QString& itemType, const QUuid& id) {
    if (itemType == "avatar") {
        return removeFromGameplayObjects(listName, (QUuid)id);
    } else if (itemType == "entity") {
        return removeFromGameplayObjects(listName, (EntityItemID)id);
    } else if (itemType == "overlay") {
        return removeFromGameplayObjects(listName, (OverlayID)id);
    }
    return false;
}

bool SelectionScriptingInterface::clearSelectedItemsList(const QString& listName) {
    _selectedItemsListMap.insert(listName, GameplayObjects());
    emit selectedItemsListChanged(listName);
    return true;
}

bool SelectionScriptingInterface::enableListHighlight(const QString& listName, const QVariantMap& highlightStyleValues) {
    bool doSetupHandler = false;

    auto highlightStyle = _highlightedListMap.find(listName);
    if (highlightStyle == _highlightedListMap.end()) {
        highlightStyle = _highlightedListMap.insert(listName, SelectionHighlightStyle());
        doSetupHandler = true;
    }

    if (!(*highlightStyle).isBoundToList()) {
        GameplayObjects currentList = _selectedItemsListMap.value(listName);
        if (!currentList.getContainsData()) {
            _selectedItemsListMap.insert(listName, currentList);
            doSetupHandler = true;
        }
        (*highlightStyle).setBoundToList(true);
    }

    (*highlightStyle).fromVariantMap(highlightStyleValues);

    if (doSetupHandler) {
        setupHandler(listName);
    }

    emit highlightStyleChanged(listName);
    return true;
}

bool SelectionScriptingInterface::disableListHighlight(const QString& listName) {
    auto highlightStyle = _highlightedListMap.find(listName);
    if (highlightStyle != _highlightedListMap.end()) {
        if ((*highlightStyle).isBoundToList()) {
            GameplayObjects currentList = _selectedItemsListMap.value(listName);
            if (currentList.getContainsData()) {
            }
            _highlightedListMap.erase(highlightStyle);
            emit selectedItemsListChanged(listName);
        }
    }

    return true;
}

QVariantMap SelectionScriptingInterface::getListHighlightStyle(const QString& listName) const {
    return QVariantMap();
}

render::HighlightStyle SelectionScriptingInterface::getHighlightStyle(const QString& listName) const {
    auto highlightStyle = _highlightedListMap.find(listName);
    if (highlightStyle != _highlightedListMap.end()) {
        return render::HighlightStyle();
    } else {
        return (*highlightStyle).getStyle();
    }
}

template <class T> bool SelectionScriptingInterface::addToGameplayObjects(const QString& listName, T idToAdd) {
    GameplayObjects currentList = _selectedItemsListMap.value(listName);
    currentList.addToGameplayObjects(idToAdd);
    _selectedItemsListMap.insert(listName, currentList);

    emit selectedItemsListChanged(listName);
    return true;
}
template <class T> bool SelectionScriptingInterface::removeFromGameplayObjects(const QString& listName, T idToRemove) {
    GameplayObjects currentList = _selectedItemsListMap.value(listName);
    if (currentList.getContainsData()) {
        currentList.removeFromGameplayObjects(idToRemove);
        _selectedItemsListMap.insert(listName, currentList);

        emit selectedItemsListChanged(listName);
        return true;
    } else {
        return false;
    }
}
//
// END HANDLING GENERIC ITEMS
//

GameplayObjects SelectionScriptingInterface::getList(const QString& listName) {
    return _selectedItemsListMap.value(listName);
}

void SelectionScriptingInterface::printList(const QString& listName) {
    GameplayObjects currentList = _selectedItemsListMap.value(listName);
    if (currentList.getContainsData()) {

        qDebug() << "Avatar IDs:";
        for (auto i : currentList.getAvatarIDs()) {
            qDebug() << i << ';';
        }
        qDebug() << "";

        qDebug() << "Entity IDs:";
        for (auto j : currentList.getEntityIDs()) {
            qDebug() << j << ';';
        }
        qDebug() << "";

        qDebug() << "Overlay IDs:";
        for (auto k : currentList.getOverlayIDs()) {
            qDebug() << k << ';';
        }
        qDebug() << "";
    } else {
        qDebug() << "List named" << listName << "doesn't exist.";
    }
}

bool SelectionScriptingInterface::removeListFromMap(const QString& listName) {
    if (_selectedItemsListMap.remove(listName)) {
        emit selectedItemsListChanged(listName);
        return true;
    } else {
        return false;
    }
}

void SelectionScriptingInterface::onSelectedItemsListChanged(const QString& listName) {
    emit selectedItemsListChanged(listName);
}

void SelectionScriptingInterface::onHighlightStyleChanged(const QString& listName) {
    emit onHighlightStyleChanged(listName);
}

void SelectionScriptingInterface::setupHandler(const QString& selectionName) {
    auto handler = _handlerMap.find(selectionName);
    if (handler == _handlerMap.end()) {
        handler = _handlerMap.insert(selectionName, new SelectionToSceneHandler());
    }


    (*handler)->initialize(selectionName);

}


SelectionToSceneHandler::SelectionToSceneHandler() {
}

void SelectionToSceneHandler::initialize(const QString& listName) {
    _listName = listName;

    connect(&(*DependencyManager::get<SelectionScriptingInterface>()), &SelectionScriptingInterface::selectedItemsListChanged, this, &SelectionToSceneHandler::selectedItemsListChanged);
    connect(&(*DependencyManager::get<SelectionScriptingInterface>()), &SelectionScriptingInterface::highlightStyleChanged, this, &SelectionToSceneHandler::highlightStyleChanged);

    auto mainScene = qApp->getMain3DScene();
    if (mainScene) {
        render::Transaction transaction;
        transaction.resetSelectionHighlight(listName.toStdString(), DependencyManager::get<SelectionScriptingInterface>()->getHighlightStyle(listName));
        mainScene->enqueueTransaction(transaction);
    }
}

void SelectionToSceneHandler::selectedItemsListChanged(const QString& listName) {
    if (listName == _listName) {
        updateSceneFromSelectedList();
    }
}

void SelectionToSceneHandler::updateSceneFromSelectedList() {
    auto mainScene = qApp->getMain3DScene();
    if (mainScene) {
        GameplayObjects thisList = DependencyManager::get<SelectionScriptingInterface>()->getList(_listName);
        render::Transaction transaction;
        render::ItemIDs finalList;
        render::ItemID currentID;
        auto entityTreeRenderer = DependencyManager::get<EntityTreeRenderer>();
        auto& overlays = qApp->getOverlays();

        for (QUuid& currentAvatarID : thisList.getAvatarIDs()) {
            auto avatar = std::static_pointer_cast<Avatar>(DependencyManager::get<AvatarManager>()->getAvatarBySessionID(currentAvatarID));
            if (avatar) {
                currentID = avatar->getRenderItemID();
                if (currentID != render::Item::INVALID_ITEM_ID) {
                    finalList.push_back(currentID);
                }
            }
        }

        for (EntityItemID& currentEntityID : thisList.getEntityIDs()) {
            currentID = entityTreeRenderer->renderableIdForEntityId(currentEntityID);
            if (currentID != render::Item::INVALID_ITEM_ID) {
                finalList.push_back(currentID);
            }
        }

        for (OverlayID& currentOverlayID : thisList.getOverlayIDs()) {
            auto overlay = overlays.getOverlay(currentOverlayID);
            if (overlay != NULL) {
                currentID = overlay->getRenderItemID();
                if (currentID != render::Item::INVALID_ITEM_ID) {
                    finalList.push_back(currentID);
                }
            }
        }

        render::Selection selection(_listName.toStdString(), finalList);
        transaction.resetSelection(selection);

        mainScene->enqueueTransaction(transaction);
    } else {
        qWarning() << "SelectionToSceneHandler::updateRendererSelectedList(), Unexpected null scene, possibly during application shutdown";
    }
}

void SelectionToSceneHandler::highlightStyleChanged(const QString& listName) {
    if (listName == _listName) {
        auto mainScene = qApp->getMain3DScene();
        if (mainScene) {
            auto thisStyle = DependencyManager::get<SelectionScriptingInterface>()->getHighlightStyle(listName);
            render::Transaction transaction;
            render::ItemIDs finalList;

            transaction.resetSelectionHighlight(listName.toStdString(), thisStyle);

            mainScene->enqueueTransaction(transaction);
        }
        else {
            qWarning() << "SelectionToSceneHandler::updateRendererSelectedList(), Unexpected null scene, possibly during application shutdown";
        }
    }
}

bool SelectionHighlightStyle::fromVariantMap(const QVariantMap& properties) {
    auto outlineColor = properties["outlineColor"];
    if (outlineColor.isValid()) {
        bool isValid;
        auto color = xColorFromVariant(properties["outlineColor"], isValid);
        if (isValid) {
            _style.color = toGlm(color);
        }
    }
    auto outlineWidth = properties["outlineWidth"];
    if (outlineWidth.isValid()) {
        _style.outlineWidth = outlineWidth.toFloat();
    }
    auto isOutlineSmooth = properties["isOutlineSmooth"];
    if (isOutlineSmooth.isValid()) {
        _style.isOutlineSmooth = isOutlineSmooth.toBool();
    }

    auto outlineIntensity = properties["outlineIntensity"];
    if (outlineIntensity.isValid()) {
        _style.outlineIntensity = outlineIntensity.toFloat();
    }

    auto unoccludedFillOpacity = properties["unoccludedFillOpacity"];
    if (unoccludedFillOpacity.isValid()) {
        _style.unoccludedFillOpacity = unoccludedFillOpacity.toFloat();
    }
    auto occludedFillOpacity = properties["occludedFillOpacity"];
    if (occludedFillOpacity.isValid()) {
        _style.occludedFillOpacity = occludedFillOpacity.toFloat();
    }

    return true;
}

QVariantMap SelectionHighlightStyle::toVariantMap() const {
    QVariantMap properties;

    properties["outlineColor"] = xColorToVariant(xColorFromGlm(_style.color));
    properties["outlineWidth"] = _style.outlineWidth;
    properties["isOutlineSmooth"] = _style.isOutlineSmooth;
    properties["outlineIntensity"] = _style.outlineIntensity;
    properties["unoccludedFillOpacity"] = _style.unoccludedFillOpacity;
    properties["occludedFillOpacity"] = _style.occludedFillOpacity;

    return properties;
}