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

/**jsdoc
 * The Clipboard API enables you to export and import entities to and from JSON files.
 *
 * @namespace Clipboard
 */
class ClipboardScriptingInterface : public QObject {
    Q_OBJECT
public:
    ClipboardScriptingInterface();

public:
    /**jsdoc
     * Compute the extents of the contents held in the clipboard.
     * @function Clipboard.getContentsDimensions
     * @returns {Vec3} The extents of the contents held in the clipboard.
     */
    Q_INVOKABLE glm::vec3 getContentsDimensions();

    /**jsdoc
     * Compute the largest dimension of the extents of the contents held in the clipboard.
     * @function Clipboard.getClipboardContentsLargestDimension
     * @returns {Number} The largest dimension computed.
     */
    Q_INVOKABLE float getClipboardContentsLargestDimension();

    /**jsdoc
     * Import entities from a JSON file containing entity data into the clipboard.
     * You can generate a JSON file using {@link Clipboard.exportEntities}.
     * @function Clipboard.importEntities
     * @param {String} filename Path and name of file to import.
     * @returns {Boolean} <code>true</code> if the import was successful, otherwise <code>false</code>.
     */
    Q_INVOKABLE bool importEntities(const QString& filename);

    /**jsdoc
     * Export the entities specified to a JSON file.
     * @function Clipboard.exportEntities
     * @param {String} filename Path and name of the file to export the entities to. Should have the extension ".json".
     * @param {EntityID[]} entityIDs Array of IDs of the entities to export.
     * @returns {Boolean} <code>true</code> if the export was successful, otherwise <code>false</code>.
     */
    Q_INVOKABLE bool exportEntities(const QString& filename, const QVector<EntityItemID>& entityIDs);
    
    /**jsdoc
    * Export the entities with centers within a cube to a JSON file.
    * @function Clipboard.exportEntities
    * @param {String} filename Path and name of the file to export the entities to. Should have the extension ".json".
    * @param {Number} x X-coordinate of the cube center.
    * @param {Number} y Y-coordinate of the cube center.
    * @param {Number} z Z-coordinate of the cube center.
    * @param {Number} scale Half dimension of the cube.
    * @returns {Boolean} <code>true</code> if the export was successful, otherwise <code>false</code>.
    */
    Q_INVOKABLE bool exportEntities(const QString& filename, float x, float y, float z, float scale);

    /**jsdoc
     * Paste the contents of the clipboard into the world.
     * @function Clipboard.pasteEntities
     * @param {Vec3} position Position to paste the clipboard contents at.
     * @returns {EntityID[]} Array of entity IDs for the new entities that were created as a result of the paste operation.
     */
    Q_INVOKABLE QVector<EntityItemID> pasteEntities(glm::vec3 position);
};

#endif // hifi_ClipboardScriptingInterface_h
