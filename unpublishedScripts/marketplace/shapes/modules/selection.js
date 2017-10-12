//
//  selection.js
//
//  Created by David Rowe on 21 Jul 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global SelectionManager:true, App, History */

SelectionManager = function (side) {
    // Manages set of selected entities. Currently supports just one set of linked entities.

    "use strict";

    var selection = [], // Subset of properties to provide externally.
        selectionIDs = [],
        selectionProperties = [], // Full set of properties for history.
        intersectedEntityID = null,
        intersectedEntityIndex,
        rootEntityID = null,
        rootPosition,
        rootOrientation,
        scaleFactor,
        scaleRootOffset,
        scaleRootOrientation,
        startPosition,
        startOrientation,
        isEditing = false,
        ENTITY_TYPE = "entity",
        ENTITY_TYPES_WITH_COLOR = ["Box", "Sphere", "Shape", "PolyLine"],
        ENTITY_TYPES_2D = ["Text", "Web"],
        MIN_HISTORY_MOVE_DISTANCE = 0.005,
        MIN_HISTORY_ROTATE_ANGLE = 0.017453; // Radians = 1 degree.

    if (!(this instanceof SelectionManager)) {
        return new SelectionManager(side);
    }

    function traverseEntityTree(id, selection, selectionIDs, selectionProperties) {
        // Recursively traverses tree of entities and their children, gather IDs and properties.
        // The root entity is always the first entry.
        var children,
            properties,
            i,
            length;

        properties = Entities.getEntityProperties(id);
        delete properties.entityID;
        selection.push({
            id: id,
            type: properties.type,
            position: properties.position,
            parentID: properties.parentID,
            localPosition: properties.localPosition,
            localRotation: properties.localRotation,
            registrationPoint: properties.registrationPoint,
            rotation: properties.rotation,
            dimensions: properties.dimensions,
            dynamic: properties.dynamic,
            collisionless: properties.collisionless,
            userData: properties.userData
        });
        selectionIDs.push(id);
        selectionProperties.push({ entityID: id, properties: properties });

        if (id === intersectedEntityID) {
            intersectedEntityIndex = selection.length - 1;
        }

        children = Entities.getChildrenIDs(id);
        for (i = 0, length = children.length; i < length; i++) {
            if (Entities.getNestableType(children[i]) === ENTITY_TYPE) {
                traverseEntityTree(children[i], selection, selectionIDs, selectionProperties);
            }
        }
    }

    function select(intersectionEntityID) {
        var entityProperties,
            PARENT_PROPERTIES = ["parentID", "position", "rotation", "dymamic", "collisionless"];

        intersectedEntityID = intersectionEntityID;

        // Find root parent.
        rootEntityID = Entities.rootOf(intersectedEntityID);

        // Selection position and orientation is that of the root entity.
        entityProperties = Entities.getEntityProperties(rootEntityID, PARENT_PROPERTIES);
        rootPosition = entityProperties.position;
        rootOrientation = entityProperties.rotation;

        // Find all children.
        selection = [];
        selectionIDs = [];
        selectionProperties = [];
        traverseEntityTree(rootEntityID, selection, selectionIDs, selectionProperties);
    }

    function append(rootEntityID) {
        // Add further entities to the selection.
        // Assumes that rootEntityID is not already in the selection.
        traverseEntityTree(rootEntityID, selection, selectionIDs, selectionProperties);
    }

    function getIntersectedEntityID() {
        return intersectedEntityID;
    }

    function getIntersectedEntityIndex() {
        return intersectedEntityIndex;
    }

    function getRootEntityID() {
        return rootEntityID;
    }

    function getSelection() {
        return selection;
    }

    function contains(entityID) {
        return selectionIDs.indexOf(entityID) !== -1;
    }

    function count() {
        return selection.length;
    }

    function getBoundingBox() {
        var center,
            localCenter,
            orientation,
            inverseOrientation,
            dimensions,
            min,
            max,
            i,
            j,
            length,
            registration,
            position,
            rotation,
            corners = [],
            NUM_CORNERS = 8;

        if (selection.length === 1) {
            if (Vec3.equal(selection[0].registrationPoint, Vec3.HALF)) {
                center = rootPosition;
            } else {
                center = Vec3.sum(rootPosition,
                    Vec3.multiplyQbyV(rootOrientation,
                        Vec3.multiplyVbyV(selection[0].dimensions,
                            Vec3.subtract(Vec3.HALF, selection[0].registrationPoint))));
            }
            localCenter = Vec3.multiplyQbyV(Quat.inverse(rootOrientation), Vec3.subtract(center, rootPosition));
            orientation = rootOrientation;
            dimensions = selection[0].dimensions;
        } else if (selection.length > 1) {
            // Find min & max x, y, z values of entities' dimension box corners in root entity coordinate system.
            // Note: Don't use entities' bounding boxes because they're in world coordinates and may make the calculated
            // bounding box be larger than necessary.
            min = Vec3.multiplyVbyV(Vec3.subtract(Vec3.ZERO, selection[0].registrationPoint), selection[0].dimensions);
            max = Vec3.multiplyVbyV(Vec3.subtract(Vec3.ONE, selection[0].registrationPoint), selection[0].dimensions);
            inverseOrientation = Quat.inverse(rootOrientation);
            for (i = 1, length = selection.length; i < length; i++) {

                registration = selection[i].registrationPoint;
                corners = [
                    { x: -registration.x, y: -registration.y, z: -registration.z },
                    { x: -registration.x, y: -registration.y, z: 1.0 - registration.z },
                    { x: -registration.x, y: 1.0 - registration.y, z: -registration.z },
                    { x: -registration.x, y: 1.0 - registration.y, z: 1.0 - registration.z },
                    { x: 1.0 - registration.x, y: -registration.y, z: -registration.z },
                    { x: 1.0 - registration.x, y: -registration.y, z: 1.0 - registration.z },
                    { x: 1.0 - registration.x, y: 1.0 - registration.y, z: -registration.z },
                    { x: 1.0 - registration.x, y: 1.0 - registration.y, z: 1.0 - registration.z }
                ];

                position = selection[i].position;
                rotation = selection[i].rotation;
                dimensions = selection[i].dimensions;

                for (j = 0; j < NUM_CORNERS; j++) {
                    // Corner position in world coordinates.
                    corners[j] = Vec3.sum(position, Vec3.multiplyQbyV(rotation, Vec3.multiplyVbyV(corners[j], dimensions)));
                    // Corner position in root entity coordinates.
                    corners[j] = Vec3.multiplyQbyV(inverseOrientation, Vec3.subtract(corners[j], rootPosition));
                    // Update min & max.
                    min = Vec3.min(corners[j], min);
                    max = Vec3.max(corners[j], max);
                }
            }

            // Calculate bounding box.
            center = Vec3.sum(rootPosition,
                Vec3.multiplyQbyV(rootOrientation, Vec3.multiply(0.5, Vec3.sum(min, max))));
            localCenter = Vec3.multiply(0.5, Vec3.sum(min, max));
            orientation = rootOrientation;
            dimensions = Vec3.subtract(max, min);
        }

        return {
            center: center,
            localCenter: localCenter,
            orientation: orientation,
            dimensions: dimensions
        };
    }

    function is2D() {
        return selection.length === 1 && ENTITY_TYPES_2D.indexOf(selection[0].type) !== -1;
    }

    function doKick(entityID) {
        var properties,
            NO_KICK_ENTITY_TYPES = ["Text", "Web", "PolyLine", "ParticleEffect"], // Don't respond to gravity so don't kick.
            DYNAMIC_VELOCITY_THRESHOLD = 0.05, // See EntityMotionState.cpp DYNAMIC_LINEAR_VELOCITY_THRESHOLD
            DYNAMIC_VELOCITY_KICK = { x: 0, y: 0.1, z: 0 };

        if (entityID === rootEntityID && isEditing) {
            // Don't kick if have started editing entity again.
            return;
        }

        properties = Entities.getEntityProperties(entityID, ["type", "velocity", "gravity"]);
        if (NO_KICK_ENTITY_TYPES.indexOf(properties.type) === -1
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

    function startEditing() {
        var i;

        // Remember start properties for history entry.
        startPosition = selection[0].position;
        startOrientation = selection[0].rotation;

        // Disable entity set's physics.
        for (i = selection.length - 1; i >= 0; i--) {
            Entities.editEntity(selection[i].id, {
                dynamic: false, // So that gravity doesn't fight with us trying to hold the entity in place.
                collisionless: true, // So that entity doesn't bump us about as we resize the entity.
                velocity: Vec3.ZERO, // So that entity doesn't drift if we've grabbed a set while it was moving.
                angularVelocity: Vec3.ZERO // ""
            });
        }

        // Stop moving.
        Entities.editEntity(rootEntityID, { velocity: Vec3.ZERO, angularVelocity: Vec3.ZERO });

        isEditing = true;
    }

    function finishEditing() {
        var i;

        // Restore entity set's physics.
        // Note: Need to apply children-first in order to avoid children's relative positions sometimes drifting.
        for (i = selection.length - 1; i >= 0; i--) {
            Entities.editEntity(selection[i].id, {
                dynamic: selection[i].dynamic,
                collisionless: selection[i].collisionless
            });
        }

        // Add history entry.
        if (selection.length > 0
                && (!Vec3.equal(startPosition, rootPosition) || !Quat.equal(startOrientation, rootOrientation))) {
            // Positions and orientations can be identical if change grabbing hands when finish scaling.
            History.push(
                side,
                {
                    setProperties: [
                        { entityID: rootEntityID, properties: { position: startPosition, rotation: startOrientation } }
                    ]
                },
                {
                    setProperties: [
                        { entityID: rootEntityID, properties: { position: rootPosition, rotation: rootOrientation } }
                    ]
                }
            );
        }

        // Kick off physics if necessary.
        if (selection.length > 0 && selection[0].dynamic) {
            kickPhysics(selection[0].id);
        }

        isEditing = false;
    }

    function getPositionAndOrientation() {
        // Position and orientation of root entity.
        return {
            position: rootPosition,
            orientation: rootOrientation
        };
    }

    function setPositionAndOrientation(position, orientation) {
        // Position and orientation of root entity.
        rootPosition = position;
        rootOrientation = orientation;
        Entities.editEntity(rootEntityID, {
            position: position,
            rotation: orientation
        });
    }

    function startDirectScaling(center) {
        // Save initial position and orientation so that can scale relative to these without accumulating float errors.
        scaleRootOffset = Vec3.subtract(rootPosition, center);
        scaleRootOrientation = rootOrientation;

        // User is grabbing entity; add a history entry for movement up until the start of scaling and update start position and
        // orientation; unless very small movement.
        if (Vec3.distance(startPosition, rootPosition) >= MIN_HISTORY_MOVE_DISTANCE
                || Quat.rotationBetween(startOrientation, rootOrientation) >= MIN_HISTORY_ROTATE_ANGLE) {
            History.push(
                side,
                {
                    setProperties: [
                        { entityID: rootEntityID, properties: { position: startPosition, rotation: startOrientation } }
                    ]
                },
                {
                    setProperties: [
                        { entityID: rootEntityID, properties: { position: rootPosition, rotation: rootOrientation } }
                    ]
                }
            );
            startPosition = rootPosition;
            startOrientation = rootOrientation;
        }
    }

    function directScale(factor, rotation, center) {
        // Scale, position, and rotate selection.
        // We can get away with scaling the z size of 2D entities - incongruities are barely noticeable and things recover.
        var i,
            length;

        // Scale, position, and orient root.
        rootPosition = Vec3.sum(center, Vec3.multiply(factor, Vec3.multiplyQbyV(rotation, scaleRootOffset)));
        rootOrientation = Quat.multiply(rotation, scaleRootOrientation);
        Entities.editEntity(selection[0].id, {
            dimensions: Vec3.multiply(factor, selection[0].dimensions),
            position: rootPosition,
            rotation: rootOrientation
        });

        // Scale and position children.
        for (i = 1, length = selection.length; i < length; i++) {
            Entities.editEntity(selection[i].id, {
                dimensions: Vec3.multiply(factor, selection[i].dimensions),
                localPosition: Vec3.multiply(factor, selection[i].localPosition),
                localRotation: selection[i].localRotation // Always specify localRotation otherwise rotations can drift.
            });
        }

        // Save most recent scale factor.
        scaleFactor = factor;
    }

    function finishDirectScaling() {
        // Update selection with final entity properties.
        var undoData = [],
            redoData = [],
            i,
            length;

        // Final scale, position, and orientation of root.
        undoData.push({
            entityID: selection[0].id,
            properties: {
                dimensions: selection[0].dimensions,
                position: startPosition,
                rotation: startOrientation
            }
        });
        selection[0].dimensions = Vec3.multiply(scaleFactor, selection[0].dimensions);
        selection[0].position = rootPosition;
        selection[0].rotation = rootOrientation;
        redoData.push({
            entityID: selection[0].id,
            properties: {
                dimensions: selection[0].dimensions,
                position: selection[0].position,
                rotation: selection[0].rotation
            }
        });

        // Final scale and position of children.
        for (i = 1, length = selection.length; i < length; i++) {
            undoData.push({
                entityID: selection[i].id,
                properties: {
                    dimensions: selection[i].dimensions,
                    localPosition: selection[i].localPosition,
                    localRotation: selection[i].localRotation
                }
            });
            selection[i].dimensions = Vec3.multiply(scaleFactor, selection[i].dimensions);
            selection[i].localPosition = Vec3.multiply(scaleFactor, selection[i].localPosition);
            redoData.push({
                entityID: selection[i].id,
                properties: {
                    dimensions: selection[i].dimensions,
                    localPosition: selection[i].localPosition,
                    localRotation: selection[i].localRotation
                }
            });
        }

        // Add history entry.
        History.push(side, { setProperties: undoData }, { setProperties: redoData });

        // Update grab start data for its undo.
        startPosition = rootPosition;
        startOrientation = rootOrientation;
    }

    function startHandleScaling(position) {
        // Save initial offset from hand position to root position so that can scale without accumulating float errors.
        scaleRootOffset = Vec3.multiplyQbyV(Quat.inverse(rootOrientation), Vec3.subtract(rootPosition, position));

        // User is grabbing entity; add a history entry for movement up until the start of scaling and update start position and
        // orientation; unless very small movement.
        if (Vec3.distance(startPosition, rootPosition) >= MIN_HISTORY_MOVE_DISTANCE
                || Quat.rotationBetween(startOrientation, rootOrientation) >= MIN_HISTORY_ROTATE_ANGLE) {
            History.push(
                side,
                {
                    setProperties: [
                        { entityID: rootEntityID, properties: { position: startPosition, rotation: startOrientation } }
                    ]
                },
                {
                    setProperties: [
                        { entityID: rootEntityID, properties: { position: rootPosition, rotation: rootOrientation } }
                    ]
                }
            );
            startPosition = rootPosition;
            startOrientation = rootOrientation;
        }
    }

    function handleScale(factor, position, orientation) {
        // Scale and reposition and orient selection.
        // We can get away with scaling the z size of 2D entities - incongruities are barely noticeable and things recover.
        var i,
            length;

        // Scale and position root.
        rootPosition = Vec3.sum(Vec3.multiplyQbyV(orientation, Vec3.multiplyVbyV(factor, scaleRootOffset)), position);
        rootOrientation = orientation;
        Entities.editEntity(selection[0].id, {
            dimensions: Vec3.multiplyVbyV(factor, selection[0].dimensions),
            position: rootPosition,
            rotation: rootOrientation
        });

        // Scale and position children.
        // Only corner handles are used for scaling multiple entities so scale factor is the same in all dimensions.
        // Therefore don't need to take into account orientation relative to parent when scaling local position.
        for (i = 1, length = selection.length; i < length; i++) {
            Entities.editEntity(selection[i].id, {
                dimensions: Vec3.multiplyVbyV(factor, selection[i].dimensions),
                localPosition: Vec3.multiplyVbyV(factor, selection[i].localPosition),
                localRotation: selection[i].localRotation // Always specify localRotation otherwise rotations can drift.
            });
        }

        // Save most recent scale factor.
        scaleFactor = factor;
    }

    function finishHandleScaling() {
        // Update selection with final entity properties.
        var undoData = [],
            redoData = [],
            i,
            length;

        // Final scale and position of root.
        undoData.push({
            entityID: selection[0].id,
            properties: {
                dimensions: selection[0].dimensions,
                position: startPosition,
                rotation: startOrientation
            }
        });
        selection[0].dimensions = Vec3.multiplyVbyV(scaleFactor, selection[0].dimensions);
        selection[0].position = rootPosition;
        selection[0].rotation = rootOrientation;
        redoData.push({
            entityID: selection[0].id,
            properties: {
                dimensions: selection[0].dimensions,
                position: selection[0].position,
                rotation: selection[0].rotation
            }
        });

        // Final scale and position of children.
        for (i = 1, length = selection.length; i < length; i++) {
            undoData.push({
                entityID: selection[i].id,
                properties: {
                    dimensions: selection[i].dimensions,
                    localPosition: selection[i].localPosition,
                    localRotation: selection[i].localRotation
                }
            });
            selection[i].dimensions = Vec3.multiplyVbyV(scaleFactor, selection[i].dimensions);
            selection[i].localPosition = Vec3.multiplyVbyV(scaleFactor, selection[i].localPosition);
            redoData.push({
                entityID: selection[i].id,
                properties: {
                    dimensions: selection[i].dimensions,
                    localPosition: selection[i].localPosition,
                    localRotation: selection[i].localRotation
                }
            });
        }

        // Add history entry.
        History.push(side, { setProperties: undoData }, { setProperties: redoData });

        // Update grab start data for its undo.
        startPosition = rootPosition;
        startOrientation = rootOrientation;
    }

    function cloneEntities() {
        var parentIDIndexes = [],
            intersectedEntityIndex = 0,
            parentID,
            properties,
            undoData = [],
            redoData = [],
            i,
            j,
            length;

        // Map parent IDs; find intersectedEntityID's index.
        for (i = 1, length = selection.length; i < length; i++) {
            if (selection[i].id === intersectedEntityID) {
                intersectedEntityIndex = i;
            }
            parentID = selection[i].parentID;
            for (j = 0; j < i; j++) {
                if (parentID === selection[j].id) {
                    parentIDIndexes[i] = j;
                    break;
                }
            }
        }

        // Clone entities.
        for (i = 0, length = selection.length; i < length; i++) {
            properties = Entities.getEntityProperties(selection[i].id);
            if (i > 0) {
                properties.parentID = selection[parentIDIndexes[i]].id;
            }
            selection[i].id = Entities.addEntity(properties);
            undoData.push({ entityID: selection[i].id });
            redoData.push({ entityID: selection[i].id, properties: properties });
        }

        // Update selection info.
        intersectedEntityID = selection[intersectedEntityIndex].id;
        rootEntityID = selection[0].id;

        // Add history entry.
        History.prePush(side, { deleteEntities: undoData }, { createEntities: redoData });
    }

    function applyColor(color, isApplyToAll) {
        // Entities without a color property simply ignore the edit.
        var properties,
            isOK = false,
            undoData = [],
            redoData = [],
            i,
            length;

        if (isApplyToAll) {
            for (i = 0, length = selection.length; i < length; i++) {
                properties = Entities.getEntityProperties(selection[i].id, ["type", "color"]);
                if (ENTITY_TYPES_WITH_COLOR.indexOf(properties.type) !== -1) {
                    Entities.editEntity(selection[i].id, {
                        color: color
                    });
                    undoData.push({ entityID: intersectedEntityID, properties: { color: properties.color } });
                    redoData.push({ entityID: intersectedEntityID, properties: { color: color } });
                    isOK = true;
                }
            }
            if (undoData.length > 0) {
                History.push(side, { setProperties: undoData }, { setProperties: redoData });
            }
        } else {
            properties = Entities.getEntityProperties(intersectedEntityID, ["type", "color"]);
            if (ENTITY_TYPES_WITH_COLOR.indexOf(properties.type) !== -1) {
                Entities.editEntity(intersectedEntityID, {
                    color: color
                });
                History.push(
                    side,
                    { setProperties: [{ entityID: intersectedEntityID, properties: { color: properties.color } }] },
                    { setProperties: [{ entityID: intersectedEntityID, properties: { color: color } }] }
                );
                isOK = true;
            }
        }

        return isOK;
    }

    function getColor(entityID) {
        var properties;

        properties = Entities.getEntityProperties(entityID, "color");
        if (ENTITY_TYPES_WITH_COLOR.indexOf(properties.type) === -1) {
            // Some entities don't have a color property.
            return null;
        }

        return properties.color;
    }

    function updatePhysicsUserData(userDataString, physicsUserData) {
        var userData = {};

        if (userDataString !== "") {
            try {
                userData = JSON.parse(userDataString);
            } catch (e) {
                App.log(side, "ERROR: Invalid userData in entity being updated! " + userDataString);
            }
        }

        if (!userData.hasOwnProperty("grabbableKey")) {
            userData.grabbableKey = {};
        }
        userData.grabbableKey.grabbable = physicsUserData.grabbableKey.grabbable;

        return JSON.stringify(userData);
    }

    function applyPhysics(physicsProperties) {
        // Regarding trees of entities, when physics is to be enabled the physics engine currently:
        // - Only works with physics applied to the root entity; i.e., child entities are ignored for collisions.
        // - Requires child entities to be dynamic if the root entity is dynamic, otherwise child entities can drift.
        // - Requires child entities to be collisionless, otherwise the entity tree can become self-propelled.
        // See also: Groups.group() and ungroup().
        var properties,
            property,
            undoData = [],
            redoData = [],
            i,
            length;

        // Make children cater to physicsProperties.
        properties = {
            dynamic: physicsProperties.dynamic,
            collisionless: physicsProperties.dynamic || physicsProperties.collisionless
        };
        for (i = 1, length = selection.length; i < length; i++) {
            undoData.push({
                entityID: selection[i].id,
                properties: {
                    dynamic: selection[i].dynamic,
                    collisionless: selection[i].collisionless
                }
            });
            Entities.editEntity(selection[i].id, properties);
            undoData.push({
                entityID: selection[i].id,
                properties: properties
            });
        }

        // Undo data.
        properties = {
            position: selection[0].position,
            rotation: selection[0].rotation,
            velocity: Vec3.ZERO,
            angularVelocity: Vec3.ZERO
        };
        for (property in physicsProperties) {
            if (physicsProperties.hasOwnProperty(property)) {
                properties[property] = selectionProperties[0].properties[property];
            }
        }
        if (properties.userData === undefined) {
            properties.userData = "";
        }
        undoData.push({
            entityID: selection[0].id,
            properties: properties
        });

        // Set root per physicsProperties.
        properties = Object.clone(physicsProperties);
        properties.userData = updatePhysicsUserData(selection[intersectedEntityIndex].userData, physicsProperties.userData);
        Entities.editEntity(rootEntityID, properties);

        // Redo data.
        properties.position = selection[0].position;
        properties.rotation = selection[0].rotation;
        properties.velocity = Vec3.ZERO;
        properties.angularVelocity = Vec3.ZERO;
        redoData.push({
            entityID: selection[0].id,
            properties: properties
        });

        // Add history entry.
        History.push(side, { setProperties: undoData }, { setProperties: redoData });

        // Kick off physics if necessary.
        if (physicsProperties.dynamic) {
            kickPhysics(rootEntityID);
        }
    }

    function clear() {
        selection = [];
        intersectedEntityID = null;
        rootEntityID = null;
    }

    function deleteEntities() {
        if (rootEntityID) {
            History.push(side, { createEntities: selectionProperties }, { deleteEntities: [{ entityID: rootEntityID }] });
            Entities.deleteEntity(rootEntityID); // Children are automatically deleted.
            clear();
        }
    }

    function destroy() {
        clear();
    }

    return {
        select: select,
        append: append,
        selection: getSelection,
        contains: contains,
        count: count,
        intersectedEntityID: getIntersectedEntityID,
        intersectedEntityIndex: getIntersectedEntityIndex,
        rootEntityID: getRootEntityID,
        boundingBox: getBoundingBox,
        is2D: is2D,
        getPositionAndOrientation: getPositionAndOrientation,
        setPositionAndOrientation: setPositionAndOrientation,
        startEditing: startEditing,
        startDirectScaling: startDirectScaling,
        directScale: directScale,
        finishDirectScaling: finishDirectScaling,
        startHandleScaling: startHandleScaling,
        handleScale: handleScale,
        finishHandleScaling: finishHandleScaling,
        finishEditing: finishEditing,
        cloneEntities: cloneEntities,
        applyColor: applyColor,
        getColor: getColor,
        applyPhysics: applyPhysics,
        deleteEntities: deleteEntities,
        clear: clear,
        destroy: destroy
    };
};

SelectionManager.prototype = {};
