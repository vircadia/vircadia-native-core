//
//  history.js
//
//  Created by David Rowe on 12 Sep 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global History */

History = (function () {
    // Provides undo facility.
    // Global object.

    "use strict";

    var history = [
            /*
            {
                undoData: {
                    setProperties: [
                        {
                            entityID: <entity ID>,
                            properties: { <name>: <value>, <name>: <value>, ... }
                        }
                    ],
                    createEntities: [
                        {
                            entityID: <entity ID>,
                            properties: { <name>: <value>, <name>: <value>, ... }
                        }
                    ],
                    deleteEntities: [
                        {
                            entityID: <entity ID>,
                            properties: { <name>: <value>, <name>: <value>, ... }
                        }
                    ]
                },
                redoData: {
                    ""
                }
            }
            */
        ],
        MAX_HISTORY_ITEMS = 100,
        undoPosition = -1;  // The next history item to undo; the next history item to redo = undoIndex + 1.

    function push(undoData, redoData) {
        // Wipe any redo history after current undo position.
        if (undoPosition < history.length - 1) {
            history.splice(undoPosition + 1, history.length - undoPosition - 1);
        }

        // Limit the number of history items.
        if (history.length >= MAX_HISTORY_ITEMS) {
            history.splice(0, history.length - MAX_HISTORY_ITEMS + 1);
        }

        history.push({ undoData: undoData, redoData: redoData });
        undoPosition += 1;
    }

    function updateEntityIDs(oldEntityID, newEntityID) {
        // Replace oldEntityID value with newEntityID in history.
        var i,
            length;

        function updateEntityIDsInProperty(properties) {
            var i,
                length;

            if (properties) {
                for (i = 0, length = properties.length; i < length; i += 1) {
                    if (properties[i].entityID === oldEntityID) {
                        properties[i].entityID = newEntityID;
                    }
                    if (properties[i].properties && properties[i].properties.parentID === oldEntityID) {
                        properties[i].properties.parentID = newEntityID;
                    }
                }
            }
        }

        for (i = 0, length = history.length; i < length; i += 1) {
            if (history[i].undoData) {
                updateEntityIDsInProperty(history[i].undoData.setProperties);
                updateEntityIDsInProperty(history[i].undoData.createEntities);
                updateEntityIDsInProperty(history[i].undoData.deleteEntities);
            }
            if (history[i].redoData) {
                updateEntityIDsInProperty(history[i].redoData.setProperties);
                updateEntityIDsInProperty(history[i].redoData.createEntities);
                updateEntityIDsInProperty(history[i].redoData.deleteEntities);
            }
        }
    }

    function hasUndo() {
        return undoPosition > -1;
    }

    function hasRedo() {
        return undoPosition < history.length - 1;
    }

    function undo() {
        var undoData,
            i,
            length;

        if (undoPosition > -1) {
            undoData = history[undoPosition].undoData;

            // TODO

            if (undoData.deleteEntities) {
                for (i = 0, length = undoData.deleteEntities.length; i < length; i += 1) {
                    Entities.deleteEntity(undoData.deleteEntities[i].entityID);
                }
            }

            undoPosition -= 1;
        }
    }

    function redo() {
        var redoData,
            entityID,
            i,
            length;


        if (undoPosition < history.length - 1) {
            redoData = history[undoPosition + 1].redoData;

            // TODO

            if (redoData.createEntities) {
                for (i = 0, length = redoData.createEntities.length; i < length; i += 1) {
                    entityID = Entities.addEntity(redoData.createEntities[i].properties);
                    updateEntityIDs(redoData.createEntities[i].entityID, entityID);
                }
            }

            undoPosition += 1;
        }
    }

    return {
        push: push,
        hasUndo: hasUndo,
        hasRedo: hasRedo,
        undo: undo,
        redo: redo
    };
}());
