//
//  selection.js
//
//  Created by David Rowe on 21 Jul 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global Selection */

Selection = function (side) {
    // Manages set of selected entities. Currently supports just one set of linked entities.

    "use strict";

    var selection = [],
        selectedEntityID = null,
        rootEntityID = null,
        rootPosition,
        rootOrientation,
        scaleCenter,
        scaleRootOffset,
        scaleRootOrientation,
        ENTITY_TYPE = "entity";

    if (!this instanceof Selection) {
        return new Selection(side);
    }

    function traverseEntityTree(id, result) {
        // Recursively traverses tree of entities and their children, gather IDs and properties.
        var children,
            properties,
            SELECTION_PROPERTIES = ["position", "registrationPoint", "rotation", "dimensions", "localPosition",
                "dynamic", "collisionless"],
            i,
            length;

        properties = Entities.getEntityProperties(id, SELECTION_PROPERTIES);
        result.push({
            id: id,
            position: properties.position,
            localPosition: properties.localPosition,
            registrationPoint: properties.registrationPoint,
            rotation: properties.rotation,
            dimensions: properties.dimensions,
            dynamic: properties.dynamic,
            collisionless: properties.collisionless
        });

        children = Entities.getChildrenIDs(id);
        for (i = 0, length = children.length; i < length; i += 1) {
            if (Entities.getNestableType(children[i]) === ENTITY_TYPE) {
                traverseEntityTree(children[i], result);
            }
        }
    }

    function select(entityID) {
        var entityProperties,
            PARENT_PROPERTIES = ["parentID", "position", "rotation", "dymamic", "collisionless"];

        // Find root parent.
        rootEntityID = Entities.rootOf(entityID);

        // Selection position and orientation is that of the root entity.
        entityProperties = Entities.getEntityProperties(rootEntityID, PARENT_PROPERTIES);
        rootPosition = entityProperties.position;
        rootOrientation = entityProperties.rotation;

        // Find all children.
        selection = [];
        traverseEntityTree(rootEntityID, selection);

        selectedEntityID = entityID;
    }

    function getRootEntityID() {
        return rootEntityID;
    }

    function getSelection() {
        return selection;
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
            for (i = 1, length = selection.length; i < length; i += 1) {

                registration = selection[i].registrationPoint;
                corners[0] = { x: -registration.x, y: -registration.y, z: -registration.z };
                corners[1] = { x: -registration.x, y: -registration.y, z: 1.0 - registration.z };
                corners[2] = { x: -registration.x, y: 1.0 - registration.y, z: -registration.z };
                corners[3] = { x: -registration.x, y: 1.0 - registration.y, z: 1.0 - registration.z };
                corners[4] = { x: 1.0 - registration.x, y: -registration.y, z: -registration.z };
                corners[5] = { x: 1.0 - registration.x, y: -registration.y, z: 1.0 - registration.z };
                corners[6] = { x: 1.0 - registration.x, y: 1.0 - registration.y, z: -registration.z };
                corners[7] = { x: 1.0 - registration.x, y: 1.0 - registration.y, z: 1.0 - registration.z };

                position = selection[i].position;
                rotation = selection[i].rotation;
                dimensions = selection[i].dimensions;

                for (j = 0; j < NUM_CORNERS; j += 1) {
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

    function startEditing() {
        var count,
            i;

        // Disable entity set's physics.
        for (i = 0, count = selection.length; i < count; i += 1) {
            Entities.editEntity(selection[i].id, {
                dynamic: false,         // So that gravity doesn't fight with us trying to hold the entity in place.
                collisionless: true     // So that entity doesn't bump us about as we resize the entity.
            });
        }
    }

    function finishEditing() {
        var firstDynamicEntityID = null,
            properties,
            VELOCITY_THRESHOLD = 0.05,  // See EntityMotionState.cpp DYNAMIC_LINEAR_VELOCITY_THRESHOLD
            VELOCITY_KICK = { x: 0, y: 0.02, z: 0 },
            count,
            i;

        // Restore entity set's physics.
        for (i = 0, count = selection.length; i < count; i += 1) {
            if (firstDynamicEntityID === null && selection[i].dynamic) {
                firstDynamicEntityID = selection[i].id;
            }
            Entities.editEntity(selection[i].id, {
                dynamic: selection[i].dynamic,
                collisionless: selection[i].collisionless
            });
        }

        // If dynamic with gravity, and velocity is zero, give the entity set a little kick to set off physics.
        if (firstDynamicEntityID) {
            properties = Entities.getEntityProperties(firstDynamicEntityID, ["velocity", "gravity"]);
            if (Vec3.length(properties.gravity) > 0 && Vec3.length(properties.velocity) < VELOCITY_THRESHOLD) {
                Entities.editEntity(firstDynamicEntityID, { velocity: VELOCITY_KICK });
            }
        }
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
        scaleCenter = center;
        scaleRootOffset = Vec3.subtract(rootPosition, center);
        scaleRootOrientation = rootOrientation;
    }

    function directScale(factor, rotation, center) {
        // Scale, position, and rotate selection.
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
        for (i = 1, length = selection.length; i < length; i += 1) {
            Entities.editEntity(selection[i].id, {
                dimensions: Vec3.multiply(factor, selection[i].dimensions),
                localPosition: Vec3.multiply(factor, selection[i].localPosition)
            });
        }
    }

    function finishDirectScaling() {
        select(selectedEntityID);  // Refresh.
    }

    function startHandleScaling() {
        // Nothing to do.
    }

    function handleScale(factor, position, orientation) {
        // Scale and reposition and orient selection.
        var i,
            length;

        // Scale and position root.
        rootPosition = position;
        rootOrientation = orientation;
        Entities.editEntity(selection[0].id, {
            dimensions: Vec3.multiplyVbyV(factor, selection[0].dimensions),
            position: rootPosition,
            rotation: rootOrientation
        });

        // Scale and position children.
        // Only corner handles are used for scaling multiple entities so scale factor is the same in all dimensions.
        // Therefore don't need to take into account orientation relative to parent when scaling local position.
        for (i = 1, length = selection.length; i < length; i += 1) {
            Entities.editEntity(selection[i].id, {
                dimensions: Vec3.multiplyVbyV(factor, selection[i].dimensions),
                localPosition: Vec3.multiplyVbyV(factor, selection[i].localPosition)
            });
        }
    }

    function finishHandleScaling() {
        select(selectedEntityID);  // Refresh.
    }

    function clear() {
        selection = [];
        selectedEntityID = null;
        rootEntityID = null;
    }

    function deleteEntities() {
        if (rootEntityID) {
            Entities.deleteEntity(rootEntityID);  // Children are automatically deleted.
            clear();
        }
    }

    function destroy() {
        clear();
    }

    return {
        select: select,
        selection: getSelection,
        count: count,
        rootEntityID: getRootEntityID,
        boundingBox: getBoundingBox,
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
        deleteEntities: deleteEntities,
        clear: clear,
        destroy: destroy
    };
};

Selection.prototype = {};
