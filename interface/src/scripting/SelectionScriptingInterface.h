
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
#include <QtCore/QSharedPointer>
#include <DependencyManager.h>

#include <AbstractViewStateInterface.h>

#include "RenderableEntityItem.h"
#include <avatar/AvatarManager.h>
#include <render/HighlightStyle.h>

class GameplayObjects {
public:
    GameplayObjects();

    bool getContainsData() const { return _containsData; }

    std::vector<QUuid> getAvatarIDs() const { return _avatarIDs; }
    bool addToGameplayObjects(const QUuid& avatarID);
    bool removeFromGameplayObjects(const QUuid& avatarID);

    std::vector<EntityItemID> getEntityIDs() const { return _entityIDs; }
    bool addToGameplayObjects(const EntityItemID& entityID);
    bool removeFromGameplayObjects(const EntityItemID& entityID);

private:
    bool _containsData { false };
    std::vector<QUuid> _avatarIDs;
    std::vector<EntityItemID> _entityIDs;
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

/*@jsdoc
 * The <code>Selection</code> API provides a means of grouping together and highlighting avatars and entities in named lists.
 *
 * @namespace Selection
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 *
 * @example <caption>Outline an entity when it is grabbed by the mouse or a controller.</caption>
 * // Create an entity and copy the following script into the entity's "Script URL" field.
 * // Move the entity behind another entity to see the occluded outline.
 * (function () {
 *     var LIST_NAME = "SelectionExample",
 *         ITEM_TYPE = "entity",
 *         HIGHLIGHT_STYLE = {
 *             outlineUnoccludedColor: { red: 0, green: 180, blue: 239 },
 *             outlineUnoccludedAlpha: 0.5,
 *             outlineOccludedColor: { red: 239, green: 180, blue: 0 },
 *             outlineOccludedAlpha: 0.5,
 *             outlineWidth: 4
 *         };
 * 
 *     Selection.enableListHighlight(LIST_NAME, HIGHLIGHT_STYLE);
 * 
 *     this.startNearGrab = function (entityID) {
 *         Selection.addToSelectedItemsList(LIST_NAME, ITEM_TYPE, entityID);
 *     };
 * 
 *     this.startDistanceGrab = function (entityID) {
 *         Selection.addToSelectedItemsList(LIST_NAME, ITEM_TYPE, entityID);
 *     };
 * 
 *     this.releaseGrab = function (entityID) {
 *         Selection.removeFromSelectedItemsList(LIST_NAME, ITEM_TYPE, entityID);
 *     };
 * 
 *     Script.scriptEnding.connect(function () {
 *         Selection.removeListFromMap(LIST_NAME);
 *     });
 * });
 */
class SelectionScriptingInterface : public QObject, public Dependency {
    Q_OBJECT

public:
    SelectionScriptingInterface();

    /*@jsdoc
     * Gets the names of all current selection lists.
     * @function Selection.getListNames
     * @returns {string[]} The names of all current selection lists.
     * @example <caption>List all the current selection lists.</caption>
     * print("Selection lists: " + Selection.getListNames());
     */
    Q_INVOKABLE QStringList getListNames() const;

    /*@jsdoc
     * Deletes a selection list.
     * @function Selection.removeListFromMap
     * @param {string} listName - The name of the selection list to delete.
     * @returns {boolean} <code>true</code> if the selection existed and was successfully removed, otherwise <code>false</code>.
     */
    Q_INVOKABLE bool removeListFromMap(const QString& listName);

    /*@jsdoc
     * Adds an item to a selection list. The list is created if it doesn't exist.
     * @function Selection.addToSelectedItemsList
     * @param {string} listName - The name of the selection list to add the item to.
     * @param {Selection.ItemType} itemType - The type of item being added.
     * @param {Uuid} itemID - The ID of the item to add.
     * @returns {boolean} <code>true</code> if the item was successfully added or already existed in the list, otherwise 
     *     <code>false</code>.
     */
    Q_INVOKABLE bool addToSelectedItemsList(const QString& listName, const QString& itemType, const QUuid& id);

    /*@jsdoc
     * Removes an item from a selection list.
     * @function Selection.removeFromSelectedItemsList
     * @param {string} listName - The name of the selection list to remove the item from.
     * @param {Selection.ItemType} itemType - The type of item being removed.
     * @param {Uuid} itemID - The ID of the item to remove.
     * @returns {boolean} <code>true</code> if the item was successfully removed or was not in the list, otherwise 
     *     <code>false</code>.
     */
    Q_INVOKABLE bool removeFromSelectedItemsList(const QString& listName, const QString& itemType, const QUuid& id);

    /*@jsdoc
     * Removes all items from a selection list.
     * @function Selection.clearSelectedItemsList
     * @param {string} listName - The name of the selection list.
     * @returns {boolean} <code>true</code> always.
     */
    Q_INVOKABLE bool clearSelectedItemsList(const QString& listName);

    /*@jsdoc
     * Prints the list of avatars and entities in a selection to the program log (but not the Script Log window).
     * @function Selection.printList
     * @param {string} listName - The name of the selection list.
     */
    Q_INVOKABLE void printList(const QString& listName);

    /*@jsdoc
     * Gets the list of avatars and entities in a selection list.
     * @function Selection.getSelectedItemsList
     * @param {string} listName - The name of the selection list.
     * @returns {Selection.SelectedItemsList} The content of the selection list if the list exists, otherwise an empty object.
     */
    Q_INVOKABLE QVariantMap getSelectedItemsList(const QString& listName) const;

    /*@jsdoc
     * Gets the names of all current selection lists that have highlighting enabled.
     * @function Selection.getHighlightedListNames
     * @returns {string[]} The names of the selection lists that currently have highlighting enabled.
     */
    Q_INVOKABLE QStringList getHighlightedListNames() const;

    /*@jsdoc
     * Enables highlighting for a selection list. All items in or subsequently added to the list are displayed with the 
     * highlight effect specified. The method can be called multiple times with different values in the style to modify the 
     * highlighting.
     * <p>Note: This function implicitly calls {@link Selection.enableListToScene|enableListToScene}.</p>
     * @function Selection.enableListHighlight
     * @param {string} listName - The name of the selection list.
     * @param {Selection.HighlightStyle} highlightStyle - The highlight style.
     * @returns {boolean} <code>true</code> always.
     */
    Q_INVOKABLE bool enableListHighlight(const QString& listName, const QVariantMap& highlightStyle);

    /*@jsdoc
     * Disables highlighting for a selection list.
     * <p>Note: This function implicitly calls {@link Selection.disableListToScene|disableListToScene}.</p>
     * @function Selection.disableListHighlight
     * @param {string} listName - The name of the selection list.
     * @returns {boolean} <code>true</code> always.
     */
    Q_INVOKABLE bool disableListHighlight(const QString& listName);

    /*@jsdoc
     * Enables scene selection for a selection list. All items in or subsequently added to the list are sent to a scene 
     * selection in the rendering engine for debugging purposes.
     * @function Selection.enableListToScene
     * @param {string} listName - The name of the selection list.
     * @returns {boolean} <code>true</code> always.
     */
    Q_INVOKABLE bool enableListToScene(const QString& listName);

    /*@jsdoc
     * Disables scene selection for a selection list.
     * @function Selection.disableListToScene
     * @param {string} listName - The name of the selection list.
     * @returns {boolean} <code>true</code> always.
     */
    Q_INVOKABLE bool disableListToScene(const QString& listName);

    /*@jsdoc
     * Gets the current highlighting style for a selection list.
     * @function Selection.getListHighlightStyle
     * @param {string} listName - The name of the selection list.
     * @returns {Selection.HighlightStyle} The highlight style of the selection list if the list exists and highlighting is 
     *     enabled, otherwise an empty object.
     */
    Q_INVOKABLE QVariantMap getListHighlightStyle(const QString& listName) const;


    GameplayObjects getList(const QString& listName);

    render::HighlightStyle getHighlightStyle(const QString& listName) const;

    void onSelectedItemsListChanged(const QString& listName);

signals:
    /*@jsdoc
     * Triggered when a selection list's content changes or the list is deleted.
     * @function Selection.selectedItemsListChanged
     * @param {string} listName - The name of the selection list that changed.
     * @returns {Signal}
     */
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
    void removeHandler(const QString& selectionName);

};

#endif // hifi_SelectionScriptingInterface_h
