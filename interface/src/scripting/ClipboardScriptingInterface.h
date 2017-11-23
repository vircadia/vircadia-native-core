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
 * @namespace Clipboard
 */
class ClipboardScriptingInterface : public QObject {
    Q_OBJECT
public:
    ClipboardScriptingInterface();

signals:
    void readyToImport();
    
public:
    /**jsdoc
     * @function Clipboard.getContentsDimensions
     * @return {Vec3} The extents of the contents held in the clipboard.
     */
    Q_INVOKABLE glm::vec3 getContentsDimensions();

    /**jsdoc
     * Compute largest dimension of the extents of the contents held in the clipboard
     * @function Clipboard.getClipboardContentsLargestDimension
     * @return {float} The largest dimension computed.
     */
    Q_INVOKABLE float getClipboardContentsLargestDimension();

    /**jsdoc
     * Import entities from a .json file containing entity data into the clipboard.
     * You can generate * a .json file using {Clipboard.exportEntities}.
     * @function Clipboard.importEntities
     * @param {string} filename Filename of file to import.
     * @return {bool} True if the import was succesful, otherwise false.
     */
    Q_INVOKABLE bool importEntities(const QString& filename);

    /**jsdoc
     * Export the entities listed in `entityIDs` to the file `filename`
     * @function Clipboard.exportEntities
     * @param {string} filename Path to the file to export entities to.
     * @param {EntityID[]} entityIDs IDs of entities to export.
     * @return {bool} True if the export was succesful, otherwise false.
     */
    Q_INVOKABLE bool exportEntities(const QString& filename, const QVector<EntityItemID>& entityIDs);
    Q_INVOKABLE bool exportEntities(const QString& filename, float x, float y, float z, float s);

    /**jsdoc
     * Paste the contents of the clipboard into the world.
     * @function Clipboard.pasteEntities
     * @param {Vec3} position Position to paste clipboard at.
     * @return {EntityID[]} Array of entity IDs for the new entities that were
     *     created as a result of the paste operation.
     */
    Q_INVOKABLE QVector<EntityItemID> pasteEntities(glm::vec3 position);
};

#endif // hifi_ClipboardScriptingInterface_h
