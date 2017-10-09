//
//  history.js
//
//  Created by David Rowe on 12 Sep 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global History: true */

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
        MAX_HISTORY_ITEMS = 1000,
        undoPosition = -1, // The next history item to undo; the next history item to redo = undoIndex + 1.
        undoData = [{}, {}, {}], // Left side, right side, no side.
        redoData = [{}, {}, {}],
        NO_SIDE = 2;

    function doKick(entityID) {
        var properties,
            NO_KICK_ENTITY_TYPES = ["Text", "Web", "PolyLine", "ParticleEffect"], // Don't respond to gravity so don't kick.
            DYNAMIC_VELOCITY_THRESHOLD = 0.05, // See EntityMotionState.cpp DYNAMIC_LINEAR_VELOCITY_THRESHOLD
            DYNAMIC_VELOCITY_KICK = { x: 0, y: 0.1, z: 0 };

        properties = Entities.getEntityProperties(entityID, ["type", "dynamic", "gravity", "velocity"]);
        if (NO_KICK_ENTITY_TYPES.indexOf(properties.type) === -1 && properties.dynamic
                && Vec3.length(properties.gravity) > 0 && Vec3.length(properties.velocity) < DYNAMIC_VELOCITY_THRESHOLD) {
            Entities.editEntity(entityID, { velocity: DYNAMIC_VELOCITY_KICK });
        }
    }

    function kickPhysics(entityID) {
        // Gives entities a small kick to start off physics, if necessary.
        var KICK_DELAY = 750; // ms

        // Give physics a chance to catch up. Avoids some erratic behavior.
        Script.setTimeout(function () { 
            doKick(entityID);
        }, KICK_DELAY);
    }

    function prePush(side, undo, redo) {
        // Stores undo and redo data to include in the next history entry generated for the side.
        undoData[side] = undo;
        redoData[side] = redo;
    }

    function push(side, undo, redo) {
        // Add a history entry.
        if (side === null) {
            side = NO_SIDE;
        }
        undoData[side] = Object.merge(undoData[side], undo);
        redoData[side] = Object.merge(redoData[side], redo);

        // Wipe any redo history after current undo position.
        if (undoPosition < history.length - 1) {
            history.splice(undoPosition + 1, history.length - undoPosition - 1);
        }

        // Limit the number of history items.
        if (history.length >= MAX_HISTORY_ITEMS) {
            history.splice(0, history.length - MAX_HISTORY_ITEMS + 1);
            undoPosition = history.length - 1;
        }

        history.push({ undoData: undoData[side], redoData: redoData[side] });
        undoPosition++;

        undoData[side] = {};
        redoData[side] = {};
    }

    function updateEntityIDs(oldEntityID, newEntityID) {
        // Replace oldEntityID value with newEntityID in history.
        var i,
            length;

        function updateEntityIDsInProperty(properties) {
            var i,
                length;

            if (properties) {
                for (i = 0, length = properties.length; i < length; i++) {
                    if (properties[i].entityID === oldEntityID) {
                        properties[i].entityID = newEntityID;
                    }
                    if (properties[i].properties && properties[i].properties.parentID === oldEntityID) {
                        properties[i].properties.parentID = newEntityID;
                    }
                }
            }
        }

        for (i = 0, length = history.length; i < length; i++) {
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

    function undoSetProperties(entityID, properties) {
        Entities.editEntity(entityID, properties);
        if (properties.gravity) {
            kickPhysics(entityID);
        }
    }

    function undo() {
        var undoData,
            entityID,
            REPEAT_UNDO_DELAY = 500, // ms
            i,
            length;

        if (undoPosition > -1) {
            undoData = history[undoPosition].undoData;

            if (undoData.createEntities) {
                for (i = 0, length = undoData.createEntities.length; i < length; i++) {
                    entityID = Entities.addEntity(undoData.createEntities[i].properties);
                    updateEntityIDs(undoData.createEntities[i].entityID, entityID);
                }
            }

            if (undoData.setProperties) {
                for (i = 0, length = undoData.setProperties.length; i < length; i++) {
                    undoSetProperties(undoData.setProperties[i].entityID, undoData.setProperties[i].properties);
                    if (undoData.setProperties[i].properties.velocity
                        && Vec3.equal(undoData.setProperties[i].properties.velocity, Vec3.ZERO)
                        && undoData.setProperties[i].properties.angularVelocity
                        && Vec3.equal(undoData.setProperties[i].properties.angularVelocity, Vec3.ZERO)) {
                        // Work around physics bug wherein the entity doesn't always end up at the correct position.
                        Script.setTimeout(
                            undoSetProperties(undoData.setProperties[i].entityID, undoData.setProperties[i].properties),
                            REPEAT_UNDO_DELAY
                        );
                    }
                }
            }

            if (undoData.deleteEntities) {
                for (i = 0, length = undoData.deleteEntities.length; i < length; i++) {
                    Entities.deleteEntity(undoData.deleteEntities[i].entityID);
                }
            }

            undoPosition--;
        }
    }

    function redo() {
        var redoData,
            entityID,
            i,
            length;


        if (undoPosition < history.length - 1) {
            redoData = history[undoPosition + 1].redoData;

            if (redoData.createEntities) {
                for (i = 0, length = redoData.createEntities.length; i < length; i++) {
                    entityID = Entities.addEntity(redoData.createEntities[i].properties);
                    updateEntityIDs(redoData.createEntities[i].entityID, entityID);
                }
            }

            if (redoData.setProperties) {
                for (i = 0, length = redoData.setProperties.length; i < length; i++) {
                    Entities.editEntity(redoData.setProperties[i].entityID, redoData.setProperties[i].properties);
                    if (redoData.setProperties[i].properties.gravity) {
                        kickPhysics(redoData.setProperties[i].entityID);
                    }
                }
            }

            if (redoData.deleteEntities) {
                for (i = 0, length = redoData.deleteEntities.length; i < length; i++) {
                    Entities.deleteEntity(redoData.deleteEntities[i].entityID);
                }
            }

            undoPosition++;
        }
    }

    return {
        prePush: prePush,
        push: push,
        hasUndo: hasUndo,
        hasRedo: hasRedo,
        undo: undo,
        redo: redo
    };
}());
