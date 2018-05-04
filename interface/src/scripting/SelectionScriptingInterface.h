
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

/**jsdoc
 * The <code>Selection</code> API provides a means of grouping together avatars, entities, and overlays in named lists.
 * @namespace Selection
 *
 * @hifi-interface
 * @hifi-client-entity
 *
 * @example <caption>Outline an entity when it is grabbed by a controller.</caption>
 * // Create a box and copy the following text into the entity's "Script URL" field.
 * (function () {
 *     print("Starting highlight script...............");
 *     var _this = this;
 *     var prevID = 0;
 *     var listName = "contextOverlayHighlightList";
 *     var listType = "entity";
 *
 *     _this.startNearGrab = function(entityID){
 *         if (prevID !== entityID) {
 *             Selection.addToSelectedItemsList(listName, listType, entityID);
 *             prevID = entityID;
 *         }
 *     };
 *
 *     _this.releaseGrab = function(entityID){
 *         if (prevID !== 0) {
 *             Selection.removeFromSelectedItemsList("contextOverlayHighlightList", listType, prevID);
 *             prevID = 0;
 *         }
 *     };
 *
 *     var cleanup = function(){
 *         Entities.findEntities(MyAvatar.position, 1000).forEach(function(entity) {
 *             try {
 *                 Selection.removeListFromMap(listName);
 *             } catch (e) {
 *                 print("Error cleaning up.");
 *             }
 *         });
 *     };
 *
 *     Script.scriptEnding.connect(cleanup);
 * });
 */
class SelectionScriptingInterface : public QObject, public Dependency {
    Q_OBJECT

public:
    SelectionScriptingInterface();

    /**jsdoc
    * Get the names of all the selection lists.
    * @function Selection.getListNames
    * @returns {list[]} An array of names of all the selection lists.
    */
    Q_INVOKABLE QStringList getListNames() const;

    /**jsdoc
    * Delete a named selection list.
    * @function Selection.removeListFromMap
    * @param {string} listName - The name of the selection list.
    * @returns {boolean} <code>true</code> if the selection existed and was successfully removed, otherwise <code>false</code>.
    */
    Q_INVOKABLE bool removeListFromMap(const QString& listName);

    /**jsdoc
    * Add an item to a selection list.
    * @function Selection.addToSelectedItemsList
    * @param {string} listName - The name of the selection list to add the item to.
    * @param {Selection.ItemType} itemType - The type of the item being added.
    * @param {Uuid} id - The ID of the item to add to the selection.
    * @returns {boolean} <code>true</code> if the item was successfully added, otherwise <code>false</code>.
    */
    Q_INVOKABLE bool addToSelectedItemsList(const QString& listName, const QString& itemType, const QUuid& id);
    /**jsdoc
    * Remove an item from a selection list.
    * @function Selection.removeFromSelectedItemsList
    * @param {string} listName - The name of the selection list to remove the item from.
    * @param {Selection.ItemType} itemType - The type of the item being removed.
    * @param {Uuid} id - The ID of the item to remove.
    * @returns {boolean} <code>true</code> if the item was successfully removed, otherwise <code>false</code>.
    *     <codefalse</code> is returned if the list doesn't contain any data.
    */
    Q_INVOKABLE bool removeFromSelectedItemsList(const QString& listName, const QString& itemType, const QUuid& id);
    /**jsdoc
    * Remove all items from a selection.
    * @function Selection.clearSelectedItemsList
    * @param {string} listName - The name of the selection list.
    * @returns {boolean} <code>true</code> if the item was successfully cleared, otherwise <code>false</code>.
    */
    Q_INVOKABLE bool clearSelectedItemsList(const QString& listName);

    /**jsdoc
    * Print out the list of avatars, entities, and overlays in a selection to the <em>debug log</em> (not the script log).
    * @function Selection.printList
    * @param {string} listName - The name of the selection list.
    */
    Q_INVOKABLE void printList(const QString& listName);

    /**jsdoc
    * Get the list of avatars, entities, and overlays stored in a selection list.
    * @function Selection.getList
    * @param {string} listName - The name of the selection list.
    * @returns {Selection.SelectedItemsList} The content of a selection list. If the list name doesn't exist, the function 
    *     returns an empty object with no properties.
    */
    Q_INVOKABLE QVariantMap getSelectedItemsList(const QString& listName) const;

    /**jsdoc
    * Get the names of the highlighted selection lists.
    * @function Selection.getHighlightedListNames
    * @returns {string[]} An array of names of the selection list currently highlight enabled.
    */
    Q_INVOKABLE QStringList getHighlightedListNames() const;

    /**jsdoc
    * Enable highlighting for a selection list.
    * If the selection list doesn't exist, it will be created.
    * All objects in the list will be displayed with the highlight effect specified.
    * The function can be called several times with different values in the style to modify it.<br />
    * Note: This function implicitly calls {@link Selection.enableListToScene}.
    * @function Selection.enableListHighlight
    * @param {string} listName - The name of the selection list.
    * @param {Selection.HighlightStyle} highlightStyle - The highlight style.
    * @returns {boolean} true if the selection was successfully enabled for highlight.
    */
    Q_INVOKABLE bool enableListHighlight(const QString& listName, const QVariantMap& highlightStyle);

    /**jsdoc
    * Disable highlighting for the selection list.
    * If the selection list doesn't exist or wasn't enabled for highlighting then nothing happens and <code>false</code> is
    * returned.<br />
    * Note: This function implicitly calls {@link Selection.disableListToScene}.
    * @function Selection.disableListHighlight
    * @param {string} listName - The name of the selection list.
    * @returns {boolean} <code>true</code> if the selection was successfully disabled for highlight, otherwise 
    *     <code>false</code>.
    */
    Q_INVOKABLE bool disableListHighlight(const QString& listName);
    /**jsdoc
    * Enable scene selection for the selection list.
    * If the Selection doesn't exist, it will be created.
    * All objects in the list will be sent to a scene selection.
    * @function Selection.enableListToScene
    * @param {string} listName - The name of the selection list.
    * @returns {boolean} <code>true</code> if the selection was successfully enabled on the scene, otherwise <code>false</code>.
    */
    Q_INVOKABLE bool enableListToScene(const QString& listName);

    /**jsdoc
    * Disable scene selection for the named selection.
    * If the selection list doesn't exist or wasn't enabled on the scene then nothing happens and <code>false</code> is
    * returned.
    * @function Selection.disableListToScene
    * @param {string} listName - The name of the selection list.
    * @returns {boolean} true if the selection was successfully disabled on the scene, false otherwise.
    */
    Q_INVOKABLE bool disableListToScene(const QString& listName);

    /**jsdoc
    * Get the highlight style values for the a selection list.
    * If the selection doesn't exist or hasn't been highlight enabled yet, an empty object is returned.
    * @function Selection.getListHighlightStyle
    * @param {string} listName - The name of the selection list.
    * @returns {Selection.HighlightStyle} highlight style 
    */
    Q_INVOKABLE QVariantMap getListHighlightStyle(const QString& listName) const;


    GameplayObjects getList(const QString& listName);

    render::HighlightStyle getHighlightStyle(const QString& listName) const;

    void onSelectedItemsListChanged(const QString& listName);

signals:
    /**jsoc
     * Triggered when a list's content changes.
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
