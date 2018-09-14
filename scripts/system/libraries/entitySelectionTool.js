//
//  entitySelectionTool.js
//  examples
//
//  Created by Brad hefta-Gaub on 10/1/14.
//    Modified by Daniela Fontes * @DanielaFifo and Tiago Andrade @TagoWill on 4/7/2017
//    Modified by David Back on 1/9/2018
//  Copyright 2014 High Fidelity, Inc.
//
//  This script implements a class useful for building tools for editing entities.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global SelectionManager, SelectionDisplay, grid, rayPlaneIntersection, rayPlaneIntersection2, pushCommandForSelections,
   getMainTabletIDs, getControllerWorldLocation, TRIGGER_ON_VALUE */

var SPACE_LOCAL = "local";
var SPACE_WORLD = "world";
var HIGHLIGHT_LIST_NAME = "editHandleHighlightList";

Script.include([
    "./controllers.js",
    "./controllerDispatcherUtils.js",
    "./utils.js"
]);

SelectionManager = (function() {
    var that = {};

    /**
     * @description Removes known to be broken properties from a properties object
     * @param properties
     * @return properties
     */
    var fixRemoveBrokenProperties = function (properties) {
        // Reason: Entity property is always set to 0,0,0 which causes it to override angularVelocity (see MS17131)
        delete properties.localAngularVelocity;
        return properties;
    }

    // FUNCTION: SUBSCRIBE TO UPDATE MESSAGES
    function subscribeToUpdateMessages() {
        Messages.subscribe("entityToolUpdates");
        Messages.messageReceived.connect(handleEntitySelectionToolUpdates);
    }

    // FUNCTION: HANDLE ENTITY SELECTION TOOL UDPATES
    function handleEntitySelectionToolUpdates(channel, message, sender) {
        if (channel !== 'entityToolUpdates') {
            return;
        }
        if (sender !== MyAvatar.sessionUUID) {
            return;
        }

        var wantDebug = false;
        var messageParsed;
        try {
            messageParsed = JSON.parse(message);
        } catch (err) {
            print("ERROR: entitySelectionTool.handleEntitySelectionToolUpdates - got malformed message: " + message);
            return;
        }

        if (messageParsed.method === "selectEntity") {
            if (!SelectionDisplay.triggered() || SelectionDisplay.triggeredHand === messageParsed.hand) {
                if (wantDebug) {
                    print("setting selection to " + messageParsed.entityID);
                }
                that.setSelections([messageParsed.entityID]);
            }
        } else if (messageParsed.method === "clearSelection") {
            if (!SelectionDisplay.triggered() || SelectionDisplay.triggeredHand === messageParsed.hand) {
                that.clearSelections();
            }
        } else if (messageParsed.method === "pointingAt") {
            if (messageParsed.hand === Controller.Standard.RightHand) {
                that.pointingAtDesktopWindowRight = messageParsed.desktopWindow;
                that.pointingAtTabletRight = messageParsed.tablet;
            } else {
                that.pointingAtDesktopWindowLeft = messageParsed.desktopWindow;
                that.pointingAtTabletLeft = messageParsed.tablet;
            }
        }
    }

    subscribeToUpdateMessages();

    // disabling this for now as it is causing rendering issues with the other handle overlays
    /*
    var COLOR_ORANGE_HIGHLIGHT = { red: 255, green: 99, blue: 9 };
    var editHandleOutlineStyle = {
        outlineUnoccludedColor: COLOR_ORANGE_HIGHLIGHT,
        outlineOccludedColor: COLOR_ORANGE_HIGHLIGHT,
        fillUnoccludedColor: COLOR_ORANGE_HIGHLIGHT,
        fillOccludedColor: COLOR_ORANGE_HIGHLIGHT,
        outlineUnoccludedAlpha: 1,
        outlineOccludedAlpha: 0,
        fillUnoccludedAlpha: 0,
        fillOccludedAlpha: 0,
        outlineWidth: 3,
        isOutlineSmooth: true
    };
    Selection.enableListHighlight(HIGHLIGHT_LIST_NAME, editHandleOutlineStyle);
    */

    that.savedProperties = {};
    that.selections = [];
    var listeners = [];

    that.localRotation = Quat.IDENTITY;
    that.localPosition = Vec3.ZERO;
    that.localDimensions = Vec3.ZERO;
    that.localRegistrationPoint = Vec3.HALF;

    that.worldRotation = Quat.IDENTITY;
    that.worldPosition = Vec3.ZERO;
    that.worldDimensions = Vec3.ZERO;
    that.worldRegistrationPoint = Vec3.HALF;
    that.centerPosition = Vec3.ZERO;
    
    that.pointingAtDesktopWindowLeft = false;
    that.pointingAtDesktopWindowRight = false;
    that.pointingAtTabletLeft = false;
    that.pointingAtTabletRight = false;

    that.saveProperties = function() {
        that.savedProperties = {};
        for (var i = 0; i < that.selections.length; i++) {
            var entityID = that.selections[i];
            that.savedProperties[entityID] = fixRemoveBrokenProperties(Entities.getEntityProperties(entityID));
        }
    };

    that.addEventListener = function(func) {
        listeners.push(func);
    };

    that.hasSelection = function() {
        return that.selections.length > 0;
    };

    that.setSelections = function(entityIDs) {
        that.selections = [];
        for (var i = 0; i < entityIDs.length; i++) {
            var entityID = entityIDs[i];
            that.selections.push(entityID);
            Selection.addToSelectedItemsList(HIGHLIGHT_LIST_NAME, "entity", entityID);
        }

        that._update(true);
    };

    that.addEntity = function(entityID, toggleSelection) {
        if (entityID) {
            var idx = -1;
            for (var i = 0; i < that.selections.length; i++) {
                if (entityID === that.selections[i]) {
                    idx = i;
                    break;
                }
            }
            if (idx === -1) {
                that.selections.push(entityID);
                Selection.addToSelectedItemsList(HIGHLIGHT_LIST_NAME, "entity", entityID);
            } else if (toggleSelection) {
                that.selections.splice(idx, 1);
                Selection.removeFromSelectedItemsList(HIGHLIGHT_LIST_NAME, "entity", entityID);
            }
        }

        that._update(true);
    };

    function removeEntityByID(entityID) {
        var idx = that.selections.indexOf(entityID);
        if (idx >= 0) {
            that.selections.splice(idx, 1);
            Selection.removeFromSelectedItemsList(HIGHLIGHT_LIST_NAME, "entity", entityID);
        }
    }

    that.removeEntity = function (entityID) {
        removeEntityByID(entityID);
        that._update(true);
    };

    that.removeEntities = function(entityIDs) {
        for (var i = 0, length = entityIDs.length; i < length; i++) {
            removeEntityByID(entityIDs[i]);
        }
        that._update(true);
    };

    that.clearSelections = function() {
        that.selections = [];
        that._update(true);
    };
    
    that.addChildrenEntities = function(parentEntityID, entityList) {
        var children = Entities.getChildrenIDs(parentEntityID);
        for (var i = 0; i < children.length; i++) {
            var childID = children[i];
            if (entityList.indexOf(childID) < 0) {
                entityList.push(childID);
            }
            that.addChildrenEntities(childID, entityList);
        }
    };

    // Return true if the given entity with `properties` is being grabbed by an avatar.
    // This is mostly a heuristic - there is no perfect way to know if an entity is being
    // grabbed.
    function nonDynamicEntityIsBeingGrabbedByAvatar(properties) {
        if (properties.dynamic || Uuid.isNull(properties.parentID)) {
            return false;
        }

        var avatar = AvatarList.getAvatar(properties.parentID);
        if (Uuid.isNull(avatar.sessionUUID)) {
            return false;
        }

        var grabJointNames = [
            'RightHand', 'LeftHand',
            '_CONTROLLER_RIGHTHAND', '_CONTROLLER_LEFTHAND',
            '_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND', '_CAMERA_RELATIVE_CONTROLLER_LEFTHAND'];

        for (var i = 0; i < grabJointNames.length; ++i) {
            if (avatar.getJointIndex(grabJointNames[i]) === properties.parentJointIndex) {
                return true;
            }
        }

        return false;
    }

    that.duplicateSelection = function() {
        var entitiesToDuplicate = [];
        var duplicatedEntityIDs = [];
        var duplicatedChildrenWithOldParents = [];
        var originalEntityToNewEntityID = [];

        SelectionManager.saveProperties();
        
        // build list of entities to duplicate by including any unselected children of selected parent entities
        Object.keys(that.savedProperties).forEach(function(originalEntityID) {
            if (entitiesToDuplicate.indexOf(originalEntityID) < 0) {
                entitiesToDuplicate.push(originalEntityID);
            }
            that.addChildrenEntities(originalEntityID, entitiesToDuplicate);
        });
        
        // duplicate entities from above and store their original to new entity mappings and children needing re-parenting
        for (var i = 0; i < entitiesToDuplicate.length; i++) {
            var originalEntityID = entitiesToDuplicate[i];
            var properties = that.savedProperties[originalEntityID];
            if (properties === undefined) {
                properties = fixRemoveBrokenProperties(Entities.getEntityProperties(originalEntityID));
            }
            if (!properties.locked && (!properties.clientOnly || properties.owningAvatarID === MyAvatar.sessionUUID)) {
                if (nonDynamicEntityIsBeingGrabbedByAvatar(properties)) {
                    properties.parentID = null;
                    properties.parentJointIndex = null;
                    properties.localPosition = properties.position;
                    properties.localRotation = properties.rotation;
                }
                delete properties.actionData;
                var newEntityID = Entities.addEntity(properties);

                // Re-apply actions from the original entity
                var actionIDs = Entities.getActionIDs(properties.id);
                for (var j = 0; j < actionIDs.length; ++j) {
                    var actionID = actionIDs[j];
                    var actionArguments = Entities.getActionArguments(properties.id, actionID);
                    if (actionArguments) {
                        var type = actionArguments.type;
                        if (type == 'hold' || type == 'far-grab') {
                            continue;
                        }
                        delete actionArguments.ttl;
                        Entities.addAction(type, newEntityID, actionArguments);
                    }
                }

                duplicatedEntityIDs.push({
                    entityID: newEntityID,
                    properties: properties
                });
                if (properties.parentID !== Uuid.NULL) {
                    duplicatedChildrenWithOldParents[newEntityID] = properties.parentID;
                }
                originalEntityToNewEntityID[originalEntityID] = newEntityID;
            }
        }
        
        // re-parent duplicated children to the duplicate entities of their original parents (if they were duplicated)
        Object.keys(duplicatedChildrenWithOldParents).forEach(function(childIDNeedingNewParent) {
            var originalParentID = duplicatedChildrenWithOldParents[childIDNeedingNewParent];
            var newParentID = originalEntityToNewEntityID[originalParentID];
            if (newParentID) {
                Entities.editEntity(childIDNeedingNewParent, { parentID: newParentID });
                for (var i = 0; i < duplicatedEntityIDs.length; i++) {
                    var duplicatedEntity = duplicatedEntityIDs[i];
                    if (duplicatedEntity.entityID === childIDNeedingNewParent) {
                        duplicatedEntity.properties.parentID = newParentID;
                    }
                }
            }
        });
        
        return duplicatedEntityIDs;
    };

    that._update = function(selectionUpdated) {
        var properties = null;
        if (that.selections.length === 0) {
            that.localDimensions = null;
            that.localPosition = null;
            that.worldDimensions = null;
            that.worldPosition = null;
            that.worldRotation = null;
        } else if (that.selections.length === 1) {
            properties = Entities.getEntityProperties(that.selections[0],
                ['dimensions', 'position', 'rotation', 'registrationPoint', 'boundingBox', 'type']);
            that.localDimensions = properties.dimensions;
            that.localPosition = properties.position;
            that.localRotation = properties.rotation;
            that.localRegistrationPoint = properties.registrationPoint;

            that.worldDimensions = properties.boundingBox.dimensions;
            that.worldPosition = properties.boundingBox.center;
            that.worldRotation = Quat.IDENTITY;

            that.entityType = properties.type;
            
            if (selectionUpdated) {
                SelectionDisplay.setSpaceMode(SPACE_LOCAL);
            }
        } else {
            properties = Entities.getEntityProperties(that.selections[0], ['type', 'boundingBox']);

            that.entityType = properties.type;

            var brn = properties.boundingBox.brn;
            var tfl = properties.boundingBox.tfl;

            for (var i = 1; i < that.selections.length; i++) {
                properties = Entities.getEntityProperties(that.selections[i], 'boundingBox');
                var bb = properties.boundingBox;
                brn.x = Math.min(bb.brn.x, brn.x);
                brn.y = Math.min(bb.brn.y, brn.y);
                brn.z = Math.min(bb.brn.z, brn.z);
                tfl.x = Math.max(bb.tfl.x, tfl.x);
                tfl.y = Math.max(bb.tfl.y, tfl.y);
                tfl.z = Math.max(bb.tfl.z, tfl.z);
            }

            that.localRotation = null;
            that.localDimensions = null;
            that.localPosition = null;
            that.worldDimensions = {
                x: tfl.x - brn.x,
                y: tfl.y - brn.y,
                z: tfl.z - brn.z
            };
            that.worldRotation = Quat.IDENTITY;
            that.worldPosition = {
                x: brn.x + (that.worldDimensions.x / 2),
                y: brn.y + (that.worldDimensions.y / 2),
                z: brn.z + (that.worldDimensions.z / 2)
            };

            // For 1+ selections we can only modify selections in world space
            SelectionDisplay.setSpaceMode(SPACE_WORLD);
        }

        for (var j = 0; j < listeners.length; j++) {
            try {
                listeners[j](selectionUpdated === true);
            } catch (e) {
                print("ERROR: entitySelectionTool.update got exception: " + JSON.stringify(e));
            }
        }
    };

    return that;
})();

// Normalize degrees to be in the range (-180, 180)
function normalizeDegrees(degrees) {
    var maxDegrees = 360;
    var halfMaxDegrees = maxDegrees / 2;
    degrees = ((degrees + halfMaxDegrees) % maxDegrees) - halfMaxDegrees;
    if (degrees <= -halfMaxDegrees) {
        degrees += maxDegrees;
    }
    return degrees;
}

// SELECTION DISPLAY DEFINITION
SelectionDisplay = (function() {
    var that = {};

    var NEGATE_VECTOR = -1;

    var COLOR_GREEN = { red: 31, green: 198, blue: 166 };
    var COLOR_BLUE = { red: 0, green: 147, blue: 197 };
    var COLOR_RED = { red: 226, green: 51, blue: 77 };
    var COLOR_HOVER = { red: 227, green: 227, blue: 227 };
    var COLOR_ROTATE_CURRENT_RING = { red: 255, green: 99, blue: 9 };
    var COLOR_BOUNDING_EDGE = { red: 87, green: 87, blue: 87 };
    var COLOR_SCALE_CUBE = { red: 106, green: 106, blue: 106 };
    var COLOR_SCALE_CUBE_SELECTED = { red: 18, green: 18, blue: 18 };

    var TRANSLATE_ARROW_CYLINDER_OFFSET = 0.1;
    var TRANSLATE_ARROW_CYLINDER_CAMERA_DISTANCE_MULTIPLE = 0.005;
    var TRANSLATE_ARROW_CYLINDER_Y_MULTIPLE = 7.5;
    var TRANSLATE_ARROW_CONE_CAMERA_DISTANCE_MULTIPLE = 0.025;
    var TRANSLATE_ARROW_CONE_OFFSET_CYLINDER_DIMENSION_MULTIPLE = 0.83;

    var ROTATE_RING_CAMERA_DISTANCE_MULTIPLE = 0.15;
    var ROTATE_CTRL_SNAP_ANGLE = 22.5;
    var ROTATE_DEFAULT_SNAP_ANGLE = 1;
    var ROTATE_DEFAULT_TICK_MARKS_ANGLE = 5;
    var ROTATE_RING_IDLE_INNER_RADIUS = 0.92;
    var ROTATE_RING_SELECTED_INNER_RADIUS = 0.9;

    // These are multipliers for sizing the rotation degrees display while rotating an entity
    var ROTATE_DISPLAY_DISTANCE_MULTIPLIER = 2;
    var ROTATE_DISPLAY_SIZE_X_MULTIPLIER = 0.2;
    var ROTATE_DISPLAY_SIZE_Y_MULTIPLIER = 0.09;
    var ROTATE_DISPLAY_LINE_HEIGHT_MULTIPLIER = 0.07;

    var STRETCH_CUBE_OFFSET = 0.06;
    var STRETCH_CUBE_CAMERA_DISTANCE_MULTIPLE = 0.02;
    var STRETCH_MINIMUM_DIMENSION = 0.001;
    var STRETCH_ALL_MINIMUM_DIMENSION = 0.01;
    var STRETCH_DIRECTION_ALL_CAMERA_DISTANCE_MULTIPLE = 2;
    var STRETCH_PANEL_WIDTH = 0.01;

    var BOUNDING_EDGE_OFFSET = 0.5;
    var SCALE_CUBE_CAMERA_DISTANCE_MULTIPLE = 0.02;

    var CLONER_OFFSET = { x: 0.9, y: -0.9, z: 0.9 };    
    
    var CTRL_KEY_CODE = 16777249;

    var RAIL_AXIS_LENGTH = 10000;
        
    var NO_HAND = -1;

    var TRANSLATE_DIRECTION = {
        X: 0,
        Y: 1,
        Z: 2
    };

    var STRETCH_DIRECTION = {
        X: 0,
        Y: 1,
        Z: 2,
        ALL: 3
    };

    var SCALE_DIRECTION = {
        LBN: 0,
        RBN: 1,
        LBF: 2,
        RBF: 3,
        LTN: 4,
        RTN: 5,
        LTF: 6,
        RTF: 7
    };

    var ROTATE_DIRECTION = {
        PITCH: 0,
        YAW: 1,
        ROLL: 2
    };

    var spaceMode = SPACE_LOCAL;
    var overlayNames = [];
    var lastControllerPoses = [
        getControllerWorldLocation(Controller.Standard.LeftHand, true),
        getControllerWorldLocation(Controller.Standard.RightHand, true)
    ];

    var rotationZero;
    var rotationNormal;
    var rotationDegreesPosition;

    var worldRotationX;
    var worldRotationY;
    var worldRotationZ;
    
    var activeStretchCubePanelOffset = null;

    var previousHandle = null;
    var previousHandleHelper = null;
    var previousHandleColor;

    var ctrlPressed = false;

    that.replaceCollisionsAfterStretch = false;

    var handlePropertiesTranslateArrowCones = {
        alpha: 1,
        shape: "Cone",
        solid: true,
        visible: false,
        ignoreRayIntersection: false,
        drawInFront: true
    };
    var handlePropertiesTranslateArrowCylinders = {
        alpha: 1,
        shape: "Cylinder",
        solid: true,
        visible: false,
        ignoreRayIntersection: false,
        drawInFront: true
    };
    var handleTranslateXCone = Overlays.addOverlay("shape", handlePropertiesTranslateArrowCones);
    var handleTranslateXCylinder = Overlays.addOverlay("shape", handlePropertiesTranslateArrowCylinders);
    Overlays.editOverlay(handleTranslateXCone, { color: COLOR_RED });
    Overlays.editOverlay(handleTranslateXCylinder, { color: COLOR_RED });
    var handleTranslateYCone = Overlays.addOverlay("shape", handlePropertiesTranslateArrowCones);
    var handleTranslateYCylinder = Overlays.addOverlay("shape", handlePropertiesTranslateArrowCylinders);
    Overlays.editOverlay(handleTranslateYCone, { color: COLOR_GREEN });
    Overlays.editOverlay(handleTranslateYCylinder, { color: COLOR_GREEN });
    var handleTranslateZCone = Overlays.addOverlay("shape", handlePropertiesTranslateArrowCones);
    var handleTranslateZCylinder = Overlays.addOverlay("shape", handlePropertiesTranslateArrowCylinders);
    Overlays.editOverlay(handleTranslateZCone, { color: COLOR_BLUE });
    Overlays.editOverlay(handleTranslateZCylinder, { color: COLOR_BLUE });

    var handlePropertiesRotateRings = {
        alpha: 1,
        solid: true,
        startAt: 0,
        endAt: 360,
        innerRadius: ROTATE_RING_IDLE_INNER_RADIUS,
        majorTickMarksAngle: ROTATE_DEFAULT_TICK_MARKS_ANGLE,
        majorTickMarksLength: 0.1,
        visible: false,
        ignoreRayIntersection: false,
        drawInFront: true
    };
    var handleRotatePitchRing = Overlays.addOverlay("circle3d", handlePropertiesRotateRings);
    Overlays.editOverlay(handleRotatePitchRing, { 
        color: COLOR_RED,
        majorTickMarksColor: COLOR_RED
    });
    var handleRotateYawRing = Overlays.addOverlay("circle3d", handlePropertiesRotateRings);
    Overlays.editOverlay(handleRotateYawRing, { 
        color: COLOR_GREEN,
        majorTickMarksColor: COLOR_GREEN
    });
    var handleRotateRollRing = Overlays.addOverlay("circle3d", handlePropertiesRotateRings);
    Overlays.editOverlay(handleRotateRollRing, { 
        color: COLOR_BLUE,
        majorTickMarksColor: COLOR_BLUE
    });

    var handleRotateCurrentRing = Overlays.addOverlay("circle3d", {
        alpha: 1,
        color: COLOR_ROTATE_CURRENT_RING,
        solid: true,
        innerRadius: 0.9,
        visible: false,
        ignoreRayIntersection: true,
        drawInFront: true
    });

    var rotationDegreesDisplay = Overlays.addOverlay("text3d", {
        text: "",
        color: { red: 0, green: 0, blue: 0 },
        backgroundColor: { red: 255, green: 255, blue: 255 },
        alpha: 0.7,
        backgroundAlpha: 0.7,
        visible: false,
        isFacingAvatar: true,
        drawInFront: true,
        ignoreRayIntersection: true,
        dimensions: { x: 0, y: 0 },
        lineHeight: 0.0,
        topMargin: 0,
        rightMargin: 0,
        bottomMargin: 0,
        leftMargin: 0
    });

    var handlePropertiesStretchCubes = {
        solid: true,
        visible: false,
        ignoreRayIntersection: false,
        drawInFront: true
    };
    var handleStretchXCube = Overlays.addOverlay("cube", handlePropertiesStretchCubes);
    Overlays.editOverlay(handleStretchXCube, { color: COLOR_RED });
    var handleStretchYCube = Overlays.addOverlay("cube", handlePropertiesStretchCubes);
    Overlays.editOverlay(handleStretchYCube, { color: COLOR_GREEN });
    var handleStretchZCube = Overlays.addOverlay("cube", handlePropertiesStretchCubes);
    Overlays.editOverlay(handleStretchZCube, { color: COLOR_BLUE });

    var handlePropertiesStretchPanel = {
        shape: "Quad",
        alpha: 0.5,
        solid: true,
        visible: false,
        ignoreRayIntersection: true,
        drawInFront: true
    };
    var handleStretchXPanel = Overlays.addOverlay("shape", handlePropertiesStretchPanel);
    Overlays.editOverlay(handleStretchXPanel, { color: COLOR_RED });
    var handleStretchYPanel = Overlays.addOverlay("shape", handlePropertiesStretchPanel);
    Overlays.editOverlay(handleStretchYPanel, { color: COLOR_GREEN });
    var handleStretchZPanel = Overlays.addOverlay("shape", handlePropertiesStretchPanel);
    Overlays.editOverlay(handleStretchZPanel, { color: COLOR_BLUE });

    var handleScaleCube = Overlays.addOverlay("cube", {
        size: 0.025,
        color: COLOR_SCALE_CUBE,
        solid: true,
        visible: false,
        ignoreRayIntersection: false,
        drawInFront: true,
        borderSize: 1.4
    });

    var handlePropertiesBoundingEdge = {
        alpha: 1,
        color: COLOR_BOUNDING_EDGE,
        visible: false,
        ignoreRayIntersection: true,
        drawInFront: true,
        lineWidth: 0.2
    };
    var handleBoundingTREdge = Overlays.addOverlay("line3d", handlePropertiesBoundingEdge);
    var handleBoundingTLEdge = Overlays.addOverlay("line3d", handlePropertiesBoundingEdge);
    var handleBoundingTFEdge = Overlays.addOverlay("line3d", handlePropertiesBoundingEdge);
    var handleBoundingTNEdge = Overlays.addOverlay("line3d", handlePropertiesBoundingEdge);
    var handleBoundingBREdge = Overlays.addOverlay("line3d", handlePropertiesBoundingEdge);
    var handleBoundingBLEdge = Overlays.addOverlay("line3d", handlePropertiesBoundingEdge);
    var handleBoundingBFEdge = Overlays.addOverlay("line3d", handlePropertiesBoundingEdge);
    var handleBoundingBNEdge = Overlays.addOverlay("line3d", handlePropertiesBoundingEdge);
    var handleBoundingNREdge = Overlays.addOverlay("line3d", handlePropertiesBoundingEdge);
    var handleBoundingNLEdge = Overlays.addOverlay("line3d", handlePropertiesBoundingEdge);
    var handleBoundingFREdge = Overlays.addOverlay("line3d", handlePropertiesBoundingEdge);
    var handleBoundingFLEdge = Overlays.addOverlay("line3d", handlePropertiesBoundingEdge);

    var handleCloner = Overlays.addOverlay("cube", {
        alpha: 1,
        size: 0.05,
        color: COLOR_GREEN,
        solid: true,
        visible: false,
        ignoreRayIntersection: false,
        drawInFront: true,
        borderSize: 1.4
    });

    // setting to 0 alpha for now to keep this hidden vs using visible false 
    // because its used as the translate xz tool handle overlay
    var selectionBox = Overlays.addOverlay("cube", {
        size: 1,
        color: COLOR_RED,
        alpha: 0,
        solid: false,
        visible: false,
        dashed: false
    });

    // Handle for x-z translation of particle effect and light entities while inside the bounding box.
    // Limitation: If multiple entities are selected, only the first entity's icon translates the selection.
    var iconSelectionBox = Overlays.addOverlay("cube", {
        size: 0.3, // Match entity icon size.
        color: COLOR_RED,
        alpha: 0,
        solid: false,
        visible: false,
        dashed: false
    });

    var xRailOverlay = Overlays.addOverlay("line3d", {
        visible: false,
        start: Vec3.ZERO,
        end: Vec3.ZERO,
        color: {
            red: 255,
            green: 0,
            blue: 0
        },
        ignoreRayIntersection: true // always ignore this
    });
    var yRailOverlay = Overlays.addOverlay("line3d", {
        visible: false,
        start: Vec3.ZERO,
        end: Vec3.ZERO,
        color: {
            red: 0,
            green: 255,
            blue: 0
        },
        ignoreRayIntersection: true // always ignore this
    });
    var zRailOverlay = Overlays.addOverlay("line3d", {
        visible: false,
        start: Vec3.ZERO,
        end: Vec3.ZERO,
        color: {
            red: 0,
            green: 0,
            blue: 255
        },
        ignoreRayIntersection: true // always ignore this
});

    var allOverlays = [
        handleTranslateXCone,
        handleTranslateXCylinder,
        handleTranslateYCone,
        handleTranslateYCylinder,
        handleTranslateZCone,
        handleTranslateZCylinder,
        handleRotatePitchRing,
        handleRotateYawRing,
        handleRotateRollRing,
        handleRotateCurrentRing,
        rotationDegreesDisplay,
        handleStretchXCube,
        handleStretchYCube,
        handleStretchZCube,
        handleStretchXPanel,
        handleStretchYPanel,
        handleStretchZPanel,
        handleScaleCube,
        handleBoundingTREdge,
        handleBoundingTLEdge,
        handleBoundingTFEdge,
        handleBoundingTNEdge,
        handleBoundingBREdge,
        handleBoundingBLEdge,
        handleBoundingBFEdge,
        handleBoundingBNEdge,
        handleBoundingNREdge,
        handleBoundingNLEdge,
        handleBoundingFREdge,
        handleBoundingFLEdge,
        handleCloner,
        selectionBox,
        iconSelectionBox,
        xRailOverlay,
        yRailOverlay,
        zRailOverlay

    ];
    var maximumHandleInAllOverlays = handleCloner;

    overlayNames[handleTranslateXCone] = "handleTranslateXCone";
    overlayNames[handleTranslateXCylinder] = "handleTranslateXCylinder";
    overlayNames[handleTranslateYCone] = "handleTranslateYCone";
    overlayNames[handleTranslateYCylinder] = "handleTranslateYCylinder";
    overlayNames[handleTranslateZCone] = "handleTranslateZCone";
    overlayNames[handleTranslateZCylinder] = "handleTranslateZCylinder";

    overlayNames[handleRotatePitchRing] = "handleRotatePitchRing";
    overlayNames[handleRotateYawRing] = "handleRotateYawRing";
    overlayNames[handleRotateRollRing] = "handleRotateRollRing";
    overlayNames[handleRotateCurrentRing] = "handleRotateCurrentRing";
    overlayNames[rotationDegreesDisplay] = "rotationDegreesDisplay";

    overlayNames[handleStretchXCube] = "handleStretchXCube";
    overlayNames[handleStretchYCube] = "handleStretchYCube";
    overlayNames[handleStretchZCube] = "handleStretchZCube";
    overlayNames[handleStretchXPanel] = "handleStretchXPanel";
    overlayNames[handleStretchYPanel] = "handleStretchYPanel";
    overlayNames[handleStretchZPanel] = "handleStretchZPanel";

    overlayNames[handleScaleCube] = "handleScaleCube";

    overlayNames[handleBoundingTREdge] = "handleBoundingTREdge";
    overlayNames[handleBoundingTLEdge] = "handleBoundingTLEdge";
    overlayNames[handleBoundingTFEdge] = "handleBoundingTFEdge";
    overlayNames[handleBoundingTNEdge] = "handleBoundingTNEdge";
    overlayNames[handleBoundingBREdge] = "handleBoundingBREdge";
    overlayNames[handleBoundingBLEdge] = "handleBoundingBLEdge";
    overlayNames[handleBoundingBFEdge] = "handleBoundingBFEdge";
    overlayNames[handleBoundingBNEdge] = "handleBoundingBNEdge";
    overlayNames[handleBoundingNREdge] = "handleBoundingNREdge";
    overlayNames[handleBoundingNLEdge] = "handleBoundingNLEdge";
    overlayNames[handleBoundingFREdge] = "handleBoundingFREdge";
    overlayNames[handleBoundingFLEdge] = "handleBoundingFLEdge";

    overlayNames[handleCloner] = "handleCloner";
    overlayNames[selectionBox] = "selectionBox";
    overlayNames[iconSelectionBox] = "iconSelectionBox";

    var activeTool = null;
    var handleTools = {};

    // We get mouseMoveEvents from the handControllers, via handControllerPointer.
    // But we dont' get mousePressEvents.
    that.triggerClickMapping = Controller.newMapping(Script.resolvePath('') + '-click');
    that.triggerPressMapping = Controller.newMapping(Script.resolvePath('') + '-press');
    that.triggeredHand = NO_HAND;
    that.pressedHand = NO_HAND;
    that.triggered = function() {
        return that.triggeredHand !== NO_HAND;
    }
    function pointingAtDesktopWindowOrTablet(hand) {
        var pointingAtDesktopWindow = (hand === Controller.Standard.RightHand && 
                                       SelectionManager.pointingAtDesktopWindowRight) ||
                                      (hand === Controller.Standard.LeftHand && 
                                       SelectionManager.pointingAtDesktopWindowLeft);
        var pointingAtTablet = (hand === Controller.Standard.RightHand && SelectionManager.pointingAtTabletRight) ||
                               (hand === Controller.Standard.LeftHand && SelectionManager.pointingAtTabletLeft);
        return pointingAtDesktopWindow || pointingAtTablet;
    }
    function makeClickHandler(hand) {
        return function (clicked) {
            // Don't allow both hands to trigger at the same time
            if (that.triggered() && hand !== that.triggeredHand) {
                return;
            }
            if (!that.triggered() && clicked && !pointingAtDesktopWindowOrTablet(hand)) {
                that.triggeredHand = hand;
                that.mousePressEvent({});
            } else if (that.triggered() && !clicked) {
                that.triggeredHand = NO_HAND;
                that.mouseReleaseEvent({});
            }
        };
    }
    function makePressHandler(hand) {
        return function (value) {
            if (value >= TRIGGER_ON_VALUE && !that.triggered() && !pointingAtDesktopWindowOrTablet(hand)) {
                that.pressedHand = hand;
                that.updateHighlight({});
            } else {
                that.pressedHand = NO_HAND;
                that.resetPreviousHandleColor();
            }
        }
    }
    that.triggerClickMapping.from(Controller.Standard.RTClick).peek().to(makeClickHandler(Controller.Standard.RightHand));
    that.triggerClickMapping.from(Controller.Standard.LTClick).peek().to(makeClickHandler(Controller.Standard.LeftHand));
    that.triggerPressMapping.from(Controller.Standard.RT).peek().to(makePressHandler(Controller.Standard.RightHand));
    that.triggerPressMapping.from(Controller.Standard.LT).peek().to(makePressHandler(Controller.Standard.LeftHand));
    that.enableTriggerMapping = function() {
        that.triggerClickMapping.enable();
        that.triggerPressMapping.enable();
    };
    that.disableTriggerMapping = function() {
        that.triggerClickMapping.disable();
        that.triggerPressMapping.disable();
    }
    Script.scriptEnding.connect(that.disableTriggerMapping);

    // FUNCTION DEF(s): Intersection Check Helpers
    function testRayIntersect(queryRay, overlayIncludes, overlayExcludes) {
        var wantDebug = false;
        if ((queryRay === undefined) || (queryRay === null)) {
            if (wantDebug) {
                print("testRayIntersect - EARLY EXIT -> queryRay is undefined OR null!");
            }
            return null;
        }

        var intersectObj = Overlays.findRayIntersection(queryRay, true, overlayIncludes, overlayExcludes);

        if (wantDebug) {
            if (!overlayIncludes) {
                print("testRayIntersect - no overlayIncludes provided.");
            }
            if (!overlayExcludes) {
                print("testRayIntersect - no overlayExcludes provided.");
            }
            print("testRayIntersect - Hit: " + intersectObj.intersects);
            print("    intersectObj.overlayID:" + intersectObj.overlayID + "[" + overlayNames[intersectObj.overlayID] + "]");
            print("        OverlayName: " + overlayNames[intersectObj.overlayID]);
            print("    intersectObj.distance:" + intersectObj.distance);
            print("    intersectObj.face:" + intersectObj.face);
            Vec3.print("    intersectObj.intersection:", intersectObj.intersection);
        }

        return intersectObj;
    }

    function isPointInsideBox(point, box) {
        var position = Vec3.subtract(point, box.position);
        position = Vec3.multiplyQbyV(Quat.inverse(box.rotation), position);
        return Math.abs(position.x) <= box.dimensions.x / 2 && Math.abs(position.y) <= box.dimensions.y / 2
            && Math.abs(position.z) <= box.dimensions.z / 2;
    }
    
    that.isEditHandle = function(overlayID) {
        var overlayIndex = allOverlays.indexOf(overlayID);
        var maxHandleIndex = allOverlays.indexOf(maximumHandleInAllOverlays);
        return overlayIndex >= 0 && overlayIndex <= maxHandleIndex;
    };

    // FUNCTION: MOUSE PRESS EVENT
    that.mousePressEvent = function (event) {
        var wantDebug = false;
        if (wantDebug) {
            print("=============== eST::MousePressEvent BEG =======================");
        }
        if (!event.isLeftButton && !that.triggered()) {
            // EARLY EXIT-(if another mouse button than left is pressed ignore it)
            return false;
        }

        var pickRay = generalComputePickRay(event.x, event.y);
        // TODO_Case6491:  Move this out to setup just to make it once
        var interactiveOverlays = getMainTabletIDs();
        for (var key in handleTools) {
            if (handleTools.hasOwnProperty(key)) {
                interactiveOverlays.push(key);
            }
        }

        // Start with unknown mode, in case no tool can handle this.
        activeTool = null;

        var results = testRayIntersect(pickRay, interactiveOverlays);
        if (results.intersects) {
            var hitOverlayID = results.overlayID;
            if ((HMD.tabletID && hitOverlayID === HMD.tabletID) || (HMD.tabletScreenID && hitOverlayID === HMD.tabletScreenID)
                || (HMD.homeButtonID && hitOverlayID === HMD.homeButtonID)) {
                // EARLY EXIT-(mouse clicks on the tablet should override the edit affordances)
                return false;
            }

            var hitTool = handleTools[ hitOverlayID ];
            if (hitTool) {
                activeTool = hitTool;
                if (activeTool.onBegin) {
                    activeTool.onBegin(event, pickRay, results);
                } else {
                    print("ERROR: entitySelectionTool.mousePressEvent - ActiveTool(" + activeTool.mode + ") missing onBegin");
                }
            } else {
                print("ERROR: entitySelectionTool.mousePressEvent - Hit unexpected object, check interactiveOverlays");
            }// End_if (hitTool)
        }// End_If(results.intersects)

        if (wantDebug) {
            print("    DisplayMode: " + getMode());
            print("=============== eST::MousePressEvent END =======================");
        }

        // If mode is known then we successfully handled this;
        // otherwise, we're missing a tool.
        return activeTool;
    };

    that.resetPreviousHandleColor = function() {
        if (previousHandle !== null) {
            Overlays.editOverlay(previousHandle, { color: previousHandleColor });
            previousHandle = null;
        }
        if (previousHandleHelper !== null) {
            Overlays.editOverlay(previousHandleHelper, { color: previousHandleColor });
            previousHandleHelper = null;
        }
    };

    that.getHandleHelper = function(overlay) {
        if (overlay === handleTranslateXCone) {
            return handleTranslateXCylinder;
        } else if (overlay === handleTranslateXCylinder) {
            return handleTranslateXCone;
        } else if (overlay === handleTranslateYCone) {
            return handleTranslateYCylinder;
        } else if (overlay === handleTranslateYCylinder) {
            return handleTranslateYCone;
        } else if (overlay === handleTranslateZCone) {
            return handleTranslateZCylinder;
        } else if (overlay === handleTranslateZCylinder) {
            return handleTranslateZCone;
        }
        return Uuid.NULL;
    };
    
    that.updateHighlight = function(event) {
        // if no tool is active, then just look for handles to highlight...
        var pickRay = generalComputePickRay(event.x, event.y);        
        var result = Overlays.findRayIntersection(pickRay);
        var pickedColor;
        var highlightNeeded = false;

        if (result.intersects) {
            switch (result.overlayID) {
                case handleTranslateXCone:
                case handleTranslateXCylinder:
                case handleRotatePitchRing:
                case handleStretchXCube:
                    pickedColor = COLOR_RED;
                    highlightNeeded = true;
                    break;
                case handleTranslateYCone:
                case handleTranslateYCylinder:
                case handleRotateYawRing:
                case handleStretchYCube:
                    pickedColor = COLOR_GREEN;
                    highlightNeeded = true;
                    break;
                case handleTranslateZCone:
                case handleTranslateZCylinder:
                case handleRotateRollRing:
                case handleStretchZCube:
                    pickedColor = COLOR_BLUE;
                    highlightNeeded = true;
                    break;
                case handleScaleCube:
                    pickedColor = COLOR_SCALE_CUBE;
                    highlightNeeded = true;
                    break;
                default:
                    that.resetPreviousHandleColor();
                    break;
            }

            if (highlightNeeded) {
                that.resetPreviousHandleColor();
                Overlays.editOverlay(result.overlayID, { color: COLOR_HOVER });
                previousHandle = result.overlayID;
                previousHandleHelper = that.getHandleHelper(result.overlayID);
                if (previousHandleHelper !== null) {
                    Overlays.editOverlay(previousHandleHelper, { color: COLOR_HOVER });
                }
                previousHandleColor = pickedColor;
            }

        } else {
            that.resetPreviousHandleColor();
        }
    };

    // FUNCTION: MOUSE MOVE EVENT
    var lastMouseEvent = null;
    that.mouseMoveEvent = function(event) {
        var wantDebug = false;
        if (wantDebug) {
            print("=============== eST::MouseMoveEvent BEG =======================");
        }
        lastMouseEvent = event;
        if (activeTool) {
            if (wantDebug) {
                print("    Trigger ActiveTool(" + activeTool.mode + ")'s onMove");
            }
            activeTool.onMove(event);

            if (wantDebug) {
                print("    Trigger SelectionManager::update");
            }
            SelectionManager._update();

            if (wantDebug) {
                print("=============== eST::MouseMoveEvent END =======================");
            }
            // EARLY EXIT--(Move handled via active tool)
            return true;
        }

        that.updateHighlight(event);
        
        if (wantDebug) {
            print("=============== eST::MouseMoveEvent END =======================");
        }
        return false;
    };

    // FUNCTION: MOUSE RELEASE EVENT
    that.mouseReleaseEvent = function(event) {
        var wantDebug = false;
        if (wantDebug) {
            print("=============== eST::MouseReleaseEvent BEG =======================");
        }
        var showHandles = false;
        if (activeTool) {
            if (activeTool.onEnd) {
                if (wantDebug) {
                    print("    Triggering ActiveTool(" + activeTool.mode + ")'s onEnd");
                }
                activeTool.onEnd(event);
            } else if (wantDebug) {
                print("    ActiveTool(" + activeTool.mode + ")'s missing onEnd");
            }
        }

        showHandles = activeTool; // base on prior tool value
        activeTool = null;

        // if something is selected, then reset the "original" properties for any potential next click+move operation
        if (SelectionManager.hasSelection()) {
            if (showHandles) {
                if (wantDebug) {
                    print("    Triggering that.select");
                }
                that.select(SelectionManager.selections[0], event);
            }
        }

        if (wantDebug) {
            print("=============== eST::MouseReleaseEvent END =======================");
        }
    };

    // Control key remains active only while key is held down
    that.keyReleaseEvent = function(event) {
        if (event.key === CTRL_KEY_CODE) {
            ctrlPressed = false;
            that.updateActiveRotateRing();
        }
        if (activeTool && lastMouseEvent !== null) {
            lastMouseEvent.isShifted = event.isShifted;
            lastMouseEvent.isMeta = event.isMeta;
            lastMouseEvent.isControl = event.isControl;
            lastMouseEvent.isAlt = event.isAlt;
            activeTool.onMove(lastMouseEvent);
            SelectionManager._update();
        }
    };

    // Triggers notification on specific key driven events
    that.keyPressEvent = function(event) {
        if (event.key === CTRL_KEY_CODE) {
            ctrlPressed = true;
            that.updateActiveRotateRing();
        }
        if (activeTool && lastMouseEvent !== null) {
            lastMouseEvent.isShifted = event.isShifted;
            lastMouseEvent.isMeta = event.isMeta;
            lastMouseEvent.isControl = event.isControl;
            lastMouseEvent.isAlt = event.isAlt;
            activeTool.onMove(lastMouseEvent);
            SelectionManager._update();
        }
    };

    // NOTE: mousePressEvent and mouseMoveEvent from the main script should call us., so we don't hook these:
    //       Controller.mousePressEvent.connect(that.mousePressEvent);
    //       Controller.mouseMoveEvent.connect(that.mouseMoveEvent);
    Controller.mouseReleaseEvent.connect(that.mouseReleaseEvent);
    Controller.keyPressEvent.connect(that.keyPressEvent);
    Controller.keyReleaseEvent.connect(that.keyReleaseEvent);

    that.checkControllerMove = function() {
        if (SelectionManager.hasSelection()) {
            var controllerPose = getControllerWorldLocation(that.triggeredHand, true);
            var hand = (that.triggeredHand === Controller.Standard.LeftHand) ? 0 : 1;
            if (controllerPose.valid && lastControllerPoses[hand].valid && that.triggered()) {
                if (!Vec3.equal(controllerPose.position, lastControllerPoses[hand].position) ||
                    !Vec3.equal(controllerPose.rotation, lastControllerPoses[hand].rotation)) {
                    that.mouseMoveEvent({});
                }
            }
            lastControllerPoses[hand] = controllerPose;
        }
    };

    function controllerComputePickRay(hand) {
        var hand = that.triggered() ? that.triggeredHand : that.pressedHand;
        var controllerPose = getControllerWorldLocation(hand, true);
        if (controllerPose.valid) {
            var controllerPosition = controllerPose.translation;
            // This gets point direction right, but if you want general quaternion it would be more complicated:
            var controllerDirection = Quat.getUp(controllerPose.rotation);
            return {origin: controllerPosition, direction: controllerDirection};
        }
    }

    function generalComputePickRay(x, y) {
        return controllerComputePickRay() || Camera.computePickRay(x, y);
    }
    
    function getControllerAvatarFramePositionFromPickRay(pickRay) {
        var controllerPosition = Vec3.subtract(pickRay.origin, MyAvatar.position);
        controllerPosition = Vec3.multiplyQbyV(Quat.inverse(MyAvatar.orientation), controllerPosition);
        return controllerPosition;
    }

    function getDistanceToCamera(position) {
        var cameraPosition = Camera.getPosition();
        var toCameraDistance = Vec3.length(Vec3.subtract(cameraPosition, position));
        return toCameraDistance;
    }
    
    function usePreviousPickRay(pickRayDirection, previousPickRayDirection, normal) {
        return (Vec3.dot(pickRayDirection, normal) > 0 && Vec3.dot(previousPickRayDirection, normal) < 0) ||
               (Vec3.dot(pickRayDirection, normal) < 0 && Vec3.dot(previousPickRayDirection, normal) > 0);
    }

    // @return string - The mode of the currently active tool;
    //                  otherwise, "UNKNOWN" if there's no active tool.
    function getMode() {
        return (activeTool ? activeTool.mode : "UNKNOWN");
    }

    that.cleanup = function() {
        for (var i = 0; i < allOverlays.length; i++) {
            Overlays.deleteOverlay(allOverlays[i]);
        }
    };

    that.select = function(entityID, event) {
        var properties = Entities.getEntityProperties(SelectionManager.selections[0]);

        if (event !== false) {
            var wantDebug = false;
            if (wantDebug) {
                print("select() with EVENT...... ");
                print("                event.y:" + event.y);
                Vec3.print("       current position:", properties.position);
            }
        }

        that.updateHandles();
    };

    // FUNCTION: SET SPACE MODE
    that.setSpaceMode = function(newSpaceMode) {
        var wantDebug = false;
        if (wantDebug) {
            print("======> SetSpaceMode called. ========");
        }

        if (spaceMode !== newSpaceMode) {
            if (wantDebug) {
                print("    Updating SpaceMode From: " + spaceMode + " To: " + newSpaceMode);
            }
            spaceMode = newSpaceMode;
            that.updateHandles();
        } else if (wantDebug) {
            print("WARNING: entitySelectionTool.setSpaceMode - Can't update SpaceMode. CurrentMode: " + 
                  spaceMode + " DesiredMode: " + newSpaceMode);
        }
        if (wantDebug) {
            print("====== SetSpaceMode called. <========");
        }
    };

    // FUNCTION: TOGGLE SPACE MODE
    that.toggleSpaceMode = function() {
        var wantDebug = false;
        if (wantDebug) {
            print("========> ToggleSpaceMode called. =========");
        }
        if ((spaceMode === SPACE_WORLD) && (SelectionManager.selections.length > 1)) {
            if (wantDebug) {
                print("Local space editing is not available with multiple selections");
            }
            return;
        }
        if (wantDebug) {
            print("PreToggle: " + spaceMode);
        }
        spaceMode = (spaceMode === SPACE_LOCAL) ? SPACE_WORLD : SPACE_LOCAL;
        that.updateHandles();
        if (wantDebug) {
            print("PostToggle: " + spaceMode);        
            print("======== ToggleSpaceMode called. <=========");
        }
    };

    function addHandleTool(overlay, tool) {
        handleTools[overlay] = tool;
        return tool;
    }

    // @param: toolHandle:  The overlayID associated with the tool
    //         that correlates to the tool you wish to query.
    // @note: If toolHandle is null or undefined then activeTool
    //        will be checked against those values as opposed to
    //        the tool registered under toolHandle.  Null & Undefined 
    //        are treated as separate values.
    // @return: bool - Indicates if the activeTool is that queried.
    function isActiveTool(toolHandle) {
        if (!toolHandle) {
            // Allow isActiveTool(null) and similar to return true if there's
            // no active tool
            return (activeTool === toolHandle);
        }

        if (!handleTools.hasOwnProperty(toolHandle)) {
            print("WARNING: entitySelectionTool.isActiveTool - Encountered unknown grabberToolHandle: " + 
                  toolHandle + ". Tools should be registered via addHandleTool.");
            // EARLY EXIT
            return false;
        }

        return (activeTool === handleTools[ toolHandle ]);
    }

    // FUNCTION: UPDATE HANDLES
    that.updateHandles = function() {
        var wantDebug = false;
        if (wantDebug) {
            print("======> Update Handles =======");
            print("    Selections Count: " + SelectionManager.selections.length);
            print("    SpaceMode: " + spaceMode);
            print("    DisplayMode: " + getMode());
        }

        if (SelectionManager.selections.length === 0) {
            that.setOverlaysVisible(false);
            return;
        }

        if (SelectionManager.hasSelection()) {
            var position = SelectionManager.worldPosition;
            var rotation = spaceMode === SPACE_LOCAL ? SelectionManager.localRotation : SelectionManager.worldRotation;
            var dimensions = spaceMode === SPACE_LOCAL ? SelectionManager.localDimensions : SelectionManager.worldDimensions;
            var rotationInverse = Quat.inverse(rotation);
            var toCameraDistance = getDistanceToCamera(position);

            var rotationDegrees = 90;
            var localRotationX = Quat.fromPitchYawRollDegrees(0, 0, -rotationDegrees);
            var rotationX = Quat.multiply(rotation, localRotationX);
            worldRotationX = rotationX;
            var localRotationY = Quat.fromPitchYawRollDegrees(0, rotationDegrees, 0);
            var rotationY = Quat.multiply(rotation, localRotationY);
            worldRotationY = rotationY;
            var localRotationZ = Quat.fromPitchYawRollDegrees(rotationDegrees, 0, 0);
            var rotationZ = Quat.multiply(rotation, localRotationZ);
            worldRotationZ = rotationZ;
            
            var selectionBoxGeometry = {
                position: position,
                rotation: rotation,
                dimensions: dimensions
            };
            var isCameraInsideBox = isPointInsideBox(Camera.position, selectionBoxGeometry);

            // in HMD if outside the bounding box clamp the overlays to the bounding box for now so lasers can hit them
            var maxHandleDimension = 0;
            if (HMD.active && !isCameraInsideBox) {
                maxHandleDimension = Math.max(dimensions.x, dimensions.y, dimensions.z);
            }

            // UPDATE ROTATION RINGS
            // rotateDimension is used as the base dimension for all overlays
            var rotateDimension = Math.max(maxHandleDimension, toCameraDistance * ROTATE_RING_CAMERA_DISTANCE_MULTIPLE);
            var rotateDimensions = { x: rotateDimension, y: rotateDimension, z: rotateDimension };
            if (!isActiveTool(handleRotatePitchRing)) {
                Overlays.editOverlay(handleRotatePitchRing, { 
                    position: position, 
                    rotation: rotationY,
                    dimensions: rotateDimensions,
                    majorTickMarksAngle: ROTATE_DEFAULT_TICK_MARKS_ANGLE
                });
            }
            if (!isActiveTool(handleRotateYawRing)) {
                Overlays.editOverlay(handleRotateYawRing, { 
                    position: position, 
                    rotation: rotationZ,
                    dimensions: rotateDimensions,
                    majorTickMarksAngle: ROTATE_DEFAULT_TICK_MARKS_ANGLE
                });
            }
            if (!isActiveTool(handleRotateRollRing)) {
                Overlays.editOverlay(handleRotateRollRing, { 
                    position: position, 
                    rotation: rotationX,
                    dimensions: rotateDimensions,
                    majorTickMarksAngle: ROTATE_DEFAULT_TICK_MARKS_ANGLE
                });
            }
            Overlays.editOverlay(handleRotateCurrentRing, { dimensions: rotateDimensions });
            that.updateActiveRotateRing();

            // UPDATE TRANSLATION ARROWS
            var arrowCylinderDimension = rotateDimension * TRANSLATE_ARROW_CYLINDER_CAMERA_DISTANCE_MULTIPLE / 
                                                           ROTATE_RING_CAMERA_DISTANCE_MULTIPLE;
            var arrowCylinderDimensions = { 
                x: arrowCylinderDimension, 
                y: arrowCylinderDimension * TRANSLATE_ARROW_CYLINDER_Y_MULTIPLE, 
                z: arrowCylinderDimension 
            };
            var arrowConeDimension = rotateDimension * TRANSLATE_ARROW_CONE_CAMERA_DISTANCE_MULTIPLE / 
                                                       ROTATE_RING_CAMERA_DISTANCE_MULTIPLE;
            var arrowConeDimensions = { x: arrowConeDimension, y: arrowConeDimension, z: arrowConeDimension };
            var arrowCylinderOffset = rotateDimension * TRANSLATE_ARROW_CYLINDER_OFFSET / ROTATE_RING_CAMERA_DISTANCE_MULTIPLE;
            var arrowConeOffset = arrowCylinderDimensions.y * TRANSLATE_ARROW_CONE_OFFSET_CYLINDER_DIMENSION_MULTIPLE;
            var cylinderXPosition = { x: arrowCylinderOffset, y: 0, z: 0 };
            cylinderXPosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, cylinderXPosition));
            Overlays.editOverlay(handleTranslateXCylinder, { 
                position: cylinderXPosition, 
                rotation: rotationX,
                dimensions: arrowCylinderDimensions
            });
            var cylinderXOffset = Vec3.subtract(cylinderXPosition, position);
            var coneXPosition = Vec3.sum(cylinderXPosition, Vec3.multiply(Vec3.normalize(cylinderXOffset), arrowConeOffset));
            Overlays.editOverlay(handleTranslateXCone, { 
                position: coneXPosition, 
                rotation: rotationX,
                dimensions: arrowConeDimensions
            });
            var cylinderYPosition = { x: 0, y: arrowCylinderOffset, z: 0 };
            cylinderYPosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, cylinderYPosition));
            Overlays.editOverlay(handleTranslateYCylinder, { 
                position: cylinderYPosition, 
                rotation: rotationY,
                dimensions: arrowCylinderDimensions
            });
            var cylinderYOffset = Vec3.subtract(cylinderYPosition, position);
            var coneYPosition = Vec3.sum(cylinderYPosition, Vec3.multiply(Vec3.normalize(cylinderYOffset), arrowConeOffset));
            Overlays.editOverlay(handleTranslateYCone, { 
                position: coneYPosition, 
                rotation: rotationY,
                dimensions: arrowConeDimensions
            });
            var cylinderZPosition = { x: 0, y: 0, z: arrowCylinderOffset };
            cylinderZPosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, cylinderZPosition));
            Overlays.editOverlay(handleTranslateZCylinder, { 
                position: cylinderZPosition, 
                rotation: rotationZ,
                dimensions: arrowCylinderDimensions
            });
            var cylinderZOffset = Vec3.subtract(cylinderZPosition, position);
            var coneZPosition = Vec3.sum(cylinderZPosition, Vec3.multiply(Vec3.normalize(cylinderZOffset), arrowConeOffset));
            Overlays.editOverlay(handleTranslateZCone, { 
                position: coneZPosition, 
                rotation: rotationZ,
                dimensions: arrowConeDimensions
            });

            // UPDATE SCALE CUBE
            var scaleCubeRotation = spaceMode === SPACE_LOCAL ? rotation : Quat.IDENTITY;            
            var scaleCubeDimension = rotateDimension * SCALE_CUBE_CAMERA_DISTANCE_MULTIPLE / 
                                                           ROTATE_RING_CAMERA_DISTANCE_MULTIPLE;
            var scaleCubeDimensions = { x: scaleCubeDimension, y: scaleCubeDimension, z: scaleCubeDimension };
            Overlays.editOverlay(handleScaleCube, { 
                position: position, 
                rotation: scaleCubeRotation,
                dimensions: scaleCubeDimensions
            });

            // UPDATE BOUNDING BOX EDGES
            var edgeOffsetX = BOUNDING_EDGE_OFFSET * dimensions.x;
            var edgeOffsetY = BOUNDING_EDGE_OFFSET * dimensions.y;
            var edgeOffsetZ = BOUNDING_EDGE_OFFSET * dimensions.z;
            var LBNPosition = { x: -edgeOffsetX, y: -edgeOffsetY, z: -edgeOffsetZ };
            LBNPosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, LBNPosition));
            var RBNPosition = { x: edgeOffsetX, y: -edgeOffsetY, z: -edgeOffsetZ };
            RBNPosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, RBNPosition));
            var LBFPosition = { x: -edgeOffsetX, y: -edgeOffsetY, z: edgeOffsetZ };
            LBFPosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, LBFPosition));
            var RBFPosition = { x: edgeOffsetX, y: -edgeOffsetY, z: edgeOffsetZ };
            RBFPosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, RBFPosition));
            var LTNPosition = { x: -edgeOffsetX, y: edgeOffsetY, z: -edgeOffsetZ };
            LTNPosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, LTNPosition));
            var RTNPosition = { x: edgeOffsetX, y: edgeOffsetY, z: -edgeOffsetZ };
            RTNPosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, RTNPosition));
            var LTFPosition = { x: -edgeOffsetX, y: edgeOffsetY, z: edgeOffsetZ };
            LTFPosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, LTFPosition));
            var RTFPosition = { x: edgeOffsetX, y: edgeOffsetY, z: edgeOffsetZ };
            RTFPosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, RTFPosition));
            Overlays.editOverlay(handleBoundingTREdge, { start: RTNPosition, end: RTFPosition });
            Overlays.editOverlay(handleBoundingTLEdge, { start: LTNPosition, end: LTFPosition });
            Overlays.editOverlay(handleBoundingTFEdge, { start: LTFPosition, end: RTFPosition });
            Overlays.editOverlay(handleBoundingTNEdge, { start: LTNPosition, end: RTNPosition });
            Overlays.editOverlay(handleBoundingBREdge, { start: RBNPosition, end: RBFPosition });
            Overlays.editOverlay(handleBoundingBLEdge, { start: LBNPosition, end: LBFPosition });
            Overlays.editOverlay(handleBoundingBFEdge, { start: LBFPosition, end: RBFPosition });
            Overlays.editOverlay(handleBoundingBNEdge, { start: LBNPosition, end: RBNPosition });
            Overlays.editOverlay(handleBoundingNREdge, { start: RTNPosition, end: RBNPosition });
            Overlays.editOverlay(handleBoundingNLEdge, { start: LTNPosition, end: LBNPosition });
            Overlays.editOverlay(handleBoundingFREdge, { start: RTFPosition, end: RBFPosition });
            Overlays.editOverlay(handleBoundingFLEdge, { start: LTFPosition, end: LBFPosition });
            
            // UPDATE STRETCH HIGHLIGHT PANELS
            var RBFPositionRotated = Vec3.multiplyQbyV(rotationInverse, RBFPosition);
            var RTFPositionRotated = Vec3.multiplyQbyV(rotationInverse, RTFPosition);
            var LTNPositionRotated = Vec3.multiplyQbyV(rotationInverse, LTNPosition);
            var RTNPositionRotated = Vec3.multiplyQbyV(rotationInverse, RTNPosition);
            var stretchPanelXDimensions = Vec3.subtract(RTNPositionRotated, RBFPositionRotated);
            var tempY = Math.abs(stretchPanelXDimensions.y);
            stretchPanelXDimensions.x = STRETCH_PANEL_WIDTH;
            stretchPanelXDimensions.y = Math.abs(stretchPanelXDimensions.z);
            stretchPanelXDimensions.z = tempY;
            var stretchPanelXPosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, { x: dimensions.x / 2, y: 0, z: 0 }));
            Overlays.editOverlay(handleStretchXPanel, { 
                position: stretchPanelXPosition, 
                rotation: rotationZ,
                dimensions: stretchPanelXDimensions
            });
            var stretchPanelYDimensions = Vec3.subtract(LTNPositionRotated, RTFPositionRotated);
            var tempX = Math.abs(stretchPanelYDimensions.x);
            stretchPanelYDimensions.x = Math.abs(stretchPanelYDimensions.z);
            stretchPanelYDimensions.y = STRETCH_PANEL_WIDTH;
            stretchPanelYDimensions.z = tempX;
            var stretchPanelYPosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, { x: 0, y: dimensions.y / 2, z: 0 }));
            Overlays.editOverlay(handleStretchYPanel, { 
                position: stretchPanelYPosition, 
                rotation: rotationY,
                dimensions: stretchPanelYDimensions
            });
            var stretchPanelZDimensions = Vec3.subtract(LTNPositionRotated, RBFPositionRotated);
            tempX = Math.abs(stretchPanelZDimensions.x);
            stretchPanelZDimensions.x = Math.abs(stretchPanelZDimensions.y);
            stretchPanelZDimensions.y = tempX;
            stretchPanelZDimensions.z = STRETCH_PANEL_WIDTH;
            var stretchPanelZPosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, { x: 0, y: 0, z: dimensions.z / 2 }));
            Overlays.editOverlay(handleStretchZPanel, { 
                position: stretchPanelZPosition, 
                rotation: rotationX,
                dimensions: stretchPanelZDimensions
            });

            // UPDATE STRETCH CUBES
            var stretchCubeDimension = rotateDimension * STRETCH_CUBE_CAMERA_DISTANCE_MULTIPLE / 
                                                           ROTATE_RING_CAMERA_DISTANCE_MULTIPLE;
            var stretchCubeDimensions = { x: stretchCubeDimension, y: stretchCubeDimension, z: stretchCubeDimension };
            var stretchCubeOffset = rotateDimension * STRETCH_CUBE_OFFSET / ROTATE_RING_CAMERA_DISTANCE_MULTIPLE;
            var stretchXPosition, stretchYPosition, stretchZPosition;
            if (isActiveTool(handleStretchXCube)) {
                stretchXPosition = Vec3.subtract(stretchPanelXPosition, activeStretchCubePanelOffset);
            } else {
                stretchXPosition = { x: stretchCubeOffset, y: 0, z: 0 };
                stretchXPosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, stretchXPosition));
            }
            if (isActiveTool(handleStretchYCube)) {
                stretchYPosition = Vec3.subtract(stretchPanelYPosition, activeStretchCubePanelOffset);
            } else {
                stretchYPosition = { x: 0, y: stretchCubeOffset, z: 0 };
                stretchYPosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, stretchYPosition));
            }
            if (isActiveTool(handleStretchZCube)) {
                stretchZPosition = Vec3.subtract(stretchPanelZPosition, activeStretchCubePanelOffset);
            } else {
                stretchZPosition = { x: 0, y: 0, z: stretchCubeOffset };
                stretchZPosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, stretchZPosition));
            }
            Overlays.editOverlay(handleStretchXCube, { 
                position: stretchXPosition, 
                rotation: rotationX,
                dimensions: stretchCubeDimensions 
            });
            Overlays.editOverlay(handleStretchYCube, { 
                position: stretchYPosition, 
                rotation: rotationY,
                dimensions: stretchCubeDimensions 
            });
            Overlays.editOverlay(handleStretchZCube, { 
                position: stretchZPosition,
                rotation: rotationZ,
                dimensions: stretchCubeDimensions 
            });

            // UPDATE SELECTION BOX (CURRENTLY INVISIBLE WITH 0 ALPHA FOR TRANSLATE XZ TOOL)
            var inModeRotate = isActiveTool(handleRotatePitchRing) || 
                               isActiveTool(handleRotateYawRing) || 
                               isActiveTool(handleRotateRollRing);
            selectionBoxGeometry.visible = !inModeRotate && !isCameraInsideBox;
            Overlays.editOverlay(selectionBox, selectionBoxGeometry);

            // UPDATE ICON TRANSLATE HANDLE
            if (SelectionManager.entityType === "ParticleEffect" || SelectionManager.entityType === "Light") {
                var iconSelectionBoxGeometry = {
                    position: position,
                    rotation: rotation
                };
                iconSelectionBoxGeometry.visible = !inModeRotate && isCameraInsideBox;
                Overlays.editOverlay(iconSelectionBox, iconSelectionBoxGeometry);
            } else {
                Overlays.editOverlay(iconSelectionBox, { visible: false });
            }

            // UPDATE CLONER (CURRENTLY HIDDEN FOR NOW)
            var handleClonerOffset = { 
                x: CLONER_OFFSET.x * dimensions.x, 
                y: CLONER_OFFSET.y * dimensions.y, 
                z: CLONER_OFFSET.z * dimensions.z 
            };
            var handleClonerPos = Vec3.sum(position, Vec3.multiplyQbyV(rotation, handleClonerOffset));
            Overlays.editOverlay(handleCloner, {
                position: handleClonerPos,
                rotation: rotation,
                dimensions: scaleCubeDimensions
            });
        }

        that.setHandleTranslateXVisible(!activeTool || isActiveTool(handleTranslateXCone) || 
                                                       isActiveTool(handleTranslateXCylinder));
        that.setHandleTranslateYVisible(!activeTool || isActiveTool(handleTranslateYCone) || 
                                                       isActiveTool(handleTranslateYCylinder));
        that.setHandleTranslateZVisible(!activeTool || isActiveTool(handleTranslateZCone) || 
                                                       isActiveTool(handleTranslateZCylinder));
        that.setHandleRotatePitchVisible(!activeTool || isActiveTool(handleRotatePitchRing));
        that.setHandleRotateYawVisible(!activeTool || isActiveTool(handleRotateYawRing));
        that.setHandleRotateRollVisible(!activeTool || isActiveTool(handleRotateRollRing));

        var showScaleStretch = !activeTool && SelectionManager.selections.length === 1 && spaceMode === SPACE_LOCAL;
        that.setHandleStretchXVisible(showScaleStretch || isActiveTool(handleStretchXCube));
        that.setHandleStretchYVisible(showScaleStretch || isActiveTool(handleStretchYCube));
        that.setHandleStretchZVisible(showScaleStretch || isActiveTool(handleStretchZCube));
        that.setHandleScaleCubeVisible(showScaleStretch || isActiveTool(handleScaleCube));

        var showOutlineForZone = (SelectionManager.selections.length === 1 && 
                                    typeof SelectionManager.savedProperties[SelectionManager.selections[0]] !== "undefined" &&
                                    SelectionManager.savedProperties[SelectionManager.selections[0]].type === "Zone");
        that.setHandleBoundingEdgeVisible(showOutlineForZone || (!isActiveTool(handleRotatePitchRing) &&
                                                              !isActiveTool(handleRotateYawRing) &&
                                                              !isActiveTool(handleRotateRollRing)));

        // keep cloner always hidden for now since you can hold Alt to clone while  
        // translating an entity - we may bring cloner back for HMD only later
        // that.setHandleClonerVisible(!activeTool || isActiveTool(handleCloner));

        if (wantDebug) {
            print("====== Update Handles <=======");
        }
    };
    Script.update.connect(that.updateHandles);

    // FUNCTION: UPDATE ACTIVE ROTATE RING
    that.updateActiveRotateRing = function() {
        var activeRotateRing = null;
        if (isActiveTool(handleRotatePitchRing)) {
            activeRotateRing = handleRotatePitchRing;
        } else if (isActiveTool(handleRotateYawRing)) {
            activeRotateRing = handleRotateYawRing;
        } else if (isActiveTool(handleRotateRollRing)) {
            activeRotateRing = handleRotateRollRing;
        }
        if (activeRotateRing !== null) {
            var tickMarksAngle = ctrlPressed ? ROTATE_CTRL_SNAP_ANGLE : ROTATE_DEFAULT_TICK_MARKS_ANGLE;
            Overlays.editOverlay(activeRotateRing, { majorTickMarksAngle: tickMarksAngle });
        }
    };

    // FUNCTION: SET OVERLAYS VISIBLE
    that.setOverlaysVisible = function(isVisible) {
        for (var i = 0, length = allOverlays.length; i < length; i++) {
            Overlays.editOverlay(allOverlays[i], { visible: isVisible });
        }
    };

    // FUNCTION: SET HANDLE TRANSLATE VISIBLE
    that.setHandleTranslateVisible = function(isVisible) {
        that.setHandleTranslateXVisible(isVisible);
        that.setHandleTranslateYVisible(isVisible);
        that.setHandleTranslateZVisible(isVisible);
    };

    that.setHandleTranslateXVisible = function(isVisible) {
        Overlays.editOverlay(handleTranslateXCone, { visible: isVisible });
        Overlays.editOverlay(handleTranslateXCylinder, { visible: isVisible });
    };

    that.setHandleTranslateYVisible = function(isVisible) {
        Overlays.editOverlay(handleTranslateYCone, { visible: isVisible });
        Overlays.editOverlay(handleTranslateYCylinder, { visible: isVisible });
    };

    that.setHandleTranslateZVisible = function(isVisible) {
        Overlays.editOverlay(handleTranslateZCone, { visible: isVisible });
        Overlays.editOverlay(handleTranslateZCylinder, { visible: isVisible });
    };

    // FUNCTION: SET HANDLE ROTATE VISIBLE
    that.setHandleRotateVisible = function(isVisible) {
        that.setHandleRotatePitchVisible(isVisible);
        that.setHandleRotateYawVisible(isVisible);
        that.setHandleRotateRollVisible(isVisible);
    };

    that.setHandleRotatePitchVisible = function(isVisible) {
        Overlays.editOverlay(handleRotatePitchRing, { visible: isVisible });
    };

    that.setHandleRotateYawVisible = function(isVisible) {
        Overlays.editOverlay(handleRotateYawRing, { visible: isVisible });
    };

    that.setHandleRotateRollVisible = function(isVisible) {
        Overlays.editOverlay(handleRotateRollRing, { visible: isVisible });
    };

    // FUNCTION: SET HANDLE STRETCH VISIBLE
    that.setHandleStretchVisible = function(isVisible) {
        that.setHandleStretchXVisible(isVisible);
        that.setHandleStretchYVisible(isVisible);
        that.setHandleStretchZVisible(isVisible);
    };

    that.setHandleStretchXVisible = function(isVisible) {
        Overlays.editOverlay(handleStretchXCube, { visible: isVisible });
    };

    that.setHandleStretchYVisible = function(isVisible) {
        Overlays.editOverlay(handleStretchYCube, { visible: isVisible });
    };

    that.setHandleStretchZVisible = function(isVisible) {
        Overlays.editOverlay(handleStretchZCube, { visible: isVisible });
    };
    
    // FUNCTION: SET HANDLE SCALE VISIBLE
    that.setHandleScaleVisible = function(isVisible) {
        that.setHandleScaleCubeVisible(isVisible);
        that.setHandleBoundingEdgeVisible(isVisible);
    };

    that.setHandleScaleCubeVisible = function(isVisible) {
        Overlays.editOverlay(handleScaleCube, { visible: isVisible });
    };

    that.setHandleBoundingEdgeVisible = function(isVisible) {
        Overlays.editOverlay(handleBoundingTREdge, { visible: isVisible });
        Overlays.editOverlay(handleBoundingTLEdge, { visible: isVisible });
        Overlays.editOverlay(handleBoundingTFEdge, { visible: isVisible });
        Overlays.editOverlay(handleBoundingTNEdge, { visible: isVisible });
        Overlays.editOverlay(handleBoundingBREdge, { visible: isVisible });
        Overlays.editOverlay(handleBoundingBLEdge, { visible: isVisible });
        Overlays.editOverlay(handleBoundingBFEdge, { visible: isVisible });
        Overlays.editOverlay(handleBoundingBNEdge, { visible: isVisible });
        Overlays.editOverlay(handleBoundingNREdge, { visible: isVisible });
        Overlays.editOverlay(handleBoundingNLEdge, { visible: isVisible });
        Overlays.editOverlay(handleBoundingFREdge, { visible: isVisible });
        Overlays.editOverlay(handleBoundingFLEdge, { visible: isVisible });
    };

    // FUNCTION: SET HANDLE CLONER VISIBLE
    that.setHandleClonerVisible = function(isVisible) {
        Overlays.editOverlay(handleCloner, { visible: isVisible });
    };

    // TOOL DEFINITION: TRANSLATE XZ TOOL
    var initialXZPick = null;
    var isConstrained = false;
    var constrainMajorOnly = false;
    var startPosition = null;
    var duplicatedEntityIDs = null;
    var translateXZTool = addHandleTool(selectionBox, {
        mode: 'TRANSLATE_XZ',
        pickPlanePosition: { x: 0, y: 0, z: 0 },
        greatestDimension: 0.0,
        startingDistance: 0.0,
        startingElevation: 0.0,
        onBegin: function(event, pickRay, pickResult, doClone) {
            var wantDebug = false;
            if (wantDebug) {
                print("================== TRANSLATE_XZ(Beg) -> =======================");
                Vec3.print("    pickRay", pickRay);
                Vec3.print("    pickRay.origin", pickRay.origin);
                Vec3.print("    pickResult.intersection", pickResult.intersection);
            }

            // Duplicate entities if alt is pressed.  This will make a
            // copy of the selected entities and move the _original_ entities, not
            // the new ones.
            if (event.isAlt || doClone) {
                duplicatedEntityIDs = SelectionManager.duplicateSelection();
                var ids = [];
                for (var i = 0; i < duplicatedEntityIDs.length; ++i) {
                    ids.push(duplicatedEntityIDs[i].entityID);
                }
                SelectionManager.setSelections(ids);
            } else {
                duplicatedEntityIDs = null;
            }

            SelectionManager.saveProperties();
            that.resetPreviousHandleColor();

            that.setHandleTranslateVisible(false);
            that.setHandleRotateVisible(false);
            that.setHandleScaleCubeVisible(false);
            that.setHandleStretchVisible(false);
            that.setHandleClonerVisible(false);

            startPosition = SelectionManager.worldPosition;

            translateXZTool.pickPlanePosition = pickResult.intersection;
            translateXZTool.greatestDimension = Math.max(Math.max(SelectionManager.worldDimensions.x, 
                                                                  SelectionManager.worldDimensions.y),
                                                                  SelectionManager.worldDimensions.z);
            translateXZTool.startingDistance = Vec3.distance(pickRay.origin, SelectionManager.position);
            translateXZTool.startingElevation = translateXZTool.elevation(pickRay.origin, translateXZTool.pickPlanePosition);
            if (wantDebug) {
                print("    longest dimension: " + translateXZTool.greatestDimension);
                print("    starting distance: " + translateXZTool.startingDistance);
                print("    starting elevation: " + translateXZTool.startingElevation);
            }

            initialXZPick = rayPlaneIntersection(pickRay, translateXZTool.pickPlanePosition, {
                x: 0,
                y: 1,
                z: 0
            });

            isConstrained = false;
            if (wantDebug) {
                print("================== TRANSLATE_XZ(End) <- =======================");
            }
        },
        onEnd: function(event, reason) {
            pushCommandForSelections(duplicatedEntityIDs);
            if (isConstrained) {
                Overlays.editOverlay(xRailOverlay, {
                    visible: false
                });
                Overlays.editOverlay(zRailOverlay, {
                    visible: false
                });
            }
        },
        elevation: function(origin, intersection) {
            return (origin.y - intersection.y) / Vec3.distance(origin, intersection);
        },
        onMove: function(event) {
            var wantDebug = false;
            var pickRay = generalComputePickRay(event.x, event.y);

            var pick = rayPlaneIntersection2(pickRay, translateXZTool.pickPlanePosition, {
                x: 0,
                y: 1,
                z: 0
            });

            // If the pick ray doesn't hit the pick plane in this direction, do nothing.
            // this will happen when someone drags across the horizon from the side they started on.
            if (!pick) {
                if (wantDebug) {
                    print("    "+ translateXZTool.mode + "Pick ray does not intersect XZ plane.");
                }
                
                // EARLY EXIT--(Invalid ray detected.)
                return;
            }

            var vector = Vec3.subtract(pick, initialXZPick);

            // If the mouse is too close to the horizon of the pick plane, stop moving
            var MIN_ELEVATION = 0.02; //  largest dimension of object divided by distance to it
            var elevation = translateXZTool.elevation(pickRay.origin, pick);
            if (wantDebug) {
                print("Start Elevation: " + translateXZTool.startingElevation + ", elevation: " + elevation);
            }
            if ((translateXZTool.startingElevation > 0.0 && elevation < MIN_ELEVATION) ||
                (translateXZTool.startingElevation < 0.0 && elevation > -MIN_ELEVATION)) {
                if (wantDebug) {
                    print("    "+ translateXZTool.mode + " - too close to horizon!");
                }

                // EARLY EXIT--(Don't proceed past the reached limit.)
                return;
            }

            //  If the angular size of the object is too small, stop moving
            var MIN_ANGULAR_SIZE = 0.01; //  Radians
            if (translateXZTool.greatestDimension > 0) {
                var angularSize = Math.atan(translateXZTool.greatestDimension / Vec3.distance(pickRay.origin, pick));
                if (wantDebug) {
                    print("Angular size = " + angularSize);
                }
                if (angularSize < MIN_ANGULAR_SIZE) {
                    return;
                }
            }

            // If shifted, constrain to one axis
            if (event.isShifted) {
                if (Math.abs(vector.x) > Math.abs(vector.z)) {
                    vector.z = 0;
                } else {
                    vector.x = 0;
                }
                if (!isConstrained) {
                    var xStart = Vec3.sum(startPosition, {
                        x: -RAIL_AXIS_LENGTH,
                        y: 0,
                        z: 0
                    });
                    var xEnd = Vec3.sum(startPosition, {
                        x: RAIL_AXIS_LENGTH,
                        y: 0,
                        z: 0
                    });
                    var zStart = Vec3.sum(startPosition, {
                        x: 0,
                        y: 0,
                        z: -RAIL_AXIS_LENGTH
                    });
                    var zEnd = Vec3.sum(startPosition, {
                        x: 0,
                        y: 0,
                        z: RAIL_AXIS_LENGTH
                    });
                    Overlays.editOverlay(xRailOverlay, {
                        start: xStart,
                        end: xEnd,
                        visible: true
                    });
                    Overlays.editOverlay(zRailOverlay, {
                        start: zStart,
                        end: zEnd,
                        visible: true
                    });
                    isConstrained = true;
                }
            } else {
                if (isConstrained) {
                    Overlays.editOverlay(xRailOverlay, {
                        visible: false
                    });
                    Overlays.editOverlay(zRailOverlay, {
                        visible: false
                    });
                    isConstrained = false;
                }
            }

            constrainMajorOnly = event.isControl;
            var negateAndHalve = -0.5;
            var cornerPosition = Vec3.sum(startPosition, Vec3.multiply(negateAndHalve, SelectionManager.worldDimensions));
            vector = Vec3.subtract(
                grid.snapToGrid(Vec3.sum(cornerPosition, vector), constrainMajorOnly),
                cornerPosition);

            // editing a parent will cause all the children to automatically follow along, so don't
            // edit any entity who has an ancestor in SelectionManager.selections
            var toMove = SelectionManager.selections.filter(function (selection) {
                if (SelectionManager.selections.indexOf(SelectionManager.savedProperties[selection].parentID) >= 0) {
                    return false; // a parent is also being moved, so don't issue an edit for this entity
                } else {
                    return true;
                }
            });

            for (var i = 0; i < toMove.length; i++) {
                var properties = SelectionManager.savedProperties[toMove[i]];
                if (!properties) {
                    continue;
                }
                var newPosition = Vec3.sum(properties.position, {
                    x: vector.x,
                    y: 0,
                    z: vector.z
                });
                Entities.editEntity(toMove[i], {
                    position: newPosition
                });

                if (wantDebug) {
                    print("translateXZ... ");
                    Vec3.print("                 vector:", vector);
                    Vec3.print("            newPosition:", properties.position);
                    Vec3.print("            newPosition:", newPosition);
                }
            }

            SelectionManager._update();
        }
    });

    // TOOL DEFINITION: HANDLE TRANSLATE TOOL    
    function addHandleTranslateTool(overlay, mode, direction) {
        var pickNormal = null;
        var lastPick = null;
        var initialPosition = null;
        var projectionVector = null;
        var previousPickRay = null;
        addHandleTool(overlay, {
            mode: mode,
            onBegin: function(event, pickRay, pickResult) {
                // Duplicate entities if alt is pressed.  This will make a
                // copy of the selected entities and move the _original_ entities, not
                // the new ones.
                if (event.isAlt) {
                    duplicatedEntityIDs = SelectionManager.duplicateSelection();
                    var ids = [];
                    for (var i = 0; i < duplicatedEntityIDs.length; ++i) {
                        ids.push(duplicatedEntityIDs[i].entityID);
                    }
                    SelectionManager.setSelections(ids);
                } else {
                    duplicatedEntityIDs = null;
                }

                var axisVector;
                if (direction === TRANSLATE_DIRECTION.X) {
                    axisVector = { x: 1, y: 0, z: 0 };
                } else if (direction === TRANSLATE_DIRECTION.Y) {
                    axisVector = { x: 0, y: 1, z: 0 };
                } else if (direction === TRANSLATE_DIRECTION.Z) {
                    axisVector = { x: 0, y: 0, z: 1 };
                }

                var rotation = spaceMode === SPACE_LOCAL ? SelectionManager.localRotation : SelectionManager.worldRotation;
                axisVector = Vec3.multiplyQbyV(rotation, axisVector);
                pickNormal = Vec3.cross(Vec3.cross(pickRay.direction, axisVector), axisVector);

                lastPick = rayPlaneIntersection(pickRay, SelectionManager.worldPosition, pickNormal);
                initialPosition = SelectionManager.worldPosition;
    
                SelectionManager.saveProperties();
                that.resetPreviousHandleColor();

                that.setHandleTranslateXVisible(direction === TRANSLATE_DIRECTION.X);
                that.setHandleTranslateYVisible(direction === TRANSLATE_DIRECTION.Y);
                that.setHandleTranslateZVisible(direction === TRANSLATE_DIRECTION.Z);
                that.setHandleRotateVisible(false);
                that.setHandleStretchVisible(false);
                that.setHandleScaleCubeVisible(false);
                that.setHandleClonerVisible(false);
                
                previousPickRay = pickRay;
            },
            onEnd: function(event, reason) {
                pushCommandForSelections(duplicatedEntityIDs);
            },
            onMove: function(event) {
                var pickRay = generalComputePickRay(event.x, event.y);
                
                // Use previousPickRay if new pickRay will cause resulting rayPlaneIntersection values to wrap around
                if (usePreviousPickRay(pickRay.direction, previousPickRay.direction, pickNormal)) {
                    pickRay = previousPickRay;
                }
    
                var newIntersection = rayPlaneIntersection(pickRay, initialPosition, pickNormal);
                var vector = Vec3.subtract(newIntersection, lastPick);
                
                if (direction === TRANSLATE_DIRECTION.X) {
                    projectionVector = { x: 1, y: 0, z: 0 };
                } else if (direction === TRANSLATE_DIRECTION.Y) {
                    projectionVector = { x: 0, y: 1, z: 0 };
                } else if (direction === TRANSLATE_DIRECTION.Z) {
                    projectionVector = { x: 0, y: 0, z: 1 };
                }

                var rotation = spaceMode === SPACE_LOCAL ? SelectionManager.localRotation : SelectionManager.worldRotation;
                projectionVector = Vec3.multiplyQbyV(rotation, projectionVector);

                var dotVector = Vec3.dot(vector, projectionVector);
                vector = Vec3.multiply(dotVector, projectionVector);
                var gridOrigin = grid.getOrigin();
                vector = Vec3.subtract(grid.snapToGrid(Vec3.sum(vector, gridOrigin)), gridOrigin);
                
                var wantDebug = false;
                if (wantDebug) {
                    print("translateUpDown... ");
                    print("                event.y:" + event.y);
                    Vec3.print("        newIntersection:", newIntersection);
                    Vec3.print("                 vector:", vector);
                }

                // editing a parent will cause all the children to automatically follow along, so don't
                // edit any entity who has an ancestor in SelectionManager.selections
                var toMove = SelectionManager.selections.filter(function (selection) {
                    if (SelectionManager.selections.indexOf(SelectionManager.savedProperties[selection].parentID) >= 0) {
                        return false; // a parent is also being moved, so don't issue an edit for this entity
                    } else {
                        return true;
                    }
                });

                for (var i = 0; i < toMove.length; i++) {
                    var id = toMove[i];
                    var properties = SelectionManager.savedProperties[id];
                    var newPosition = Vec3.sum(properties.position, vector);
                    Entities.editEntity(id, { position: newPosition });
                }
                
                previousPickRay = pickRay;
    
                SelectionManager._update();
            }
        });
    }

    // FUNCTION: VEC 3 MULT
    var vec3Mult = function(v1, v2) {
        return {
            x: v1.x * v2.x,
            y: v1.y * v2.y,
            z: v1.z * v2.z
        };
    };

    // TOOL DEFINITION: HANDLE STRETCH TOOL   
    function makeStretchTool(stretchMode, directionEnum, directionVec, pivot, offset, stretchPanel, cubeHandle) {
        var directionFor3DStretch = directionVec;
        var distanceFor3DStretch = 0;
        var DISTANCE_INFLUENCE_THRESHOLD = 1.2;
        
        var signs = {
            x: directionVec.x < 0 ? -1 : (directionVec.x > 0 ? 1 : 0),
            y: directionVec.y < 0 ? -1 : (directionVec.y > 0 ? 1 : 0),
            z: directionVec.z < 0 ? -1 : (directionVec.z > 0 ? 1 : 0)
        };

        var mask = {
            x: Math.abs(directionVec.x) > 0 ? 1 : 0,
            y: Math.abs(directionVec.y) > 0 ? 1 : 0,
            z: Math.abs(directionVec.z) > 0 ? 1 : 0
        };

        var numDimensions = mask.x + mask.y + mask.z;

        var planeNormal = null;
        var lastPick = null;
        var lastPick3D = null;
        var initialPosition = null;
        var initialDimensions = null;
        var initialProperties = null;
        var registrationPoint = null;
        var deltaPivot = null;
        var deltaPivot3D = null;
        var pickRayPosition = null;
        var pickRayPosition3D = null;
        var rotation = null;
        var previousPickRay = null;
        var beginMouseEvent = null;
        var beginControllerPosition = null;

        var onBegin = function(event, pickRay, pickResult) {     
            var proportional = directionEnum === STRETCH_DIRECTION.ALL;     
            
            var properties = Entities.getEntityProperties(SelectionManager.selections[0]);
            initialProperties = properties;
            rotation = (spaceMode === SPACE_LOCAL) ? properties.rotation : Quat.IDENTITY;

            if (spaceMode === SPACE_LOCAL) {
                rotation = SelectionManager.localRotation;
                initialPosition = SelectionManager.localPosition;
                initialDimensions = SelectionManager.localDimensions;
                registrationPoint = SelectionManager.localRegistrationPoint;
            } else {
                rotation = SelectionManager.worldRotation;
                initialPosition = SelectionManager.worldPosition;
                initialDimensions = SelectionManager.worldDimensions;
                registrationPoint = SelectionManager.worldRegistrationPoint;
            }

            // Modify range of registrationPoint to be [-0.5, 0.5]
            var centeredRP = Vec3.subtract(registrationPoint, {
                x: 0.5,
                y: 0.5,
                z: 0.5
            });

            // Scale pivot to be in the same range as registrationPoint
            var scaledPivot = Vec3.multiply(0.5, pivot);
            deltaPivot = Vec3.subtract(centeredRP, scaledPivot);

            var scaledOffset = Vec3.multiply(0.5, offset);

            // Offset from the registration point
            var offsetRP = Vec3.subtract(scaledOffset, centeredRP);

            // Scaled offset in world coordinates
            var scaledOffsetWorld = vec3Mult(initialDimensions, offsetRP);
            
            pickRayPosition = Vec3.sum(initialPosition, Vec3.multiplyQbyV(rotation, scaledOffsetWorld));
            
            if (directionFor3DStretch) {
                // pivot, offset and pickPlanePosition for 3D manipulation
                var scaledPivot3D = Vec3.multiply(0.5, Vec3.multiply(1.0, directionFor3DStretch));
                deltaPivot3D = Vec3.subtract(centeredRP, scaledPivot3D);                
                pickRayPosition3D = Vec3.sum(initialPosition, Vec3.multiplyQbyV(rotation, scaledOffsetWorld));
            }

            if (numDimensions === 1) {
                if (mask.x === 1) {
                    planeNormal = {
                        x: 0,
                        y: 1,
                        z: 0
                    };
                } else if (mask.y === 1) {
                    planeNormal = {
                        x: 1,
                        y: 0,
                        z: 0
                    };
                } else {
                    planeNormal = {
                        x: 0,
                        y: 1,
                        z: 0
                    };
                }
            } else if (numDimensions === 2) {
                if (mask.x === 0) {
                    planeNormal = {
                        x: 1,
                        y: 0,
                        z: 0
                    };
                } else if (mask.y === 0) {
                    planeNormal = {
                        x: 0,
                        y: 1,
                        z: 0
                    };
                } else {
                    planeNormal = {
                        x: 0,
                        y: 0,
                        z: 1
                    };
                }
            }
            
            planeNormal = Vec3.multiplyQbyV(rotation, planeNormal);
            lastPick = rayPlaneIntersection(pickRay, pickRayPosition, planeNormal);
                
            var planeNormal3D = {
                x: 0,
                y: 0,
                z: 0
            };
            if (directionFor3DStretch) {
                lastPick3D = rayPlaneIntersection(pickRay,
                    pickRayPosition3D,
                    planeNormal3D);
                distanceFor3DStretch = Vec3.length(Vec3.subtract(pickRayPosition3D, pickRay.origin));
            }

            that.setHandleTranslateVisible(false);
            that.setHandleRotateVisible(false);
            that.setHandleScaleCubeVisible(true);
            that.setHandleStretchXVisible(directionEnum === STRETCH_DIRECTION.X);
            that.setHandleStretchYVisible(directionEnum === STRETCH_DIRECTION.Y);
            that.setHandleStretchZVisible(directionEnum === STRETCH_DIRECTION.Z);
            that.setHandleClonerVisible(false);
        
            SelectionManager.saveProperties();
            that.resetPreviousHandleColor();

            if (stretchPanel !== null) {
                Overlays.editOverlay(stretchPanel, { visible: true });
            }

            var collisionToRemove = "myAvatar";
            if (properties.collidesWith.indexOf(collisionToRemove) > -1) {
                var newCollidesWith = properties.collidesWith.replace(collisionToRemove, "");
                Entities.editEntity(SelectionManager.selections[0], {collidesWith: newCollidesWith});
                that.replaceCollisionsAfterStretch = true;
            }
            
            if (!proportional) {
                var stretchCubePosition = Overlays.getProperty(cubeHandle, "position");
                var stretchPanelPosition = Overlays.getProperty(stretchPanel, "position");
                activeStretchCubePanelOffset = Vec3.subtract(stretchPanelPosition, stretchCubePosition);
            }
            
            previousPickRay = pickRay;
            beginMouseEvent = event;
            if (that.triggered()) {
                beginControllerPosition = getControllerAvatarFramePositionFromPickRay(pickRay);
            }
        };

        var onEnd = function(event, reason) {    
            if (stretchPanel !== null) {
                Overlays.editOverlay(stretchPanel, { visible: false });
            }
            
            if (that.replaceCollisionsAfterStretch) {
                var newCollidesWith = SelectionManager.savedProperties[SelectionManager.selections[0]].collidesWith;
                Entities.editEntity(SelectionManager.selections[0], {collidesWith: newCollidesWith});
                that.replaceCollisionsAfterStretch = false;
            }
            
            activeStretchCubePanelOffset = null;
            
            pushCommandForSelections();
        };

        var onMove = function(event) {
            var proportional = directionEnum === STRETCH_DIRECTION.ALL;
            
            var position, rotation;
            if (spaceMode === SPACE_LOCAL) {
                position = SelectionManager.localPosition;
                rotation = SelectionManager.localRotation;
            } else {
                position = SelectionManager.worldPosition;
                rotation = SelectionManager.worldRotation;
            }
            
            var localDeltaPivot = deltaPivot;
            var localSigns = signs;
            var pickRay = generalComputePickRay(event.x, event.y);
            
            // Use previousPickRay if new pickRay will cause resulting rayPlaneIntersection values to wrap around
            if (usePreviousPickRay(pickRay.direction, previousPickRay.direction, planeNormal)) {
                pickRay = previousPickRay;
            }
            
            var controllerPose = getControllerWorldLocation(that.triggeredHand, true);
            var controllerTrigger = HMD.isHMDAvailable() && HMD.isHandControllerAvailable() && 
                                    controllerPose.valid && that.triggered();

            // Are we using handControllers or Mouse - only relevant for 3D tools
            var vector = null;
            var newPick = null;
            if (controllerTrigger && directionFor3DStretch) {
                localDeltaPivot = deltaPivot3D;
                newPick = pickRay.origin;
                vector = Vec3.subtract(newPick, lastPick3D);
                vector = Vec3.multiplyQbyV(Quat.inverse(rotation), vector);
                if (distanceFor3DStretch > DISTANCE_INFLUENCE_THRESHOLD) {
                    // Range of Motion
                    vector = Vec3.multiply(distanceFor3DStretch , vector);
                }
                localSigns = directionFor3DStretch;
            } else {
                newPick = rayPlaneIntersection(pickRay, pickRayPosition, planeNormal);
                vector = Vec3.subtract(newPick, lastPick);
                vector = Vec3.multiplyQbyV(Quat.inverse(rotation), vector);
                vector = vec3Mult(mask, vector);
            }
            
            vector = grid.snapToSpacing(vector);
    
            var changeInDimensions = Vec3.multiply(NEGATE_VECTOR, vec3Mult(localSigns, vector));
            
            var newDimensions;
            if (proportional) {
                var viewportDimensions = Controller.getViewportDimensions();
                var toCameraDistance = getDistanceToCamera(position);   
                var dimensionsMultiple = toCameraDistance * STRETCH_DIRECTION_ALL_CAMERA_DISTANCE_MULTIPLE; 
                
                var dimensionChange;
                if (controllerTrigger) {
                    var controllerPosition = getControllerAvatarFramePositionFromPickRay(pickRay);
                    var vecControllerDifference = Vec3.subtract(controllerPosition, beginControllerPosition);
                    var controllerDifference = vecControllerDifference.x + vecControllerDifference.y + 
                                               vecControllerDifference.z;
                    dimensionChange = controllerDifference * dimensionsMultiple;
                } else {
                    var mouseXDifference = (event.x - beginMouseEvent.x) / viewportDimensions.x;
                    var mouseYDifference = (beginMouseEvent.y - event.y) / viewportDimensions.y;
                    var mouseDifference = mouseXDifference + mouseYDifference;
                    dimensionChange = mouseDifference * dimensionsMultiple;
                }
 
                var averageInitialDimension = (initialDimensions.x + initialDimensions.y + initialDimensions.z) / 3;
                percentChange = dimensionChange / averageInitialDimension;
                percentChange += 1.0;
                newDimensions = Vec3.multiply(percentChange, initialDimensions);
                newDimensions.x = Math.abs(newDimensions.x);
                newDimensions.y = Math.abs(newDimensions.y);
                newDimensions.z = Math.abs(newDimensions.z);
            } else {
                newDimensions = Vec3.sum(initialDimensions, changeInDimensions);
            }

            var minimumDimension = directionEnum ===
                STRETCH_DIRECTION.ALL ? STRETCH_ALL_MINIMUM_DIMENSION : STRETCH_MINIMUM_DIMENSION; 
            if (newDimensions.x < minimumDimension) {
                newDimensions.x = minimumDimension;
                changeInDimensions.x = minimumDimension - initialDimensions.x;
            }
            if (newDimensions.y < minimumDimension) {
                newDimensions.y = minimumDimension;
                changeInDimensions.y = minimumDimension - initialDimensions.y;
            }
            if (newDimensions.z < minimumDimension) {
                newDimensions.z = minimumDimension;
                changeInDimensions.z = minimumDimension - initialDimensions.z;
            }

            var changeInPosition = Vec3.multiplyQbyV(rotation, vec3Mult(localDeltaPivot, changeInDimensions));
            if (proportional) {
                changeInPosition = { x: 0, y: 0, z: 0 };
            }
            var newPosition = Vec3.sum(initialPosition, changeInPosition);
    
            Entities.editEntity(SelectionManager.selections[0], {
                position: newPosition,
                dimensions: newDimensions
            });
                
            var wantDebug = false;
            if (wantDebug) {
                print(stretchMode);
                Vec3.print("                 vector:", vector);
                Vec3.print("            changeInDimensions:", changeInDimensions);
                Vec3.print("                 newDimensions:", newDimensions);
                Vec3.print("              changeInPosition:", changeInPosition);
                Vec3.print("                   newPosition:", newPosition);
            }
            
            previousPickRay = pickRay;
    
            SelectionManager._update();
        };// End of onMove def

        return {
            mode: stretchMode,
            onBegin: onBegin,
            onMove: onMove,
            onEnd: onEnd
        };
    }

    function addHandleStretchTool(overlay, mode, directionEnum) {
        var directionVector, offset, stretchPanel, handleStretchCube;
        if (directionEnum === STRETCH_DIRECTION.X) {
            stretchPanel = handleStretchXPanel;
            handleStretchCube = handleStretchXCube;
            directionVector = { x: -1, y: 0, z: 0 };
        } else if (directionEnum === STRETCH_DIRECTION.Y) {
            stretchPanel = handleStretchYPanel;
            handleStretchCube = handleStretchYCube;
            directionVector = { x: 0, y: -1, z: 0 };
        } else if (directionEnum === STRETCH_DIRECTION.Z) {
            stretchPanel = handleStretchZPanel;
            handleStretchCube = handleStretchZCube;
            directionVector = { x: 0, y: 0, z: -1 };
        }
        offset = Vec3.multiply(directionVector, NEGATE_VECTOR);
        var tool = makeStretchTool(mode, directionEnum, directionVector, directionVector, offset, stretchPanel, handleStretchCube);
        return addHandleTool(overlay, tool);
    }

    // TOOL DEFINITION: HANDLE SCALE TOOL   
    function addHandleScaleTool(overlay, mode) {
        var directionVector = { x:0, y:0, z:0 };
        var offset = { x:0, y:0, z:0 };
        var tool = makeStretchTool(mode, STRETCH_DIRECTION.ALL, directionVector, directionVector, offset, null, handleScaleCube);
        return addHandleTool(overlay, tool);
    }

    // FUNCTION: UPDATE ROTATION DEGREES OVERLAY
    function updateRotationDegreesOverlay(angleFromZero, position) {
        var toCameraDistance = getDistanceToCamera(position);
        var overlayProps = {
            position: position,
            dimensions: {
                x: toCameraDistance * ROTATE_DISPLAY_SIZE_X_MULTIPLIER,
                y: toCameraDistance * ROTATE_DISPLAY_SIZE_Y_MULTIPLIER
            },
            lineHeight: toCameraDistance * ROTATE_DISPLAY_LINE_HEIGHT_MULTIPLIER,
            text: normalizeDegrees(-angleFromZero) + "°"
        };
        Overlays.editOverlay(rotationDegreesDisplay, overlayProps);
    }

    // FUNCTION DEF: updateSelectionsRotation
    //    Helper func used by rotation handle tools 
    function updateSelectionsRotation(rotationChange, initialPosition) {
        if (!rotationChange) {
            print("ERROR: entitySelectionTool.updateSelectionsRotation - Invalid arg specified!!");

            // EARLY EXIT
            return;
        }

        // Entities should only reposition if we are rotating multiple selections around
        // the selections center point.  Otherwise, the rotation will be around the entities
        // registration point which does not need repositioning.
        var reposition = (SelectionManager.selections.length > 1);

        // editing a parent will cause all the children to automatically follow along, so don't
        // edit any entity who has an ancestor in SelectionManager.selections
        var toRotate = SelectionManager.selections.filter(function (selection) {
            if (SelectionManager.selections.indexOf(SelectionManager.savedProperties[selection].parentID) >= 0) {
                return false; // a parent is also being moved, so don't issue an edit for this entity
            } else {
                return true;
            }
        });

        for (var i = 0; i < toRotate.length; i++) {
            var entityID = toRotate[i];
            var initialProperties = SelectionManager.savedProperties[entityID];

            var newProperties = {
                rotation: Quat.multiply(rotationChange, initialProperties.rotation)
            };

            if (reposition) {
                var dPos = Vec3.subtract(initialProperties.position, initialPosition);
                dPos = Vec3.multiplyQbyV(rotationChange, dPos);
                newProperties.position = Vec3.sum(initialPosition, dPos);
            }

            Entities.editEntity(entityID, newProperties);
        }
    }

    // TOOL DEFINITION: HANDLE ROTATION TOOL   
    function addHandleRotateTool(overlay, mode, direction) {
        var selectedHandle = null;
        var worldRotation = null;
        var rotationCenter = null;
        var initialRotation = null;
        addHandleTool(overlay, {
            mode: mode,
            onBegin: function(event, pickRay, pickResult) {
                var wantDebug = false;
                if (wantDebug) {
                    print("================== " + getMode() + "(addHandleRotateTool onBegin) -> =======================");
                }

                SelectionManager.saveProperties();
                that.resetPreviousHandleColor();
    
                that.setHandleTranslateVisible(false);
                that.setHandleRotatePitchVisible(direction === ROTATE_DIRECTION.PITCH);
                that.setHandleRotateYawVisible(direction === ROTATE_DIRECTION.YAW);
                that.setHandleRotateRollVisible(direction === ROTATE_DIRECTION.ROLL);
                that.setHandleStretchVisible(false);
                that.setHandleScaleCubeVisible(false);
                that.setHandleClonerVisible(false);

                if (direction === ROTATE_DIRECTION.PITCH) {
                    rotationNormal = { x: 1, y: 0, z: 0 };
                    worldRotation = worldRotationY;
                    selectedHandle = handleRotatePitchRing;
                } else if (direction === ROTATE_DIRECTION.YAW) {
                    rotationNormal = { x: 0, y: 1, z: 0 };
                    worldRotation = worldRotationZ;
                    selectedHandle = handleRotateYawRing;
                } else if (direction === ROTATE_DIRECTION.ROLL) {
                    rotationNormal = { x: 0, y: 0, z: 1 };
                    worldRotation = worldRotationX;
                    selectedHandle = handleRotateRollRing;
                }

                Overlays.editOverlay(selectedHandle, { 
                    hasTickMarks: true,
                    solid: false,
                    innerRadius: ROTATE_RING_SELECTED_INNER_RADIUS
                });

                initialRotation = spaceMode === SPACE_LOCAL ? SelectionManager.localRotation : SelectionManager.worldRotation;
                rotationNormal = Vec3.multiplyQbyV(initialRotation, rotationNormal);

                rotationCenter = SelectionManager.worldPosition;

                Overlays.editOverlay(rotationDegreesDisplay, { visible: true });
                Overlays.editOverlay(handleRotateCurrentRing, {
                    position: rotationCenter,
                    rotation: worldRotation,
                    startAt: 0,
                    endAt: 0,
                    visible: true
                });

                // editOverlays may not have committed rotation changes.
                // Compute zero position based on where the overlay will be eventually.
                var result = rayPlaneIntersection(pickRay, rotationCenter, rotationNormal);
                // In case of a parallel ray, this will be null, which will cause early-out
                // in the onMove helper.
                rotationZero = result;

                var rotationCenterToZero = Vec3.subtract(rotationZero, rotationCenter);
                var rotationCenterToZeroLength = Vec3.length(rotationCenterToZero);
                rotationDegreesPosition = Vec3.sum(rotationCenter, Vec3.multiply(Vec3.normalize(rotationCenterToZero), 
                                                   rotationCenterToZeroLength * ROTATE_DISPLAY_DISTANCE_MULTIPLIER));
                updateRotationDegreesOverlay(0, rotationDegreesPosition);

                if (wantDebug) {
                    print("================== " + getMode() + "(addHandleRotateTool onBegin) <- =======================");
                }
            },
            onEnd: function(event, reason) {
                var wantDebug = false;
                if (wantDebug) {
                    print("================== " + getMode() + "(addHandleRotateTool onEnd) -> =======================");
                }
                Overlays.editOverlay(rotationDegreesDisplay, { visible: false });
                Overlays.editOverlay(selectedHandle, { 
                    hasTickMarks: false,
                    solid: true,
                    innerRadius: ROTATE_RING_IDLE_INNER_RADIUS
                });
                Overlays.editOverlay(handleRotateCurrentRing, { visible: false });
                pushCommandForSelections();
                if (wantDebug) {
                    print("================== " + getMode() + "(addHandleRotateTool onEnd) <- =======================");
                }
            },
            onMove: function(event) {
                if (!rotationZero) {
                    print("ERROR: entitySelectionTool.addHandleRotateTool.onMove - " +
                          "Invalid RotationZero Specified (missed rotation target plane?)");

                    // EARLY EXIT
                    return;
                }
                
                var wantDebug = false;
                if (wantDebug) {
                    print("================== "+ getMode() + "(addHandleRotateTool onMove) -> =======================");
                    Vec3.print("    rotationZero: ", rotationZero);
                }

                var pickRay = generalComputePickRay(event.x, event.y);
                var result = rayPlaneIntersection(pickRay, rotationCenter, rotationNormal);
                if (result) {
                    var centerToZero = Vec3.subtract(rotationZero, rotationCenter);
                    var centerToIntersect = Vec3.subtract(result, rotationCenter);

                    if (wantDebug) {
                        Vec3.print("    RotationNormal:    ", rotationNormal);
                        Vec3.print("    rotationZero:           ", rotationZero);
                        Vec3.print("    rotationCenter:         ", rotationCenter);
                        Vec3.print("    intersect:         ", result);
                        Vec3.print("    centerToZero:      ", centerToZero);
                        Vec3.print("    centerToIntersect: ", centerToIntersect);
                    }

                    // Note: orientedAngle which wants normalized centerToZero and centerToIntersect
                    //             handles that internally, so it's to pass unnormalized vectors here.
                    var angleFromZero = Vec3.orientedAngle(centerToZero, centerToIntersect, rotationNormal);        
                    var snapAngle = ctrlPressed ? ROTATE_CTRL_SNAP_ANGLE : ROTATE_DEFAULT_SNAP_ANGLE;
                    angleFromZero = Math.floor(angleFromZero / snapAngle) * snapAngle;
                    var rotationChange = Quat.angleAxis(angleFromZero, rotationNormal);
                    updateSelectionsRotation(rotationChange, rotationCenter);
                    updateRotationDegreesOverlay(-angleFromZero, rotationDegreesPosition);

                    var startAtCurrent = 0;
                    var endAtCurrent = angleFromZero;
                    var maxDegrees = 360;
                    if (angleFromZero < 0) {
                        startAtCurrent = maxDegrees + angleFromZero;
                        endAtCurrent = maxDegrees;
                    }
                    Overlays.editOverlay(handleRotateCurrentRing, {
                        startAt: startAtCurrent,
                        endAt: endAtCurrent
                    });

                    // not sure why but this seems to be needed to fix an reverse rotation for yaw ring only
                    if (direction === ROTATE_DIRECTION.YAW) {
                        if (spaceMode === SPACE_LOCAL) {
                            Overlays.editOverlay(handleRotateCurrentRing, { rotation: worldRotationZ });
                        } else {
                            var rotationDegrees = 90;
                            Overlays.editOverlay(handleRotateCurrentRing, { 
                                rotation: Quat.fromPitchYawRollDegrees(-rotationDegrees, 0, 0) 
                            });
                        }
                    }
                }

                if (wantDebug) {
                    print("================== "+ getMode() + "(addHandleRotateTool onMove) <- =======================");
                }
            }
        });
    }

    // TOOL DEFINITION: HANDLE CLONER
    addHandleTool(handleCloner, {
        mode: "CLONE",
        onBegin: function(event, pickRay, pickResult) {
            var doClone = true;
            translateXZTool.onBegin(event,pickRay,pickResult,doClone);
        },
        elevation: function (event) {
            translateXZTool.elevation(event);
        },
    
        onEnd: function (event) {
            translateXZTool.onEnd(event);
        },
    
        onMove: function (event) {
            translateXZTool.onMove(event);
        }
    });

    addHandleTool(iconSelectionBox, {
        mode: "TRANSLATE_XZ",
        onBegin: function (event, pickRay, pickResult) {
            translateXZTool.onBegin(event, pickRay, pickResult, false);
        },
        elevation: function (event) {
            translateXZTool.elevation(event);
        },
        onEnd: function (event) {
            translateXZTool.onEnd(event);
        },
        onMove: function (event) {
            translateXZTool.onMove(event);
        }
    });

    addHandleTranslateTool(handleTranslateXCone, "TRANSLATE_X", TRANSLATE_DIRECTION.X);
    addHandleTranslateTool(handleTranslateXCylinder, "TRANSLATE_X", TRANSLATE_DIRECTION.X);
    addHandleTranslateTool(handleTranslateYCone, "TRANSLATE_Y", TRANSLATE_DIRECTION.Y);
    addHandleTranslateTool(handleTranslateYCylinder, "TRANSLATE_Y", TRANSLATE_DIRECTION.Y);
    addHandleTranslateTool(handleTranslateZCone, "TRANSLATE_Z", TRANSLATE_DIRECTION.Z);
    addHandleTranslateTool(handleTranslateZCylinder, "TRANSLATE_Z", TRANSLATE_DIRECTION.Z);

    addHandleRotateTool(handleRotatePitchRing, "ROTATE_PITCH", ROTATE_DIRECTION.PITCH);
    addHandleRotateTool(handleRotateYawRing, "ROTATE_YAW", ROTATE_DIRECTION.YAW);
    addHandleRotateTool(handleRotateRollRing, "ROTATE_ROLL", ROTATE_DIRECTION.ROLL);

    addHandleStretchTool(handleStretchXCube, "STRETCH_X", STRETCH_DIRECTION.X);
    addHandleStretchTool(handleStretchYCube, "STRETCH_Y", STRETCH_DIRECTION.Y);
    addHandleStretchTool(handleStretchZCube, "STRETCH_Z", STRETCH_DIRECTION.Z);

    addHandleScaleTool(handleScaleCube, "SCALE");
    
    return that;
}());
