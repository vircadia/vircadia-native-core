
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

    bool getContainsData() const { return containsData; }

    std::vector<QUuid> getAvatarIDs() const { return _avatarIDs; }
    bool addToGameplayObjects(const QUuid& avatarID);
    bool removeFromGameplayObjects(const QUuid& avatarID);

    std::vector<EntityItemID> getEntityIDs() const { return _entityIDs; }
    bool addToGameplayObjects(const EntityItemID& entityID);
    bool removeFromGameplayObjects(const EntityItemID& entityID);

    std::vector<OverlayID> getOverlayIDs() const { return _overlayIDs; }
    bool addToGameplayObjects(const OverlayID& overlayID);
    bool removeFromGameplayObjects(const OverlayID& overlayID);

private:
    bool containsData { false };
    std::vector<QUuid> _avatarIDs;
    std::vector<EntityItemID> _entityIDs;
    std::vector<OverlayID> _overlayIDs;
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
    QString _listName{ "" };
};
using SelectionToSceneHandlerPointer = QSharedPointer<SelectionToSceneHandler>;

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

    /**jsdoc
    * Query the names of all the selection lists
    * @function Selection.getListNames
    * @return An array of names of all the selection lists
    */
    Q_INVOKABLE QStringList getListNames() const;

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

    /**jsdoc
    * Prints out the list of avatars, entities and overlays stored in a particular selection.
    * @function Selection.printList
    * @param listName {string} name of the selection
    */
    Q_INVOKABLE void printList(const QString& listName);

    /**jsdoc
    * Query the list of avatars, entities and overlays stored in a particular selection.
    * @function Selection.getList
    * @param listName {string} name of the selection
    * @return a js object describing the content of a selection list with the following properties:
    *  - "entities": [ and array of the entityID of the entities in the selection]
    *  - "avatars": [ and array of the avatarID of the avatars in the selection]
    *  - "overlays": [ and array of the overlayID of the overlays in the selection]
    *  If the list name doesn't exist, the function returns an empty js object with no properties.
    */
    Q_INVOKABLE QVariantMap getSelectedItemsList(const QString& listName) const;

    /**jsdoc
    * Query the names of the highlighted selection lists
    * @function Selection.getHighlightedListNames
    * @return An array of names of the selection list currently highlight enabled
    */
    Q_INVOKABLE QStringList getHighlightedListNames() const;

    /**jsdoc
    * Enable highlighting for the named selection.
    * If the Selection doesn't exist, it will be created.
    * All objects in the list will be displayed with the highlight effect as specified from the highlightStyle.
    * The function can be called several times with different values in the style to modify it.
    * 
    * @function Selection.enableListHighlight
    * @param listName {string} name of the selection
    * @param highlightStyle {jsObject} highlight style fields (see Selection.getListHighlightStyle for a detailed description of the highlightStyle).
    * @returns {bool} true if the selection was successfully enabled for highlight.
    */
    Q_INVOKABLE bool enableListHighlight(const QString& listName, const QVariantMap& highlightStyle);
    /**jsdoc
    * Disable highlighting for the named selection.
    * If the Selection doesn't exist or wasn't enabled for highliting then nothing happens simply returning false.
    *
    * @function Selection.disableListHighlight
    * @param listName {string} name of the selection
    * @returns {bool} true if the selection was successfully disabled for highlight, false otherwise.
    */
    Q_INVOKABLE bool disableListHighlight(const QString& listName);
    /**jsdoc
    * Query the highlight style values for the named selection.
    * If the Selection doesn't exist or hasn't been highlight enabled yet, it will return an empty object.
    * Otherwise, the jsObject describes the highlight style properties:
    * - outlineUnoccludedColor: {xColor} Color of the specified highlight region
    * - outlineOccludedColor: {xColor} "
    * - fillUnoccludedColor: {xColor} "
    * - fillOccludedColor: {xColor} "
    *
    * - outlineUnoccludedAlpha: {float} Alpha value ranging from 0.0 (not visible) to 1.0 (fully opaque) for the specified highlight region
    * - outlineOccludedAlpha: {float} "
    * - fillUnoccludedAlpha: {float} "
    * - fillOccludedAlpha: {float} "
    *
    * - outlineWidth: {float} width of the outline expressed in pixels
    * - isOutlineSmooth: {bool} true to enable oultine smooth falloff
    *
    * @function Selection.getListHighlightStyle
    * @param listName {string} name of the selection
    * @returns {jsObject} highlight style as described above
    */
    Q_INVOKABLE QVariantMap getListHighlightStyle(const QString& listName) const;


    GameplayObjects getList(const QString& listName);

    render::HighlightStyle getHighlightStyle(const QString& listName) const;

    void onSelectedItemsListChanged(const QString& listName);

signals:
    void selectedItemsListChanged(const QString& listName);

private:
    mutable QReadWriteLock _selectionListsLock;
    QMap<QString, GameplayObjects> _selectedItemsListMap;

    mutable QReadWriteLock _selectionHandlersLock;
    QMap<QString, SelectionToSceneHandler*> _handlerMap;

    mutable QReadWriteLock _highlightStylesLock;
    QMap<QString, SelectionHighlightStyle> _highlightStyleMap;

    template <class T> bool addToGameplayObjects(const QString& listName, T idToAdd);
    template <class T> bool removeFromGameplayObjects(const QString& listName, T idToRemove);

    void setupHandler(const QString& selectionName);


    
};

#endif // hifi_SelectionScriptingInterface_h
