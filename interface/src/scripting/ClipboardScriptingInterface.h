//
//  ClipboardScriptingInterface.h
//  interface/src/scripting
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ClipboardScriptingInterface_h
#define hifi_ClipboardScriptingInterface_h

#include <QObject>

#include <glm/glm.hpp>

#include <EntityItemID.h>

/*@jsdoc
 * The <code>Clipboard</code> API enables you to export and import entities to and from JSON files.
 *
 * @namespace Clipboard
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 */
class ClipboardScriptingInterface : public QObject {
    Q_OBJECT
public:
    ClipboardScriptingInterface();

public:
    /*@jsdoc
     * Gets the extents of the entities held in the clipboard.
     * @function Clipboard.getContentsDimensions
     * @returns {Vec3} The extents of the content held in the clipboard.
     * @example <caption>Import entities to the clipboard and report their overall dimensions.</caption>
     * var filename = Window.browse("Import entities to clipboard", "", "*.json");
     * if (filename) {
     *     if (Clipboard.importEntities(filename)) {
     *         print("Clipboard dimensions: " + JSON.stringify(Clipboard.getContentsDimensions()));
     *     }
     * }
     */
    Q_INVOKABLE glm::vec3 getContentsDimensions();

    /*@jsdoc
     * Gets the largest dimension of the extents of the entities held in the clipboard.
     * @function Clipboard.getClipboardContentsLargestDimension
     * @returns {number} The largest dimension of the extents of the content held in the clipboard.
     */
    Q_INVOKABLE float getClipboardContentsLargestDimension();

    /*@jsdoc
     * Imports entities from a JSON file into the clipboard.
     * @function Clipboard.importEntities
     * @param {string} filename - The path and name of the JSON file to import.
     * @param {boolean} [isObservable=true] - <code>true</code> if the {@link ResourceRequestObserver} can observe this 
     *     request, <code>false</code> if it can't.
     * @param {number} [callerID=-1] - An integer ID that is passed through to the {@link ResourceRequestObserver}.
     * @returns {boolean} <code>true</code> if the import was successful, otherwise <code>false</code>.
     * @example <caption>Import entities and paste into the domain.</caption>
     * var filename = Window.browse("Import entities to clipboard", "", "*.json");
     * if (filename) {
     *     if (Clipboard.importEntities(filename)) {
     *         pastedEntities = Clipboard.pasteEntities(Vec3.sum(MyAvatar.position,
     *             Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -3 })));
     *         print("Entities pasted: " + JSON.stringify(pastedEntities));
     *     }
     * }
     */
    Q_INVOKABLE bool importEntities(const QString& filename, const bool isObservable = true, const qint64 callerId = -1);

    /*@jsdoc
     * Exports specified entities to a JSON file.
     * @function Clipboard.exportEntities
     * @param {string} filename - Path and name of the file to export the entities to. Should have the extension ".json".
     * @param {Uuid[]} entityIDs - The IDs of the entities to export.
     * @returns {boolean} <code>true</code> if entities were found and the file was written, otherwise <code>false</code>.
     * @example <caption>Create and export a cube and a sphere.</caption>
     * // Create entities.
     * var box = Entities.addEntity({
     *     type: "Box",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: -0.2, y: 0, z: -3 })),
     *     lifetime: 300 // Delete after 5 minutes.
     * });
     * var sphere = Entities.addEntity({
     *     type: "Sphere",
     *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0.2, y: 0, z: -3 })),
     *     lifetime: 300 // Delete after 5 minutes.
     * });
     * 
     * // Export entities.
     * var filename = Window.save("Export entities to JSON file", Paths.resources, "*.json");
     * if (filename) {
     *     Clipboard.exportEntities(filename, [box, sphere]);
     * }
     */
    Q_INVOKABLE bool exportEntities(const QString& filename, const QVector<QUuid>& entityIDs);
    
    /*@jsdoc
    * Exports all entities that have centers within a cube to a JSON file.
    * @function Clipboard.exportEntities
    * @variation 0
    * @param {string} filename - Path and name of the file to export the entities to. Should have the extension ".json".
    * @param {number} x - X-coordinate of the cube center.
    * @param {number} y - Y-coordinate of the cube center.
    * @param {number} z - Z-coordinate of the cube center.
    * @param {number} scale - Half dimension of the cube.
    * @returns {boolean} <code>true</code> if entities were found and the file was written, otherwise <code>false</code>.
    */
    Q_INVOKABLE bool exportEntities(const QString& filename, float x, float y, float z, float scale);

    /*@jsdoc
     * Pastes the contents of the clipboard into the domain.
     * @function Clipboard.pasteEntities
     * @param {Vec3} position -  The position to paste the clipboard contents at.
     * @param {Entities.EntityHostType} [entityHostType="domain"] - The type of entities to create.
     * @returns {Uuid[]} The IDs of the new entities that were created as a result of the paste operation. If entities couldn't 
     *     be created then an empty array is returned.
     */
    Q_INVOKABLE QVector<EntityItemID> pasteEntities(glm::vec3 position, const QString& entityHostType = "domain");
};

#endif // hifi_ClipboardScriptingInterface_h
