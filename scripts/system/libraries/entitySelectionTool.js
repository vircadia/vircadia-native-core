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
   getMainTabletIDs, getControllerWorldLocation */

var SPACE_LOCAL = "local";
var SPACE_WORLD = "world";
var HIGHLIGHT_LIST_NAME = "editHandleHighlightList";

Script.include([
    "./controllers.js",
    "./utils.js"
]);

SelectionManager = (function() {
    var that = {};

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
            if (wantDebug) {
                print("setting selection to " + messageParsed.entityID);
            }
            that.setSelections([messageParsed.entityID]);
        } else if (messageParsed.method === "clearSelection") {
            that.clearSelections();
        } else if (messageParsed.method === "pointingAt") {
            if (messageParsed.rightHand) {
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
            that.savedProperties[entityID] = Entities.getEntityProperties(entityID);
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

    that.duplicateSelection = function() {
        var entitiesToDuplicate = [];
        var duplicatedEntityIDs = [];
        var duplicatedChildrenWithOldParents = [];
        var originalEntityToNewEntityID = [];
        
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
                properties = Entities.getEntityProperties(originalEntityID);
            }
            if (!properties.locked && (!properties.clientOnly || properties.owningAvatarID === MyAvatar.sessionUUID)) {
                var newEntityID = Entities.addEntity(properties);
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
            properties = Entities.getEntityProperties(that.selections[0]);
            that.localDimensions = properties.dimensions;
            that.localPosition = properties.position;
            that.localRotation = properties.rotation;
            that.localRegistrationPoint = properties.registrationPoint;

            that.worldDimensions = properties.boundingBox.dimensions;
            that.worldPosition = properties.boundingBox.center;
            that.worldRotation = properties.boundingBox.rotation;

            that.entityType = properties.type;
            
            if (selectionUpdated) {
                SelectionDisplay.setSpaceMode(SPACE_LOCAL);
            }
        } else {
            that.localRotation = null;
            that.localDimensions = null;
            that.localPosition = null;

            properties = Entities.getEntityProperties(that.selections[0]);

            that.entityType = properties.type;

            var brn = properties.boundingBox.brn;
            var tfl = properties.boundingBox.tfl;

            for (var i = 1; i < that.selections.length; i++) {
                properties = Entities.getEntityProperties(that.selections[i]);
                var bb = properties.boundingBox;
                brn.x = Math.min(bb.brn.x, brn.x);
                brn.y = Math.min(bb.brn.y, brn.y);
                brn.z = Math.min(bb.brn.z, brn.z);
                tfl.x = Math.max(bb.tfl.x, tfl.x);
                tfl.y = Math.max(bb.tfl.y, tfl.y);
                tfl.z = Math.max(bb.tfl.z, tfl.z);
            }

            that.localDimensions = null;
            that.localPosition = null;
            that.worldDimensions = {
                x: tfl.x - brn.x,
                y: tfl.y - brn.y,
                z: tfl.z - brn.z
            };
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
    var COLOR_SCALE_EDGE = { red: 87, green: 87, blue: 87 };
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
    var ROTATE_RING_IDLE_INNER_RADIUS = 0.95;
    var ROTATE_RING_SELECTED_INNER_RADIUS = 0.9;

    // These are multipliers for sizing the rotation degrees display while rotating an entity
    var ROTATE_DISPLAY_DISTANCE_MULTIPLIER = 2;
    var ROTATE_DISPLAY_SIZE_X_MULTIPLIER = 0.2;
    var ROTATE_DISPLAY_SIZE_Y_MULTIPLIER = 0.09;
    var ROTATE_DISPLAY_LINE_HEIGHT_MULTIPLIER = 0.07;

    var STRETCH_SPHERE_OFFSET = 0.06;
    var STRETCH_SPHERE_CAMERA_DISTANCE_MULTIPLE = 0.01;
    var STRETCH_MINIMUM_DIMENSION = 0.001;
    var STRETCH_ALL_MINIMUM_DIMENSION = 0.01;
    var STRETCH_DIRECTION_ALL_CAMERA_DISTANCE_MULTIPLE = 6;
    var STRETCH_PANEL_WIDTH = 0.01;

    var SCALE_CUBE_OFFSET = 0.5;
    var SCALE_CUBE_CAMERA_DISTANCE_MULTIPLE = 0.0125;

    var CLONER_OFFSET = { x: 0.9, y: -0.9, z: 0.9 };    
    
    var CTRL_KEY_CODE = 16777249;

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

    var previousHandle = null;
    var previousHandleHelper = null;
    var previousHandleColor;

    var ctrlPressed = false;

    that.replaceCollisionsAfterStretch = false;

    var handlePropertiesTranslateArrowCones = {
        shape: "Cone",
        solid: true,
        visible: false,
        ignoreRayIntersection: false,
        drawInFront: true
    };
    var handlePropertiesTranslateArrowCylinders = {
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

    var handlePropertiesStretchSpheres = {
        shape: "Sphere",
        solid: true,
        visible: false,
        ignoreRayIntersection: false,
        drawInFront: true
    };
    var handleStretchXSphere = Overlays.addOverlay("shape", handlePropertiesStretchSpheres);
    Overlays.editOverlay(handleStretchXSphere, { color: COLOR_RED });
    var handleStretchYSphere = Overlays.addOverlay("shape", handlePropertiesStretchSpheres);
    Overlays.editOverlay(handleStretchYSphere, { color: COLOR_GREEN });
    var handleStretchZSphere = Overlays.addOverlay("shape", handlePropertiesStretchSpheres);
    Overlays.editOverlay(handleStretchZSphere, { color: COLOR_BLUE });

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

    var handlePropertiesScaleCubes = {
        size: 0.025,
        color: COLOR_SCALE_CUBE,
        solid: true,
        visible: false,
        ignoreRayIntersection: false,
        drawInFront: true,
        borderSize: 1.4
    };
    var handleScaleLBNCube = Overlays.addOverlay("cube", handlePropertiesScaleCubes); // (-x, -y, -z)
    var handleScaleRBNCube = Overlays.addOverlay("cube", handlePropertiesScaleCubes); // ( x, -y, -z)
    var handleScaleLBFCube = Overlays.addOverlay("cube", handlePropertiesScaleCubes); // (-x, -y,  z)
    var handleScaleRBFCube = Overlays.addOverlay("cube", handlePropertiesScaleCubes); // ( x, -y,  z)
    var handleScaleLTNCube = Overlays.addOverlay("cube", handlePropertiesScaleCubes); // (-x,  y, -z)
    var handleScaleRTNCube = Overlays.addOverlay("cube", handlePropertiesScaleCubes); // ( x,  y, -z)
    var handleScaleLTFCube = Overlays.addOverlay("cube", handlePropertiesScaleCubes); // (-x,  y,  z)
    var handleScaleRTFCube = Overlays.addOverlay("cube", handlePropertiesScaleCubes); // ( x,  y,  z)

    var handlePropertiesScaleEdge = {
        color: COLOR_SCALE_EDGE,
        visible: false,
        ignoreRayIntersection: true,
        drawInFront: true,
        lineWidth: 0.2
    };
    var handleScaleTREdge = Overlays.addOverlay("line3d", handlePropertiesScaleEdge);
    var handleScaleTLEdge = Overlays.addOverlay("line3d", handlePropertiesScaleEdge);
    var handleScaleTFEdge = Overlays.addOverlay("line3d", handlePropertiesScaleEdge);
    var handleScaleTNEdge = Overlays.addOverlay("line3d", handlePropertiesScaleEdge);
    var handleScaleBREdge = Overlays.addOverlay("line3d", handlePropertiesScaleEdge);
    var handleScaleBLEdge = Overlays.addOverlay("line3d", handlePropertiesScaleEdge);
    var handleScaleBFEdge = Overlays.addOverlay("line3d", handlePropertiesScaleEdge);
    var handleScaleBNEdge = Overlays.addOverlay("line3d", handlePropertiesScaleEdge);
    var handleScaleNREdge = Overlays.addOverlay("line3d", handlePropertiesScaleEdge);
    var handleScaleNLEdge = Overlays.addOverlay("line3d", handlePropertiesScaleEdge);
    var handleScaleFREdge = Overlays.addOverlay("line3d", handlePropertiesScaleEdge);
    var handleScaleFLEdge = Overlays.addOverlay("line3d", handlePropertiesScaleEdge);

    var handleCloner = Overlays.addOverlay("cube", {
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
        handleStretchXSphere,
        handleStretchYSphere,
        handleStretchZSphere,
        handleStretchXPanel,
        handleStretchYPanel,
        handleStretchZPanel,
        handleScaleLBNCube,
        handleScaleRBNCube,
        handleScaleLBFCube,
        handleScaleRBFCube,
        handleScaleLTNCube,
        handleScaleRTNCube,
        handleScaleLTFCube,
        handleScaleRTFCube,
        handleScaleTREdge,
        handleScaleTLEdge,
        handleScaleTFEdge,
        handleScaleTNEdge,
        handleScaleBREdge,
        handleScaleBLEdge,
        handleScaleBFEdge,
        handleScaleBNEdge,
        handleScaleNREdge,
        handleScaleNLEdge,
        handleScaleFREdge,
        handleScaleFLEdge,
        handleCloner,
        selectionBox,
        iconSelectionBox
    ];

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

    overlayNames[handleStretchXSphere] = "handleStretchXSphere";
    overlayNames[handleStretchYSphere] = "handleStretchYSphere";
    overlayNames[handleStretchZSphere] = "handleStretchZSphere";
    overlayNames[handleStretchXPanel] = "handleStretchXPanel";
    overlayNames[handleStretchYPanel] = "handleStretchYPanel";
    overlayNames[handleStretchZPanel] = "handleStretchZPanel";

    overlayNames[handleScaleLBNCube] = "handleScaleLBNCube";
    overlayNames[handleScaleRBNCube] = "handleScaleRBNCube";
    overlayNames[handleScaleLBFCube] = "handleScaleLBFCube";
    overlayNames[handleScaleRBFCube] = "handleScaleRBFCube";
    overlayNames[handleScaleLTNCube] = "handleScaleLTNCube";
    overlayNames[handleScaleRTNCube] = "handleScaleRTNCube";
    overlayNames[handleScaleLTFCube] = "handleScaleLTFCube";
    overlayNames[handleScaleRTFCube] = "handleScaleRTFCube";

    overlayNames[handleScaleTREdge] = "handleScaleTREdge";
    overlayNames[handleScaleTLEdge] = "handleScaleTLEdge";
    overlayNames[handleScaleTFEdge] = "handleScaleTFEdge";
    overlayNames[handleScaleTNEdge] = "handleScaleTNEdge";
    overlayNames[handleScaleBREdge] = "handleScaleBREdge";
    overlayNames[handleScaleBLEdge] = "handleScaleBLEdge";
    overlayNames[handleScaleBFEdge] = "handleScaleBFEdge";
    overlayNames[handleScaleBNEdge] = "handleScaleBNEdge";
    overlayNames[handleScaleNREdge] = "handleScaleNREdge";
    overlayNames[handleScaleNLEdge] = "handleScaleNLEdge";
    overlayNames[handleScaleFREdge] = "handleScaleFREdge";
    overlayNames[handleScaleFLEdge] = "handleScaleFLEdge";

    overlayNames[handleCloner] = "handleCloner";
    overlayNames[selectionBox] = "selectionBox";
    overlayNames[iconSelectionBox] = "iconSelectionBox";

    var activeTool = null;
    var handleTools = {};

    // We get mouseMoveEvents from the handControllers, via handControllerPointer.
    // But we dont' get mousePressEvents.
    that.triggerMapping = Controller.newMapping(Script.resolvePath('') + '-click');
    Script.scriptEnding.connect(that.triggerMapping.disable);
    that.TRIGGER_GRAB_VALUE = 0.85; //  From handControllerGrab/Pointer.js. Should refactor.
    that.TRIGGER_ON_VALUE = 0.4;
    that.TRIGGER_OFF_VALUE = 0.15;
    that.triggered = false;
    var activeHand = Controller.Standard.RightHand;
    function makeTriggerHandler(hand) {
        return function (value) {
            if (!that.triggered && (value > that.TRIGGER_GRAB_VALUE)) { // should we smooth?
                that.triggered = true;
                if (activeHand !== hand) {
                    // No switching while the other is already triggered, so no need to release.
                    activeHand = (activeHand === Controller.Standard.RightHand) ?
                        Controller.Standard.LeftHand : Controller.Standard.RightHand;
                }
                var pointingAtDesktopWindow = (hand === Controller.Standard.RightHand && 
                                               SelectionManager.pointingAtDesktopWindowRight) ||
                                              (hand === Controller.Standard.LeftHand && 
                                               SelectionManager.pointingAtDesktopWindowLeft);
                var pointingAtTablet = (hand === Controller.Standard.RightHand && SelectionManager.pointingAtTabletRight) ||
                                       (hand === Controller.Standard.LeftHand && SelectionManager.pointingAtTabletLeft);
                if (pointingAtDesktopWindow || pointingAtTablet) {
                    return;
                }
                that.mousePressEvent({});
            } else if (that.triggered && (value < that.TRIGGER_OFF_VALUE)) {
                that.triggered = false;
                that.mouseReleaseEvent({});
            }
        };
    }
    that.triggerMapping.from(Controller.Standard.RT).peek().to(makeTriggerHandler(Controller.Standard.RightHand));
    that.triggerMapping.from(Controller.Standard.LT).peek().to(makeTriggerHandler(Controller.Standard.LeftHand));

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

    // FUNCTION: MOUSE PRESS EVENT
    that.mousePressEvent = function (event) {
        var wantDebug = false;
        if (wantDebug) {
            print("=============== eST::MousePressEvent BEG =======================");
        }
        if (!event.isLeftButton && !that.triggered) {
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

    // FUNCTION: MOUSE MOVE EVENT
    that.mouseMoveEvent = function(event) {
        var wantDebug = false;
        if (wantDebug) {
            print("=============== eST::MouseMoveEvent BEG =======================");
        }
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
                case handleStretchXSphere:
                    pickedColor = COLOR_RED;
                    highlightNeeded = true;
                    break;
                case handleTranslateYCone:
                case handleTranslateYCylinder:
                case handleRotateYawRing:
                case handleStretchYSphere:
                    pickedColor = COLOR_GREEN;
                    highlightNeeded = true;
                    break;
                case handleTranslateZCone:
                case handleTranslateZCylinder:
                case handleRotateRollRing:
                case handleStretchZSphere:
                    pickedColor = COLOR_BLUE;
                    highlightNeeded = true;
                    break;
                case handleScaleLBNCube:
                case handleScaleRBNCube:
                case handleScaleLBFCube:
                case handleScaleRBFCube:
                case handleScaleLTNCube:
                case handleScaleRTNCube:
                case handleScaleLTFCube:
                case handleScaleRTFCube:
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
    that.keyReleaseEvent = function(key) {
        if (key.key === CTRL_KEY_CODE) {
            ctrlPressed = false;
            that.updateActiveRotateRing();
        }
    };

    // Triggers notification on specific key driven events
    that.keyPressEvent = function(key) {
        if (key.key === CTRL_KEY_CODE) {
            ctrlPressed = true;
            that.updateActiveRotateRing();
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
            var controllerPose = getControllerWorldLocation(activeHand, true);
            var hand = (activeHand === Controller.Standard.LeftHand) ? 0 : 1;
            if (controllerPose.valid && lastControllerPoses[hand].valid && that.triggered) {
                if (!Vec3.equal(controllerPose.position, lastControllerPoses[hand].position) ||
                    !Vec3.equal(controllerPose.rotation, lastControllerPoses[hand].rotation)) {
                    that.mouseMoveEvent({});
                }
            }
            lastControllerPoses[hand] = controllerPose;
        }
    };

    function controllerComputePickRay() {
        var controllerPose = getControllerWorldLocation(activeHand, true);
        if (controllerPose.valid && that.triggered) {
            var controllerPosition = controllerPose.translation;
            // This gets point direction right, but if you want general quaternion it would be more complicated:
            var controllerDirection = Quat.getUp(controllerPose.rotation);
            return {origin: controllerPosition, direction: controllerDirection};
        }
    }

    function generalComputePickRay(x, y) {
        return controllerComputePickRay() || Camera.computePickRay(x, y);
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

            // UPDATE SCALE CUBES
            var scaleCubeOffsetX = SCALE_CUBE_OFFSET * dimensions.x;
            var scaleCubeOffsetY = SCALE_CUBE_OFFSET * dimensions.y;
            var scaleCubeOffsetZ = SCALE_CUBE_OFFSET * dimensions.z;
            var scaleCubeRotation = spaceMode === SPACE_LOCAL ? rotation : Quat.IDENTITY;
            var scaleLBNCubePosition = { x: -scaleCubeOffsetX, y: -scaleCubeOffsetY, z: -scaleCubeOffsetZ };
            scaleLBNCubePosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, scaleLBNCubePosition));
            var scaleLBNCubeToCamera = getDistanceToCamera(scaleLBNCubePosition);
            var scaleRBNCubePosition = { x: scaleCubeOffsetX, y: -scaleCubeOffsetY, z: -scaleCubeOffsetZ };
            scaleRBNCubePosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, scaleRBNCubePosition));
            var scaleRBNCubeToCamera = getDistanceToCamera(scaleRBNCubePosition);
            var scaleLBFCubePosition = { x: -scaleCubeOffsetX, y: -scaleCubeOffsetY, z: scaleCubeOffsetZ };
            scaleLBFCubePosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, scaleLBFCubePosition));
            var scaleLBFCubeToCamera = getDistanceToCamera(scaleLBFCubePosition);
            var scaleRBFCubePosition = { x: scaleCubeOffsetX, y: -scaleCubeOffsetY, z: scaleCubeOffsetZ };
            scaleRBFCubePosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, scaleRBFCubePosition));
            var scaleRBFCubeToCamera = getDistanceToCamera(scaleRBFCubePosition);
            var scaleLTNCubePosition = { x: -scaleCubeOffsetX, y: scaleCubeOffsetY, z: -scaleCubeOffsetZ };
            scaleLTNCubePosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, scaleLTNCubePosition));
            var scaleLTNCubeToCamera = getDistanceToCamera(scaleLTNCubePosition);
            var scaleRTNCubePosition = { x: scaleCubeOffsetX, y: scaleCubeOffsetY, z: -scaleCubeOffsetZ };
            scaleRTNCubePosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, scaleRTNCubePosition));
            var scaleRTNCubeToCamera = getDistanceToCamera(scaleRTNCubePosition);
            var scaleLTFCubePosition = { x: -scaleCubeOffsetX, y: scaleCubeOffsetY, z: scaleCubeOffsetZ };
            scaleLTFCubePosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, scaleLTFCubePosition));
            var scaleLTFCubeToCamera = getDistanceToCamera(scaleLTFCubePosition);
            var scaleRTFCubePosition = { x: scaleCubeOffsetX, y: scaleCubeOffsetY, z: scaleCubeOffsetZ };
            scaleRTFCubePosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, scaleRTFCubePosition));
            var scaleRTFCubeToCamera = getDistanceToCamera(scaleRTFCubePosition);
            
            var scaleCubeToCamera = Math.min(scaleLBNCubeToCamera, scaleRBNCubeToCamera, scaleLBFCubeToCamera, 
                                             scaleRBFCubeToCamera, scaleLTNCubeToCamera, scaleRTNCubeToCamera, 
                                             scaleLTFCubeToCamera, scaleRTFCubeToCamera);
            var scaleCubeDimension = scaleCubeToCamera * SCALE_CUBE_CAMERA_DISTANCE_MULTIPLE;
            var scaleCubeDimensions = { x: scaleCubeDimension, y: scaleCubeDimension, z: scaleCubeDimension };

            Overlays.editOverlay(handleScaleLBNCube, { 
                position: scaleLBNCubePosition, 
                rotation: scaleCubeRotation,
                dimensions: scaleCubeDimensions
            });
            Overlays.editOverlay(handleScaleRBNCube, { 
                position: scaleRBNCubePosition, 
                rotation: scaleCubeRotation,
                dimensions: scaleCubeDimensions
            });
            Overlays.editOverlay(handleScaleLBFCube, { 
                position: scaleLBFCubePosition, 
                rotation: scaleCubeRotation,
                dimensions: scaleCubeDimensions
            });
            Overlays.editOverlay(handleScaleRBFCube, { 
                position: scaleRBFCubePosition, 
                rotation: scaleCubeRotation,
                dimensions: scaleCubeDimensions
            });
            Overlays.editOverlay(handleScaleLTNCube, { 
                position: scaleLTNCubePosition, 
                rotation: scaleCubeRotation,
                dimensions: scaleCubeDimensions
            });
            Overlays.editOverlay(handleScaleRTNCube, { 
                position: scaleRTNCubePosition, 
                rotation: scaleCubeRotation,
                dimensions: scaleCubeDimensions
            });
            Overlays.editOverlay(handleScaleLTFCube, { 
                position: scaleLTFCubePosition, 
                rotation: scaleCubeRotation,
                dimensions: scaleCubeDimensions
            });
            Overlays.editOverlay(handleScaleRTFCube, { 
                position: scaleRTFCubePosition, 
                rotation: scaleCubeRotation,
                dimensions: scaleCubeDimensions
            });

            // UPDATE SCALE EDGES
            Overlays.editOverlay(handleScaleTREdge, { start: scaleRTNCubePosition, end: scaleRTFCubePosition });
            Overlays.editOverlay(handleScaleTLEdge, { start: scaleLTNCubePosition, end: scaleLTFCubePosition });
            Overlays.editOverlay(handleScaleTFEdge, { start: scaleLTFCubePosition, end: scaleRTFCubePosition });
            Overlays.editOverlay(handleScaleTNEdge, { start: scaleLTNCubePosition, end: scaleRTNCubePosition });
            Overlays.editOverlay(handleScaleBREdge, { start: scaleRBNCubePosition, end: scaleRBFCubePosition });
            Overlays.editOverlay(handleScaleBLEdge, { start: scaleLBNCubePosition, end: scaleLBFCubePosition });
            Overlays.editOverlay(handleScaleBFEdge, { start: scaleLBFCubePosition, end: scaleRBFCubePosition });
            Overlays.editOverlay(handleScaleBNEdge, { start: scaleLBNCubePosition, end: scaleRBNCubePosition });
            Overlays.editOverlay(handleScaleNREdge, { start: scaleRTNCubePosition, end: scaleRBNCubePosition });
            Overlays.editOverlay(handleScaleNLEdge, { start: scaleLTNCubePosition, end: scaleLBNCubePosition });
            Overlays.editOverlay(handleScaleFREdge, { start: scaleRTFCubePosition, end: scaleRBFCubePosition });
            Overlays.editOverlay(handleScaleFLEdge, { start: scaleLTFCubePosition, end: scaleLBFCubePosition });

            // UPDATE STRETCH SPHERES
            var stretchSphereDimension = rotateDimension * STRETCH_SPHERE_CAMERA_DISTANCE_MULTIPLE / 
                                                           ROTATE_RING_CAMERA_DISTANCE_MULTIPLE;
            var stretchSphereDimensions = { x: stretchSphereDimension, y: stretchSphereDimension, z: stretchSphereDimension };
            var stretchSphereOffset = rotateDimension * STRETCH_SPHERE_OFFSET / ROTATE_RING_CAMERA_DISTANCE_MULTIPLE;
            var stretchXPosition = { x: stretchSphereOffset, y: 0, z: 0 };
            stretchXPosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, stretchXPosition));
            Overlays.editOverlay(handleStretchXSphere, { 
                position: stretchXPosition, 
                dimensions: stretchSphereDimensions 
            });
            var stretchYPosition = { x: 0, y: stretchSphereOffset, z: 0 };
            stretchYPosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, stretchYPosition));
            Overlays.editOverlay(handleStretchYSphere, { 
                position: stretchYPosition, 
                dimensions: stretchSphereDimensions 
            });
            var stretchZPosition = { x: 0, y: 0, z: stretchSphereOffset };
            stretchZPosition = Vec3.sum(position, Vec3.multiplyQbyV(rotation, stretchZPosition));
            Overlays.editOverlay(handleStretchZSphere, { 
                position: stretchZPosition, 
                dimensions: stretchSphereDimensions 
            });

            // UPDATE STRETCH HIGHLIGHT PANELS
            var scaleRBFCubePositionRotated = Vec3.multiplyQbyV(rotationInverse, scaleRBFCubePosition);
            var scaleRTFCubePositionRotated = Vec3.multiplyQbyV(rotationInverse, scaleRTFCubePosition);
            var scaleLTNCubePositionRotated = Vec3.multiplyQbyV(rotationInverse, scaleLTNCubePosition);
            var scaleRTNCubePositionRotated = Vec3.multiplyQbyV(rotationInverse, scaleRTNCubePosition);
            var stretchPanelXDimensions = Vec3.subtract(scaleRTNCubePositionRotated, scaleRBFCubePositionRotated);
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
            var stretchPanelYDimensions = Vec3.subtract(scaleLTNCubePositionRotated, scaleRTFCubePositionRotated);
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
            var stretchPanelZDimensions = Vec3.subtract(scaleLTNCubePositionRotated, scaleRBFCubePositionRotated);
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
        that.setHandleStretchXVisible(showScaleStretch || isActiveTool(handleStretchXSphere));
        that.setHandleStretchYVisible(showScaleStretch || isActiveTool(handleStretchYSphere));
        that.setHandleStretchZVisible(showScaleStretch || isActiveTool(handleStretchZSphere));
        that.setHandleScaleCubeVisible(showScaleStretch || isActiveTool(handleScaleLBNCube) || 
                                       isActiveTool(handleScaleRBNCube) || isActiveTool(handleScaleLBFCube) || 
                                       isActiveTool(handleScaleRBFCube) || isActiveTool(handleScaleLTNCube) || 
                                       isActiveTool(handleScaleRTNCube) || isActiveTool(handleScaleLTFCube) || 
                                       isActiveTool(handleScaleRTFCube) || isActiveTool(handleStretchXSphere) || 
                                       isActiveTool(handleStretchYSphere) || isActiveTool(handleStretchZSphere));

        var showOutlineForZone = (SelectionManager.selections.length === 1 && 
                                    typeof SelectionManager.savedProperties[SelectionManager.selections[0]] !== "undefined" &&
                                    SelectionManager.savedProperties[SelectionManager.selections[0]].type === "Zone");
        that.setHandleScaleEdgeVisible(showOutlineForZone || (!isActiveTool(handleRotatePitchRing) &&
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
        Overlays.editOverlay(handleStretchXSphere, { visible: isVisible });
    };

    that.setHandleStretchYVisible = function(isVisible) {
        Overlays.editOverlay(handleStretchYSphere, { visible: isVisible });
    };

    that.setHandleStretchZVisible = function(isVisible) {
        Overlays.editOverlay(handleStretchZSphere, { visible: isVisible });
    };
    
    // FUNCTION: SET HANDLE SCALE VISIBLE
    that.setHandleScaleVisible = function(isVisible) {
        that.setHandleScaleCubeVisible(isVisible);
        that.setHandleScaleEdgeVisible(isVisible);
    };

    that.setHandleScaleCubeVisible = function(isVisible) {
        Overlays.editOverlay(handleScaleLBNCube, { visible: isVisible });
        Overlays.editOverlay(handleScaleRBNCube, { visible: isVisible });
        Overlays.editOverlay(handleScaleLBFCube, { visible: isVisible });
        Overlays.editOverlay(handleScaleRBFCube, { visible: isVisible });
        Overlays.editOverlay(handleScaleLTNCube, { visible: isVisible });
        Overlays.editOverlay(handleScaleRTNCube, { visible: isVisible });
        Overlays.editOverlay(handleScaleLTFCube, { visible: isVisible });
        Overlays.editOverlay(handleScaleRTFCube, { visible: isVisible });
    };

    that.setHandleScaleEdgeVisible = function(isVisible) {
        Overlays.editOverlay(handleScaleTREdge, { visible: isVisible });
        Overlays.editOverlay(handleScaleTLEdge, { visible: isVisible });
        Overlays.editOverlay(handleScaleTFEdge, { visible: isVisible });
        Overlays.editOverlay(handleScaleTNEdge, { visible: isVisible });
        Overlays.editOverlay(handleScaleBREdge, { visible: isVisible });
        Overlays.editOverlay(handleScaleBLEdge, { visible: isVisible });
        Overlays.editOverlay(handleScaleBFEdge, { visible: isVisible });
        Overlays.editOverlay(handleScaleBNEdge, { visible: isVisible });
        Overlays.editOverlay(handleScaleNREdge, { visible: isVisible });
        Overlays.editOverlay(handleScaleNLEdge, { visible: isVisible });
        Overlays.editOverlay(handleScaleFREdge, { visible: isVisible });
        Overlays.editOverlay(handleScaleFLEdge, { visible: isVisible });
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

            // Duplicate entities if alt is pressed.  This will make a
            // copy of the selected entities and move the _original_ entities, not
            // the new ones.
            if (event.isAlt || doClone) {
                duplicatedEntityIDs = SelectionManager.duplicateSelection();
            } else {
                duplicatedEntityIDs = null;
            }

            isConstrained = false;
            if (wantDebug) {
                print("================== TRANSLATE_XZ(End) <- =======================");
            }
        },
        onEnd: function(event, reason) {
            pushCommandForSelections(duplicatedEntityIDs);
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
                    isConstrained = true;
                }
            } else {
                if (isConstrained) {
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
    
                // Duplicate entities if alt is pressed.  This will make a
                // copy of the selected entities and move the _original_ entities, not
                // the new ones.
                if (event.isAlt) {
                    duplicatedEntityIDs = SelectionManager.duplicateSelection();
                } else {
                    duplicatedEntityIDs = null;
                }
                
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
    function makeStretchTool(stretchMode, directionEnum, directionVec, pivot, offset, stretchPanel, scaleHandle) {
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

        var onBegin = function(event, pickRay, pickResult) {
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
            lastPick = rayPlaneIntersection(pickRay,
                pickRayPosition,
                planeNormal);
                
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
            if (scaleHandle !== null) {
                Overlays.editOverlay(scaleHandle, { color: COLOR_SCALE_CUBE_SELECTED });
            }
            
            var collisionToRemove = "myAvatar";
            if (properties.collidesWith.indexOf(collisionToRemove) > -1) {
                var newCollidesWith = properties.collidesWith.replace(collisionToRemove, "");
                Entities.editEntity(SelectionManager.selections[0], {collidesWith: newCollidesWith});
                that.replaceCollisionsAfterStretch = true;
            }
            
            previousPickRay = pickRay;
        };

        var onEnd = function(event, reason) {    
            if (stretchPanel !== null) {
                Overlays.editOverlay(stretchPanel, { visible: false });
            }
            if (scaleHandle !== null) {
                Overlays.editOverlay(scaleHandle, { color: COLOR_SCALE_CUBE });
            }
            
            if (that.replaceCollisionsAfterStretch) {
                var newCollidesWith = SelectionManager.savedProperties[SelectionManager.selections[0]].collidesWith;
                Entities.editEntity(SelectionManager.selections[0], {collidesWith: newCollidesWith});
                that.replaceCollisionsAfterStretch = false;
            }
            
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

            // Are we using handControllers or Mouse - only relevant for 3D tools
            var controllerPose = getControllerWorldLocation(activeHand, true);
            var vector = null;
            var newPick = null;
            if (HMD.isHMDAvailable() && HMD.isHandControllerAvailable() && 
                    controllerPose.valid && that.triggered && directionFor3DStretch) {
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
            if (directionEnum === STRETCH_DIRECTION.ALL) {  
                var toCameraDistance = getDistanceToCamera(position);   
                var dimensionsMultiple = toCameraDistance * STRETCH_DIRECTION_ALL_CAMERA_DISTANCE_MULTIPLE; 
                changeInDimensions = Vec3.multiply(changeInDimensions, dimensionsMultiple); 
            }

            var newDimensions;
            if (proportional) {
                var absoluteX = Math.abs(changeInDimensions.x);
                var absoluteY = Math.abs(changeInDimensions.y);
                var absoluteZ = Math.abs(changeInDimensions.z);
                var percentChange = 0;
                if (absoluteX > absoluteY && absoluteX > absoluteZ) {
                    percentChange = changeInDimensions.x / initialProperties.dimensions.x;
                    percentChange = changeInDimensions.x / initialDimensions.x;
                } else if (absoluteY > absoluteZ) {
                    percentChange = changeInDimensions.y / initialProperties.dimensions.y;
                    percentChange = changeInDimensions.y / initialDimensions.y;
                } else {
                    percentChange = changeInDimensions.z / initialProperties.dimensions.z;
                    percentChange = changeInDimensions.z / initialDimensions.z;
                }
                percentChange += 1.0;
                newDimensions = Vec3.multiply(percentChange, initialDimensions);
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
            if (directionEnum === STRETCH_DIRECTION.ALL) {
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
        var directionVector, offset, stretchPanel;
        if (directionEnum === STRETCH_DIRECTION.X) {
            stretchPanel = handleStretchXPanel;
            directionVector = { x: -1, y: 0, z: 0 };
        } else if (directionEnum === STRETCH_DIRECTION.Y) {
            stretchPanel = handleStretchYPanel;
            directionVector = { x: 0, y: -1, z: 0 };
        } else if (directionEnum === STRETCH_DIRECTION.Z) {
            stretchPanel = handleStretchZPanel;
            directionVector = { x: 0, y: 0, z: -1 };
        }
        offset = Vec3.multiply(directionVector, NEGATE_VECTOR);
        var tool = makeStretchTool(mode, directionEnum, directionVector, directionVector, offset, stretchPanel, null);
        return addHandleTool(overlay, tool);
    }

    // TOOL DEFINITION: HANDLE SCALE TOOL   
    function addHandleScaleTool(overlay, mode, directionEnum) {
        var directionVector, offset, selectedHandle;
        if (directionEnum === SCALE_DIRECTION.LBN) {
            directionVector = { x: 1, y: 1, z: 1 };
            selectedHandle = handleScaleLBNCube;
        } else if (directionEnum === SCALE_DIRECTION.RBN) {
            directionVector = { x: -1, y: 1, z: 1 };
            selectedHandle = handleScaleRBNCube;
        } else if (directionEnum === SCALE_DIRECTION.LBF) {
            directionVector = { x: 1, y: 1, z: -1 };
            selectedHandle = handleScaleLBFCube;
        } else if (directionEnum === SCALE_DIRECTION.RBF) {
            directionVector = { x: -1, y: 1, z: -1 };
            selectedHandle = handleScaleRBFCube;
        } else if (directionEnum === SCALE_DIRECTION.LTN) { 
            directionVector = { x: 1, y: -1, z: 1 };
            selectedHandle = handleScaleLTNCube;
        } else if (directionEnum === SCALE_DIRECTION.RTN) {
            directionVector = { x: -1, y: -1, z: 1 };
            selectedHandle = handleScaleRTNCube;
        } else if (directionEnum === SCALE_DIRECTION.LTF) {
            directionVector = { x: 1, y: -1, z: -1 };
            selectedHandle = handleScaleLTFCube;
        } else if (directionEnum === SCALE_DIRECTION.RTF) {
            directionVector = { x: -1, y: -1, z: -1 };
            selectedHandle = handleScaleRTFCube;
        }
        offset = Vec3.multiply(directionVector, NEGATE_VECTOR);
        var tool = makeStretchTool(mode, STRETCH_DIRECTION.ALL, directionVector, directionVector, offset, null, selectedHandle);
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

    addHandleStretchTool(handleStretchXSphere, "STRETCH_X", STRETCH_DIRECTION.X);
    addHandleStretchTool(handleStretchYSphere, "STRETCH_Y", STRETCH_DIRECTION.Y);
    addHandleStretchTool(handleStretchZSphere, "STRETCH_Z", STRETCH_DIRECTION.Z);

    addHandleScaleTool(handleScaleLBNCube, "SCALE_LBN", SCALE_DIRECTION.LBN);
    addHandleScaleTool(handleScaleRBNCube, "SCALE_RBN", SCALE_DIRECTION.RBN);
    addHandleScaleTool(handleScaleLBFCube, "SCALE_LBF", SCALE_DIRECTION.LBF);
    addHandleScaleTool(handleScaleRBFCube, "SCALE_RBF", SCALE_DIRECTION.RBF);
    addHandleScaleTool(handleScaleLTNCube, "SCALE_LTN", SCALE_DIRECTION.LTN);
    addHandleScaleTool(handleScaleRTNCube, "SCALE_RTN", SCALE_DIRECTION.RTN);
    addHandleScaleTool(handleScaleLTFCube, "SCALE_LTF", SCALE_DIRECTION.LTF);
    addHandleScaleTool(handleScaleRTFCube, "SCALE_RTF", SCALE_DIRECTION.RTF);

    return that;
}());
