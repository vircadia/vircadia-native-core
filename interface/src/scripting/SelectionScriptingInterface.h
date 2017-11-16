
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
#include <render/HighlightStyle.h>

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


class SelectionHighlightStyle {
public:
    SelectionHighlightStyle() {}

    void setBoundToList(bool bound) { _isBoundToList = bound; }
    bool isBoundToList() const { return _isBoundToList; }

    bool fromVariantMap(const QVariantMap& properties);
    QVariantMap toVariantMap() const;

    render::HighlightStyle getStyle() const { return _style; }

protected:
    bool _isBoundToList{ false };
    render::HighlightStyle _style;
};

class SelectionScriptingInterface : public QObject, public Dependency {
    Q_OBJECT

public:
    SelectionScriptingInterface();

    GameplayObjects getList(const QString& listName);

    /**jsdoc
    * Prints out the list of avatars, entities and overlays stored in a particular selection.
    * @function Selection.printList
    * @param listName {string} name of the selection
    */
    Q_INVOKABLE void printList(const QString& listName);
    /**jsdoc
    * Removes a named selection from the list of selections.
    * @function Selection.removeListFromMap
    * @param listName {string} name of the selection
    * @returns {bool} true if the selection existed and was successfully removed.
    */
    Q_INVOKABLE bool removeListFromMap(const QString& listName);

    /**jsdoc
    * Add an item in a selection.
    * @function Selection.addToSelectedItemsList
    * @param listName {string} name of the selection
    * @param itemType {string} the type of the item (one of "avatar", "entity" or "overlay")
    * @param id {EntityID} the Id of the item to add to the selection
    * @returns {bool} true if the item was successfully added.
    */
    Q_INVOKABLE bool addToSelectedItemsList(const QString& listName, const QString& itemType, const QUuid& id);
    /**jsdoc
    * Remove an item from a selection.
    * @function Selection.removeFromSelectedItemsList
    * @param listName {string} name of the selection
    * @param itemType {string} the type of the item (one of "avatar", "entity" or "overlay")
    * @param id {EntityID} the Id of the item to remove
    * @returns {bool} true if the item was successfully removed.
    */
    Q_INVOKABLE bool removeFromSelectedItemsList(const QString& listName, const QString& itemType, const QUuid& id);
    /**jsdoc
    * Remove all items from a selection.
    * @function Selection.clearSelectedItemsList
    * @param listName {string} name of the selection
    * @returns {bool} true if the item was successfully cleared.
    */
    Q_INVOKABLE bool clearSelectedItemsList(const QString& listName);

    Q_INVOKABLE bool enableListHighlight(const QString& listName, const QVariantMap& highlightStyle);
    Q_INVOKABLE bool disableListHighlight(const QString& listName);
    Q_INVOKABLE QVariantMap getListHighlightStyle(const QString& listName) const;

    render::HighlightStyle getHighlightStyle(const QString& listName) const;

signals:
    void selectedItemsListChanged(const QString& listName);
    void highlightStyleChanged(const QString& listName);

private:
    QMap<QString, GameplayObjects> _selectedItemsListMap;

    QMap<QString, SelectionHighlightStyle> _highlightedListMap;

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
    void highlightStyleChanged(const QString& listName);

private:
    QString _listName { "" };
};

#endif // hifi_SelectionScriptingInterface_h
