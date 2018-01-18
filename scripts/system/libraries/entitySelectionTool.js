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

/* global HIFI_PUBLIC_BUCKET, SPACE_LOCAL, Script, SelectionManager */

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

SPACE_LOCAL = "local";
SPACE_WORLD = "world";

Script.include("./controllers.js");

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
        }

        if (messageParsed.method === "selectEntity") {
            if (wantDebug) {
                print("setting selection to " + messageParsed.entityID);
            }
            that.setSelections([messageParsed.entityID]);
        }
    }

    subscribeToUpdateMessages();

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
            } else if (toggleSelection) {
                that.selections.splice(idx, 1);
            }
        }

        that._update(true);
    };

    that.removeEntity = function(entityID) {
        var idx = that.selections.indexOf(entityID);
        if (idx >= 0) {
            that.selections.splice(idx, 1);
        }
        that._update(true);
    };

    that.clearSelections = function() {
        that.selections = [];
        that._update(true);
    };

    that._update = function(selectionUpdated) {
        var properties = null;
        if (that.selections.length === 0) {
            that.localDimensions = null;
            that.localPosition = null;
            that.worldDimensions = null;
            that.worldPosition = null;
        } else if (that.selections.length === 1) {
            properties = Entities.getEntityProperties(that.selections[0]);
            that.localDimensions = properties.dimensions;
            that.localPosition = properties.position;
            that.localRotation = properties.rotation;
            that.localRegistrationPoint = properties.registrationPoint;

            that.worldDimensions = properties.boundingBox.dimensions;
            that.worldPosition = properties.boundingBox.center;

            SelectionDisplay.setSpaceMode(SPACE_LOCAL);
        } else {
            that.localRotation = null;
            that.localDimensions = null;
            that.localPosition = null;

            properties = Entities.getEntityProperties(that.selections[0]);

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

// Normalize degrees to be in the range (-180, 180]
function normalizeDegrees(degrees) {
    degrees = ((degrees + 180) % 360) - 180;
    if (degrees <= -180) {
        degrees += 360;
    }

    return degrees;
}

// FUNCTION: getRelativeCenterPosition
// Return the enter position of an entity relative to it's registrationPoint
// A registration point of (0.5, 0.5, 0.5) will have an offset of (0, 0, 0)
// A registration point of (1.0, 1.0, 1.0) will have an offset of (-dimensions.x / 2, -dimensions.y / 2, -dimensions.z / 2)
function getRelativeCenterPosition(dimensions, registrationPoint) {
    return {
        x: -dimensions.x * (registrationPoint.x - 0.5),
        y: -dimensions.y * (registrationPoint.y - 0.5),
        z: -dimensions.z * (registrationPoint.z - 0.5)
    };
}

// SELECTION DISPLAY DEFINITION
SelectionDisplay = (function() {
    var that = {};

    var COLOR_GREEN = { red:0, green:255, blue:0 };
    var COLOR_BLUE = { red:0, green:0, blue:255 };
    var COLOR_RED = { red:255, green:0, blue:0 };

    var GRABBER_TRANSLATE_ARROW_CONE_OFFSET = 0.3625;
    var GRABBER_TRANSLATE_ARROW_CYLINDER_OFFSET = 0.3;
    var GRABBER_STRETCH_SPHERE_OFFSET = 0.2;
    var GRABBER_SCALE_CUBE_OFFSET = 0.2;

    var GRABBER_SCALE_CUBE_IDLE_COLOR = { red:120, green:120, blue:120 };
    var GRABBER_SCALE_CUBE_SELECTED_COLOR = { red:0, green:0, blue:0 };
    var GRABBER_SCALE_EDGE_COLOR = { red:120, green:120, blue:120 };

    var SCALE_MINIMUM_DIMENSION = 0.02;

    // These are multipliers for sizing the rotation degrees display while rotating an entity
    var ROTATION_DISPLAY_DISTANCE_MULTIPLIER = 1.0;
    var ROTATION_DISPLAY_SIZE_X_MULTIPLIER = 0.6;
    var ROTATION_DISPLAY_SIZE_Y_MULTIPLIER = 0.18;
    var ROTATION_DISPLAY_LINE_HEIGHT_MULTIPLIER = 0.14;

    var TRANSLATE_DIRECTION = {
        X : 0,
        Y : 1,
        Z : 2
    }

    var ROTATE_DIRECTION = {
        PITCH : 0,
        YAW : 1,
        ROLL : 2
    }

    var SCALE_DIRECTION = {
        LBN : 0,
        RBN : 1,
        LBF : 2,
        RBF : 3,
        LTN : 4,
        RTN : 5,
        LTF : 6,
        RTF : 7
    }

    var spaceMode = SPACE_LOCAL;
    var overlayNames = [];
    var lastCameraPosition = Camera.getPosition();
    var lastCameraOrientation = Camera.getOrientation();
    var lastControllerPoses = [
        getControllerWorldLocation(Controller.Standard.LeftHand, true),
        getControllerWorldLocation(Controller.Standard.RightHand, true)
    ];

    var rotZero;
    var rotationNormal;

    var worldRotationX;
    var worldRotationY;
    var worldRotationZ;

    var activeTool = null;
    var grabberTools = {};

    var grabberPropertiesTranslateArrowCones = {
        shape: "Cone",
        dimensions: { x:0.05, y:0.05, z:0.05 },
        solid: true,
        visible: false,
        ignoreRayIntersection: false,
        drawInFront: true
    };
    var grabberPropertiesTranslateArrowCylinders = {
        shape: "Cylinder",
        dimensions: { x:0.01, y:0.075, z:0.01 },
        solid: true,
        visible: false,
        ignoreRayIntersection: false,
        drawInFront: true
    };
    var grabberTranslateXCone = Overlays.addOverlay("shape", grabberPropertiesTranslateArrowCones);
    var grabberTranslateXCylinder = Overlays.addOverlay("shape", grabberPropertiesTranslateArrowCylinders);
    Overlays.editOverlay(grabberTranslateXCone, { color : COLOR_RED });
    Overlays.editOverlay(grabberTranslateXCylinder, { color : COLOR_RED });
    var grabberTranslateYCone = Overlays.addOverlay("shape", grabberPropertiesTranslateArrowCones);
    var grabberTranslateYCylinder = Overlays.addOverlay("shape", grabberPropertiesTranslateArrowCylinders);
    Overlays.editOverlay(grabberTranslateYCone, { color : COLOR_GREEN });
    Overlays.editOverlay(grabberTranslateYCylinder, { color : COLOR_GREEN });
    var grabberTranslateZCone = Overlays.addOverlay("shape", grabberPropertiesTranslateArrowCones);
    var grabberTranslateZCylinder = Overlays.addOverlay("shape", grabberPropertiesTranslateArrowCylinders);
    Overlays.editOverlay(grabberTranslateZCone, { color : COLOR_BLUE });
    Overlays.editOverlay(grabberTranslateZCylinder, { color : COLOR_BLUE });

    var grabberPropertiesRotateRings = {
        size: 0.5,
        alpha: 1,
        innerRadius: 0.9,
        startAt: 0,
        endAt: 360,
        majorTickMarksAngle: 5,
        majorTickMarksLength: 0.1,
        visible: false,
        ignoreRayIntersection: false,
        drawInFront: true
    };
    var grabberRotatePitchRing = Overlays.addOverlay("circle3d", grabberPropertiesRotateRings);
    Overlays.editOverlay(grabberRotatePitchRing, { 
        color : COLOR_RED,
        majorTickMarksColor: COLOR_RED,
    });
    var grabberRotateYawRing = Overlays.addOverlay("circle3d", grabberPropertiesRotateRings);
    Overlays.editOverlay(grabberRotateYawRing, { 
        color : COLOR_GREEN,
        majorTickMarksColor: COLOR_GREEN,
    });
    var grabberRotateRollRing = Overlays.addOverlay("circle3d", grabberPropertiesRotateRings);
    Overlays.editOverlay(grabberRotateRollRing, { 
        color : COLOR_BLUE,
        majorTickMarksColor: COLOR_BLUE,
    });

    var grabberRotateCurrentRing = Overlays.addOverlay("circle3d", {
        size: 0.5,
        alpha: 1,
        color: { red: 224, green: 67, blue: 36 },
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

    var grabberPropertiesStretchSpheres = {
        shape: "Sphere",
        dimensions: { x:0.02, y:0.02, z:0.02 },
        solid: true,
        visible: false,
        ignoreRayIntersection: false,
        drawInFront: true
    };
    var grabberStretchXSphere = Overlays.addOverlay("shape", grabberPropertiesStretchSpheres);
    Overlays.editOverlay(grabberStretchXSphere, { color : COLOR_RED });
    var grabberStretchYSphere = Overlays.addOverlay("shape", grabberPropertiesStretchSpheres);
    Overlays.editOverlay(grabberStretchYSphere, { color : COLOR_GREEN });
    var grabberStretchZSphere = Overlays.addOverlay("shape", grabberPropertiesStretchSpheres);
    Overlays.editOverlay(grabberStretchZSphere, { color : COLOR_BLUE });

    var grabberPropertiesStretchPanel = {
        shape: "Quad",
        alpha: 0.5,
        dimensions: { x:GRABBER_SCALE_CUBE_OFFSET * 2, y:GRABBER_SCALE_CUBE_OFFSET * 2, z:0.01 },
        solid: true,
        visible: false,
        ignoreRayIntersection: true,
        drawInFront: true,
    }
    var grabberStretchXPanel = Overlays.addOverlay("shape", grabberPropertiesStretchPanel);
    Overlays.editOverlay(grabberStretchXPanel, { color : COLOR_RED });
    var grabberStretchYPanel = Overlays.addOverlay("shape", grabberPropertiesStretchPanel);
    Overlays.editOverlay(grabberStretchYPanel, { color : COLOR_GREEN });
    var grabberStretchZPanel = Overlays.addOverlay("shape", grabberPropertiesStretchPanel);
    Overlays.editOverlay(grabberStretchZPanel, { color : COLOR_BLUE });

    var grabberPropertiesScaleCubes = {
        size: 0.025,
        color: GRABBER_SCALE_CUBE_IDLE_COLOR,
        solid: true,
        visible: false,
        ignoreRayIntersection: false,
        drawInFront: true,
        borderSize: 1.4
    };
    var grabberScaleLBNCube = Overlays.addOverlay("cube", grabberPropertiesScaleCubes); // (-x, -y, -z)
    var grabberScaleRBNCube = Overlays.addOverlay("cube", grabberPropertiesScaleCubes); // (-x, -y,  z)
    var grabberScaleLBFCube = Overlays.addOverlay("cube", grabberPropertiesScaleCubes); // ( x, -y, -z)
    var grabberScaleRBFCube = Overlays.addOverlay("cube", grabberPropertiesScaleCubes); // ( x, -y,  z)
    var grabberScaleLTNCube = Overlays.addOverlay("cube", grabberPropertiesScaleCubes); // (-x,  y, -z)
    var grabberScaleRTNCube = Overlays.addOverlay("cube", grabberPropertiesScaleCubes); // (-x,  y,  z)
    var grabberScaleLTFCube = Overlays.addOverlay("cube", grabberPropertiesScaleCubes); // ( x,  y, -z)
    var grabberScaleRTFCube = Overlays.addOverlay("cube", grabberPropertiesScaleCubes); // ( x,  y,  z)

    var grabberPropertiesScaleEdge = {
        color: GRABBER_SCALE_EDGE_COLOR,
        visible: false,
        ignoreRayIntersection: true,
        drawInFront: true,
        lineWidth: 0.2
    }
    var grabberScaleTREdge = Overlays.addOverlay("line3d", grabberPropertiesScaleEdge);
    var grabberScaleTLEdge = Overlays.addOverlay("line3d", grabberPropertiesScaleEdge);
    var grabberScaleTFEdge = Overlays.addOverlay("line3d", grabberPropertiesScaleEdge);
    var grabberScaleTNEdge = Overlays.addOverlay("line3d", grabberPropertiesScaleEdge);
    var grabberScaleBREdge = Overlays.addOverlay("line3d", grabberPropertiesScaleEdge);
    var grabberScaleBLEdge = Overlays.addOverlay("line3d", grabberPropertiesScaleEdge);
    var grabberScaleBFEdge = Overlays.addOverlay("line3d", grabberPropertiesScaleEdge);
    var grabberScaleBNEdge = Overlays.addOverlay("line3d", grabberPropertiesScaleEdge);
    var grabberScaleNREdge = Overlays.addOverlay("line3d", grabberPropertiesScaleEdge);
    var grabberScaleNLEdge = Overlays.addOverlay("line3d", grabberPropertiesScaleEdge);
    var grabberScaleFREdge = Overlays.addOverlay("line3d", grabberPropertiesScaleEdge);
    var grabberScaleFLEdge = Overlays.addOverlay("line3d", grabberPropertiesScaleEdge);

    var allOverlays = [
        grabberTranslateXCone,
        grabberTranslateXCylinder,
        grabberTranslateYCone,
        grabberTranslateYCylinder,
        grabberTranslateZCone,
        grabberTranslateZCylinder,
        grabberRotatePitchRing,
        grabberRotateYawRing,
        grabberRotateRollRing,
        grabberRotateCurrentRing,
        rotationDegreesDisplay,
        grabberStretchXSphere,
        grabberStretchYSphere,
        grabberStretchZSphere,
        grabberStretchXPanel,
        grabberStretchYPanel,
        grabberStretchZPanel,
        grabberScaleLBNCube,
        grabberScaleRBNCube,
        grabberScaleLBFCube,
        grabberScaleRBFCube,
        grabberScaleLTNCube,
        grabberScaleRTNCube,
        grabberScaleLTFCube,
        grabberScaleRTFCube,
        grabberScaleTREdge,
        grabberScaleTLEdge,
        grabberScaleTFEdge,
        grabberScaleTNEdge,
        grabberScaleBREdge,
        grabberScaleBLEdge,
        grabberScaleBFEdge,
        grabberScaleBNEdge,
        grabberScaleNREdge,
        grabberScaleNLEdge,
        grabberScaleFREdge,
        grabberScaleFLEdge
    ];

    overlayNames[grabberTranslateXCone] = "grabberTranslateXCone";
    overlayNames[grabberTranslateXCylinder] = "grabberTranslateXCylinder";
    overlayNames[grabberTranslateYCone] = "grabberTranslateYCone";
    overlayNames[grabberTranslateYCylinder] = "grabberTranslateYCylinder";
    overlayNames[grabberTranslateZCone] = "grabberTranslateZCone";
    overlayNames[grabberTranslateZCylinder] = "grabberTranslateZCylinder";
    overlayNames[grabberRotatePitchRing] = "grabberRotatePitchRing";
    overlayNames[grabberRotateYawRing] = "grabberRotateYawRing";
    overlayNames[grabberRotateRollRing] = "grabberRotateRollRing";
    overlayNames[grabberRotateCurrentRing] = "grabberRotateCurrentRing";
    overlayNames[rotationDegreesDisplay] = "rotationDegreesDisplay";
    overlayNames[grabberStretchXSphere] = "grabberStretchXSphere";
    overlayNames[grabberStretchYSphere] = "grabberStretchYSphere";
    overlayNames[grabberStretchZSphere] = "grabberStretchZSphere";
    overlayNames[grabberStretchXPanel] = "grabberStretchXPanel";
    overlayNames[grabberStretchYPanel] = "grabberStretchYPanel";
    overlayNames[grabberStretchZPanel] = "grabberStretchZPanel";
    overlayNames[grabberScaleLBNCube] = "grabberScaleLBNCube";
    overlayNames[grabberScaleRBNCube] = "grabberScaleRBNCube";
    overlayNames[grabberScaleLBFCube] = "grabberScaleLBFCube";
    overlayNames[grabberScaleRBFCube] = "grabberScaleRBFCube";
    overlayNames[grabberScaleLTNCube] = "grabberScaleLTNCube";
    overlayNames[grabberScaleRTNCube] = "grabberScaleRTNCube";
    overlayNames[grabberScaleLTFCube] = "grabberScaleLTFCube";
    overlayNames[grabberScaleRTFCube] = "grabberScaleRTFCube";
    overlayNames[grabberScaleTREdge] = "grabberScaleTREdge";
    overlayNames[grabberScaleTLEdge] = "grabberScaleTLEdge";
    overlayNames[grabberScaleTFEdge] = "grabberScaleTFEdge";
    overlayNames[grabberScaleTNEdge] = "grabberScaleTNEdge";
    overlayNames[grabberScaleBREdge] = "grabberScaleBREdge";
    overlayNames[grabberScaleBLEdge] = "grabberScaleBLEdge";
    overlayNames[grabberScaleBFEdge] = "grabberScaleBFEdge";
    overlayNames[grabberScaleBNEdge] = "grabberScaleBNEdge";
    overlayNames[grabberScaleNREdge] = "grabberScaleNREdge";
    overlayNames[grabberScaleNLEdge] = "grabberScaleNLEdge";
    overlayNames[grabberScaleFREdge] = "grabberScaleFREdge";
    overlayNames[grabberScaleFLEdge] = "grabberScaleFLEdge";

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
                if (Reticle.pointingAtSystemOverlay || Overlays.getOverlayAtPoint(Reticle.position)) {
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
    
    function addGrabberTool(overlay, tool) {
        grabberTools[overlay] = tool;
        return tool;
    }

    function addGrabberTranslateTool(overlay, mode, direction) {
        var pickNormal = null;
        var lastPick = null;
        var projectionVector = null;
        addGrabberTool(overlay, {
            mode: mode,
            onBegin: function(event, pickRay, pickResult) {
                if (direction === TRANSLATE_DIRECTION.X) {
                    pickNormal = { x:0, y:0, z:1 };
                } else if (direction === TRANSLATE_DIRECTION.Y) {
                    pickNormal = { x:1, y:0, z:0 };
                } else if (direction === TRANSLATE_DIRECTION.Z) {
                    pickNormal = { x:0, y:1, z:0 };
                }

                var rotation = Entities.getEntityProperties(SelectionManager.selections[0]).rotation;
                pickNormal = Vec3.multiplyQbyV(rotation, pickNormal);

                lastPick = rayPlaneIntersection(pickRay, SelectionManager.worldPosition, pickNormal);
    
                SelectionManager.saveProperties();

                that.setGrabberTranslateXVisible(direction === TRANSLATE_DIRECTION.X);
                that.setGrabberTranslateYVisible(direction === TRANSLATE_DIRECTION.Y);
                that.setGrabberTranslateZVisible(direction === TRANSLATE_DIRECTION.Z);
                that.setGrabberRotateVisible(false);
                that.setGrabberStretchVisible(false);
                that.setGrabberScaleVisible(false);
    
                // Duplicate entities if alt is pressed.  This will make a
                // copy of the selected entities and move the _original_ entities, not
                // the new ones.
                if (event.isAlt) {
                    duplicatedEntityIDs = [];
                    for (var otherEntityID in SelectionManager.savedProperties) {
                        var properties = SelectionManager.savedProperties[otherEntityID];
                        if (!properties.locked) {
                            var entityID = Entities.addEntity(properties);
                            duplicatedEntityIDs.push({
                                entityID: entityID,
                                properties: properties
                            });
                        }
                    }
                } else {
                    duplicatedEntityIDs = null;
                }
            },
            onEnd: function(event, reason) {
                pushCommandForSelections(duplicatedEntityIDs);
            },
            onMove: function(event) {
                pickRay = generalComputePickRay(event.x, event.y);
    
                // translate mode left/right based on view toward entity
                var newIntersection = rayPlaneIntersection(pickRay, SelectionManager.worldPosition, pickNormal);
                var vector = Vec3.subtract(newIntersection, lastPick);
                
                if (direction === TRANSLATE_DIRECTION.X) {
                    projectionVector = { x:1, y:0, z:0 };
                } else if (direction === TRANSLATE_DIRECTION.Y) {
                    projectionVector = { x:0, y:1, z:0 };
                } else if (direction === TRANSLATE_DIRECTION.Z) {
                    projectionVector = { x:0, y:0, z:1 };
                }

                var rotation = Entities.getEntityProperties(SelectionManager.selections[0]).rotation;
                projectionVector = Vec3.multiplyQbyV(rotation, projectionVector);

                var dotVector = Vec3.dot(vector, projectionVector);
                vector = Vec3.multiply(dotVector, projectionVector);
                vector = grid.snapToGrid(vector);
    
                var wantDebug = false;
                if (wantDebug) {
                    print("translateUpDown... ");
                    print("                event.y:" + event.y);
                    Vec3.print("        newIntersection:", newIntersection);
                    Vec3.print("                 vector:", vector);
                    // Vec3.print("            newPosition:", newPosition);
                }
    
                for (var i = 0; i < SelectionManager.selections.length; i++) {
                    var id = SelectionManager.selections[i];
                    var properties = SelectionManager.savedProperties[id];
                    var newPosition = Vec3.sum(properties.position, vector);
                    Entities.editEntity(id, { position: newPosition });
                }
    
                SelectionManager._update();
            }
        });
    }

    function addGrabberStretchTool(overlay, mode, direction) {
        var pickNormal = null;
        var lastPick = null;
        var projectionVector = null;
        var stretchPanel = null;
        addGrabberTool(overlay, {
            mode: mode,
            onBegin: function(event, pickRay, pickResult) {
                if (direction === TRANSLATE_DIRECTION.X) {
                    stretchPanel = grabberStretchXPanel;
                    pickNormal = { x:0, y:0, z:1 };
                } else if (direction === TRANSLATE_DIRECTION.Y) {
                    stretchPanel = grabberStretchYPanel;
                    pickNormal = { x:1, y:0, z:0 };
                } else if (direction === TRANSLATE_DIRECTION.Z) {
                    stretchPanel = grabberStretchZPanel;
                    pickNormal = { x:0, y:1, z:0 };
                }

                Overlays.editOverlay(stretchPanel, { visible:true });

                var rotation = Entities.getEntityProperties(SelectionManager.selections[0]).rotation;
                pickNormal = Vec3.multiplyQbyV(rotation, pickNormal);

                lastPick = rayPlaneIntersection(pickRay, SelectionManager.worldPosition, pickNormal);
    
                SelectionManager.saveProperties();

                that.setGrabberTranslateVisible(false);
                that.setGrabberRotateVisible(false);
                that.setGrabberStretchXVisible(direction === TRANSLATE_DIRECTION.X);
                that.setGrabberStretchYVisible(direction === TRANSLATE_DIRECTION.Y);
                that.setGrabberStretchZVisible(direction === TRANSLATE_DIRECTION.Z);
    
                // Duplicate entities if alt is pressed.  This will make a
                // copy of the selected entities and move the _original_ entities, not
                // the new ones.
                if (event.isAlt) {
                    duplicatedEntityIDs = [];
                    for (var otherEntityID in SelectionManager.savedProperties) {
                        var properties = SelectionManager.savedProperties[otherEntityID];
                        if (!properties.locked) {
                            var entityID = Entities.addEntity(properties);
                            duplicatedEntityIDs.push({
                                entityID: entityID,
                                properties: properties
                            });
                        }
                    }
                } else {
                    duplicatedEntityIDs = null;
                }
            },
            onEnd: function(event, reason) {
                Overlays.editOverlay(stretchPanel, { visible:false });
                pushCommandForSelections(duplicatedEntityIDs);
            },
            onMove: function(event) {
                pickRay = generalComputePickRay(event.x, event.y);
    
                // translate mode left/right based on view toward entity
                var newIntersection = rayPlaneIntersection(pickRay, SelectionManager.worldPosition, pickNormal);
                var vector = Vec3.subtract(newIntersection, lastPick);

                if (direction === TRANSLATE_DIRECTION.X) {
                    projectionVector = { x:1, y:0, z:0 };
                } else if (direction === TRANSLATE_DIRECTION.Y) {
                    projectionVector = { x:0, y:1, z:0 };
                } else if (direction === TRANSLATE_DIRECTION.Z) {
                    projectionVector = { x:0, y:0, z:1 };
                }

                var rotation = Entities.getEntityProperties(SelectionManager.selections[0]).rotation;
                projectionVector = Vec3.multiplyQbyV(rotation, projectionVector);

                var dotVector = Vec3.dot(vector, projectionVector);
                vector = Vec3.multiply(dotVector, projectionVector);
                vector = grid.snapToGrid(vector);
    
                var wantDebug = false;
                if (wantDebug) {
                    print("translateUpDown... ");
                    print("                event.y:" + event.y);
                    Vec3.print("        newIntersection:", newIntersection);
                    Vec3.print("                 vector:", vector);
                    // Vec3.print("            newPosition:", newPosition);
                }
    
                for (var i = 0; i < SelectionManager.selections.length; i++) {
                    var id = SelectionManager.selections[i];
                    var properties = SelectionManager.savedProperties[id];
                    var newPosition = Vec3.sum(properties.position, vector);
                    var difference = Vec3.subtract(newPosition, SelectionManager.worldPosition);
                    var halfDifference = Vec3.multiply(difference, 0.5);
                    var quarterDifference = Vec3.multiply(halfDifference, 0.5);
                    var newDimensions = properties.dimensions;
                    var actualNewPosition = properties.position;

                    if (direction == TRANSLATE_DIRECTION.X) {
                        newDimensions.x += halfDifference.x;
                        actualNewPosition.x += quarterDifference.x;
                    } else if (direction == TRANSLATE_DIRECTION.Y) {
                        newDimensions.y += halfDifference.y;
                        actualNewPosition.y += quarterDifference.y;
                    } else if (direction == TRANSLATE_DIRECTION.Z) {
                        newDimensions.z += halfDifference.z;
                        actualNewPosition.z += quarterDifference.z;
                    }

                    Entities.editEntity(id, {
                        position: actualNewPosition,
                        dimensions: newDimensions
                    });
                }
    
                SelectionManager._update();
            }
        });
    }

    function addGrabberScaleTool(overlay, mode, direction) {
        var pickNormal = null;
        var lastPick = null;
        var selectedGrabber = null;
        addGrabberTool(overlay, {
            mode: mode,
            onBegin: function(event, pickRay, pickResult) {
                pickNormal = { x:-1, y:1, z:-1 };

                if (direction === SCALE_DIRECTION.LBN) {
                    selectedGrabber = grabberScaleLBNCube;
                } else if (direction === SCALE_DIRECTION.RBN) {
                    selectedGrabber = grabberScaleRBNCube;
                } else if (direction === SCALE_DIRECTION.LBF) {
                    selectedGrabber = grabberScaleLBFCube;
                } else if (direction === SCALE_DIRECTION.RBF) {
                    selectedGrabber = grabberScaleRBFCube;
                } else if (direction === SCALE_DIRECTION.LTN) {
                    selectedGrabber = grabberScaleLTNCube;
                } else if (direction === SCALE_DIRECTION.RTN) {
                    selectedGrabber = grabberScaleRTNCube;
                } else if (direction === SCALE_DIRECTION.LTF) {
                    selectedGrabber = grabberScaleLTFCube;
                } else if (direction === SCALE_DIRECTION.RTF) {
                    selectedGrabber = grabberScaleRTFCube;
                }
                Overlays.editOverlay(selectedGrabber, { color: GRABBER_SCALE_CUBE_SELECTED_COLOR });

                lastPick = rayPlaneIntersection(pickRay, SelectionManager.worldPosition, pickNormal);
    
                SelectionManager.saveProperties();

                that.setGrabberTranslateVisible(false);
                that.setGrabberRotateVisible(false);
                that.setGrabberStretchVisible(false);
                that.setGrabberScaleVisible(true);
    
                // Duplicate entities if alt is pressed.  This will make a
                // copy of the selected entities and move the _original_ entities, not
                // the new ones.
                if (event.isAlt) {
                    duplicatedEntityIDs = [];
                    for (var otherEntityID in SelectionManager.savedProperties) {
                        var properties = SelectionManager.savedProperties[otherEntityID];
                        if (!properties.locked) {
                            var entityID = Entities.addEntity(properties);
                            duplicatedEntityIDs.push({
                                entityID: entityID,
                                properties: properties
                            });
                        }
                    }
                } else {
                    duplicatedEntityIDs = null;
                }
            },
            onEnd: function(event, reason) {
                Overlays.editOverlay(selectedGrabber, { color: GRABBER_SCALE_CUBE_IDLE_COLOR });
                pushCommandForSelections(duplicatedEntityIDs);
            },
            onMove: function(event) {
                pickRay = generalComputePickRay(event.x, event.y);
    
                // translate mode left/right based on view toward entity
                var newIntersection = rayPlaneIntersection(pickRay, SelectionManager.worldPosition, pickNormal);
                var vector = Vec3.subtract(newIntersection, lastPick);

                var projectionVector;
                if (direction === SCALE_DIRECTION.LBN) {
                    projectionVector = { x:-1, y:-1, z:-1 };
                } else if (direction === SCALE_DIRECTION.RBN) {
                    projectionVector = { x:-1, y:-1, z:1 };
                } else if (direction === SCALE_DIRECTION.LBF) {
                    projectionVector = { x:1, y:-1, z:-1 };
                } else if (direction === SCALE_DIRECTION.RBF) {
                    projectionVector = { x:1, y:-1, z:1 };
                } else if (direction === SCALE_DIRECTION.LTN) {
                    projectionVector = { x:-1, y:1, z:-1 };
                } else if (direction === SCALE_DIRECTION.RTN) {
                    projectionVector = { x:-1, y:1, z:1 };
                } else if (direction === SCALE_DIRECTION.LTF) {
                    projectionVector = { x:1, y:1, z:-1 };
                } else if (direction === SCALE_DIRECTION.RTF) {
                    projectionVector = { x:1, y:1, z:1 };
                }

                var dotVector = Vec3.dot(vector, projectionVector);
                vector = Vec3.multiply(dotVector, projectionVector);
                vector = grid.snapToGrid(vector);

                var wantDebug = false;
                if (wantDebug) {
                    print("translateUpDown... ");
                    print("                event.y:" + event.y);
                    Vec3.print("        newIntersection:", newIntersection);
                    Vec3.print("                 vector:", vector);
                    // Vec3.print("            newPosition:", newPosition);
                }
    
                for (var i = 0; i < SelectionManager.selections.length; i++) {
                    var id = SelectionManager.selections[i];
                    var properties = SelectionManager.savedProperties[id];
                    var newPosition = Vec3.sum(properties.position, vector);
                    var difference = Vec3.subtract(newPosition, SelectionManager.worldPosition);
                    var differenceAvg = (difference.x + difference.y + difference.z) / 3;
                    var newDimensionsX = properties.dimensions.x + differenceAvg;
                    var newDimensionsY = properties.dimensions.y + differenceAvg;
                    var newDimensionsZ = properties.dimensions.z + differenceAvg;
                    if (newDimensionsX < SCALE_MINIMUM_DIMENSION) {
                        var differenceBelow = SCALE_MINIMUM_DIMENSION - newDimensionsX;
                        newDimensionsX += differenceBelow;
                        newDimensionsY += differenceBelow;
                        newDimensionsZ += differenceBelow;
                    }
                    if (newDimensionsY < SCALE_MINIMUM_DIMENSION) {
                        var differenceBelow = SCALE_MINIMUM_DIMENSION - newDimensionsY;
                        newDimensionsX += differenceBelow;
                        newDimensionsY += differenceBelow;
                        newDimensionsZ += differenceBelow;
                    }
                    if (newDimensionsZ < SCALE_MINIMUM_DIMENSION) {
                        var differenceBelow = SCALE_MINIMUM_DIMENSION - newDimensionsZ;
                        newDimensionsX += differenceBelow;
                        newDimensionsY += differenceBelow;
                        newDimensionsZ += differenceBelow;
                    }
                    Entities.editEntity(id, { dimensions: { x:newDimensionsX, y:newDimensionsY, z:newDimensionsZ }});
                }
    
                SelectionManager._update();
            }
        });
    }

    // FUNCTION: UPDATE ROTATION DEGREES OVERLAY
    function updateRotationDegreesOverlay(angleFromZero, direction, centerPosition) {
        var angle = angleFromZero * (Math.PI / 180);
        var position = {
            x: Math.cos(angle) * ROTATION_DISPLAY_DISTANCE_MULTIPLIER,
            y: Math.sin(angle) * ROTATION_DISPLAY_DISTANCE_MULTIPLIER,
            z: 0
        };
        if (direction === ROTATE_DIRECTION.PITCH)
            position = Vec3.multiplyQbyV(Quat.fromPitchYawRollDegrees(0, -90, 0), position);
        else if (direction === ROTATE_DIRECTION.YAW)
            position = Vec3.multiplyQbyV(Quat.fromPitchYawRollDegrees(90, 0, 0), position);
        else if (direction === ROTATE_DIRECTION.ROLL)
            position = Vec3.multiplyQbyV(Quat.fromPitchYawRollDegrees(0, 180, 0), position);
        position = Vec3.sum(centerPosition, position);
        var overlayProps = {
            position: position,
            dimensions: {
                x: ROTATION_DISPLAY_SIZE_X_MULTIPLIER,
                y: ROTATION_DISPLAY_SIZE_Y_MULTIPLIER
            },
            lineHeight: ROTATION_DISPLAY_LINE_HEIGHT_MULTIPLIER,
            text: normalizeDegrees(-angleFromZero) + "°"
        };
        Overlays.editOverlay(rotationDegreesDisplay, overlayProps);
    }

    // FUNCTION DEF: updateSelectionsRotation
    //    Helper func used by rotation grabber tools 
    function updateSelectionsRotation(rotationChange) {
        if (!rotationChange) {
            print("ERROR: entitySelectionTool.updateSelectionsRotation - Invalid arg specified!!");

            // EARLY EXIT
            return;
        }
        
        // Entities should only reposition if we are rotating multiple selections around
        // the selections center point.  Otherwise, the rotation will be around the entities
        // registration point which does not need repositioning.
        var reposition = (SelectionManager.selections.length > 1);
        for (var i = 0; i < SelectionManager.selections.length; i++) {
            var entityID = SelectionManager.selections[i];
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

    function addGrabberRotateTool(overlay, mode, direction) {
        var initialPosition = SelectionManager.worldPosition;
        var selectedGrabber = null;
        addGrabberTool(overlay, {
            mode: mode,
            onBegin: function(event, pickRay, pickResult) {
                SelectionManager.saveProperties();
    
                that.setGrabberTranslateVisible(false);
                that.setGrabberRotatePitchVisible(direction === ROTATE_DIRECTION.PITCH);
                that.setGrabberRotateYawVisible(direction === ROTATE_DIRECTION.YAW);
                that.setGrabberRotateRollVisible(direction === ROTATE_DIRECTION.ROLL);
                that.setGrabberStretchVisible(false);
                that.setGrabberScaleVisible(false);

                initialPosition = SelectionManager.worldPosition;

                if (direction === ROTATE_DIRECTION.PITCH) {
                    rotationNormal = { x: 1, y: 0, z: 0 };
                    selectedGrabber = grabberRotatePitchRing;
                } else if (direction === ROTATE_DIRECTION.YAW) {
                    rotationNormal = { x: 0, y: 1, z: 0 };
                    selectedGrabber = grabberRotateYawRing;
                } else if (direction === ROTATE_DIRECTION.ROLL) {
                    rotationNormal = { x: 0, y: 0, z: 1 };
                    selectedGrabber = grabberRotateRollRing;
                }

                Overlays.editOverlay(selectedGrabber, { hasTickMarks: true });

                var rotation = Entities.getEntityProperties(SelectionManager.selections[0]).rotation;
                rotationNormal = Vec3.multiplyQbyV(rotation, rotationNormal);

                var rotCenter = SelectionManager.worldPosition;

                Overlays.editOverlay(rotationDegreesDisplay, { visible: true });
                Overlays.editOverlay(grabberRotateCurrentRing, { visible: true });
                updateRotationDegreesOverlay(0, direction, rotCenter);

                // editOverlays may not have committed rotation changes.
                // Compute zero position based on where the overlay will be eventually.
                var result = rayPlaneIntersection(pickRay, rotCenter, rotationNormal);
                // In case of a parallel ray, this will be null, which will cause early-out
                // in the onMove helper.
                rotZero = result;
            },
            onEnd: function(event, reason) {
                Overlays.editOverlay(rotationDegreesDisplay, { visible: false });
                Overlays.editOverlay(selectedGrabber, { hasTickMarks: false });
                Overlays.editOverlay(grabberRotateCurrentRing, { visible: false });
                pushCommandForSelections();
            },
            onMove: function(event) {
                if (!rotZero) {
                    print("ERROR: entitySelectionTool.handleRotationHandleOnMove - Invalid RotationZero Specified (missed rotation target plane?)");
        
                    // EARLY EXIT
                    return;
                }
                var pickRay = generalComputePickRay(event.x, event.y);
                var rotCenter = SelectionManager.worldPosition;
                var result = rayPlaneIntersection(pickRay, rotCenter, rotationNormal);
                if (result) {
                    var centerToZero = Vec3.subtract(rotZero, rotCenter);
                    var centerToIntersect = Vec3.subtract(result, rotCenter);
                    // Note: orientedAngle which wants normalized centerToZero and centerToIntersect
                    //             handles that internally, so it's to pass unnormalized vectors here.
                    var angleFromZero = Math.floor((Vec3.orientedAngle(centerToZero, centerToIntersect, rotationNormal)));        
                    var rotChange = Quat.angleAxis(angleFromZero, rotationNormal);
                    updateSelectionsRotation(rotChange);
                    updateRotationDegreesOverlay(-angleFromZero, direction, rotCenter);

                    var worldRotation;
                    if (direction === ROTATE_DIRECTION.PITCH) {
                        worldRotation = worldRotationY;
                    } else if (direction === ROTATE_DIRECTION.YAW) {
                        worldRotation = worldRotationZ;
                    } else if (direction === ROTATE_DIRECTION.ROLL) {
                        worldRotation = worldRotationX;
                    }

                    var startAtCurrent = 0;
                    var endAtCurrent = angleFromZero;
                    if (angleFromZero < 0) {
                        startAtCurrent = 360 + angleFromZero;
                        endAtCurrent = 360;
                    }
                    Overlays.editOverlay(grabberRotateCurrentRing, {
                        position: rotCenter,
                        rotation: worldRotation,
                        startAt: startAtCurrent,
                        endAt: endAtCurrent
                    });
                }
            }
        });
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

        if (!grabberTools.hasOwnProperty(toolHandle)) {
            print("WARNING: entitySelectionTool.isActiveTool - Encountered unknown grabberToolHandle: " + toolHandle + ". Tools should be registered via addGrabberTool.");
            // EARLY EXIT
            return false;
        }

        return (activeTool === grabberTools[ toolHandle ]);
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

    that.highlightSelectable = function(entityID) {
        var properties = Entities.getEntityProperties(entityID);
    };

    that.unhighlightSelectable = function(entityID) {
    };

    that.select = function(entityID, event) {
        var properties = Entities.getEntityProperties(SelectionManager.selections[0]);

        lastCameraPosition = Camera.getPosition();
        lastCameraOrientation = Camera.getOrientation();

        if (event !== false) {
            pickRay = generalComputePickRay(event.x, event.y);

            var wantDebug = false;
            if (wantDebug) {
                print("select() with EVENT...... ");
                print("                event.y:" + event.y);
                Vec3.print("       current position:", properties.position);
            }
        }

        /*
        Overlays.editOverlay(highlightBox, {
            visible: false
        });
        */

        that.updateHandles();
    };

    // FUNCTION: UPDATE HANDLE POSITION ROTATION
    that.updateHandlePositionRotation = function() {
        if (SelectionManager.hasSelection()) {
            var worldPosition = SelectionManager.worldPosition;
            var worldRotation = Entities.getEntityProperties(SelectionManager.selections[0]).rotation;

            var localRotationX = Quat.fromPitchYawRollDegrees(0, 0, -90);
            worldRotationX = Quat.multiply(worldRotation, localRotationX);
            var localRotationY = Quat.fromPitchYawRollDegrees(0, 90, 0);
            worldRotationY = Quat.multiply(worldRotation, localRotationY);
            var localRotationZ = Quat.fromPitchYawRollDegrees(90, 0, 0);
            worldRotationZ = Quat.multiply(worldRotation, localRotationZ);

            var cylinderXPos = Vec3.sum(worldPosition, Vec3.multiplyQbyV(worldRotation, { x:GRABBER_TRANSLATE_ARROW_CYLINDER_OFFSET, y:0, z:0 }));
            Overlays.editOverlay(grabberTranslateXCylinder, { position: cylinderXPos, rotation:worldRotationX });
            var coneXPos = Vec3.sum(worldPosition, Vec3.multiplyQbyV(worldRotation, { x:GRABBER_TRANSLATE_ARROW_CONE_OFFSET, y:0, z:0 }));
            Overlays.editOverlay(grabberTranslateXCone, { position: coneXPos, rotation:worldRotationX });
            var stretchXPos = Vec3.sum(worldPosition, Vec3.multiplyQbyV(worldRotation, { x:GRABBER_STRETCH_SPHERE_OFFSET, y:0, z:0 }));
            Overlays.editOverlay(grabberStretchXSphere, { position: stretchXPos });
            Overlays.editOverlay(grabberStretchXPanel, { position: stretchXPos, rotation:worldRotationY });

            var cylinderYPos = Vec3.sum(worldPosition, Vec3.multiplyQbyV(worldRotation, { x:0, y:GRABBER_TRANSLATE_ARROW_CYLINDER_OFFSET, z:0 }));
            Overlays.editOverlay(grabberTranslateYCylinder, { position: cylinderYPos, rotation:worldRotationY });
            var coneYPos = Vec3.sum(worldPosition, Vec3.multiplyQbyV(worldRotation, { x:0, y:GRABBER_TRANSLATE_ARROW_CONE_OFFSET, z:0 }));
            Overlays.editOverlay(grabberTranslateYCone, { position: coneYPos, rotation:worldRotationY });
            var stretchYPos = Vec3.sum(worldPosition, Vec3.multiplyQbyV(worldRotation, { x:0, y:GRABBER_STRETCH_SPHERE_OFFSET, z:0 }));
            Overlays.editOverlay(grabberStretchYSphere, { position: stretchYPos });
            Overlays.editOverlay(grabberStretchYPanel, { position: stretchYPos, rotation:worldRotationZ });

            var cylinderZPos = Vec3.sum(worldPosition, Vec3.multiplyQbyV(worldRotation, { x:0, y:0, z:GRABBER_TRANSLATE_ARROW_CYLINDER_OFFSET }));
            Overlays.editOverlay(grabberTranslateZCylinder, { position: cylinderZPos, rotation:worldRotationZ });
            var coneZPos = Vec3.sum(worldPosition, Vec3.multiplyQbyV(worldRotation, { x:0, y:0, z:GRABBER_TRANSLATE_ARROW_CONE_OFFSET }));
            Overlays.editOverlay(grabberTranslateZCone, { position: coneZPos, rotation:worldRotationZ });
            var stretchZPos = Vec3.sum(worldPosition, Vec3.multiplyQbyV(worldRotation, { x:0, y:0, z:GRABBER_STRETCH_SPHERE_OFFSET }));
            Overlays.editOverlay(grabberStretchZSphere, { position: stretchZPos });
            Overlays.editOverlay(grabberStretchZPanel, { position: stretchZPos, rotation:worldRotationX });

            if (!isActiveTool(grabberRotatePitchRing)) {
                Overlays.editOverlay(grabberRotatePitchRing, { position: SelectionManager.worldPosition, rotation: worldRotationY });
            }
            if (!isActiveTool(grabberRotateYawRing)) {
                Overlays.editOverlay(grabberRotateYawRing, { position: SelectionManager.worldPosition, rotation: worldRotationZ });
            }
            if (!isActiveTool(grabberRotateRollRing)) {
                Overlays.editOverlay(grabberRotateRollRing, { position: SelectionManager.worldPosition, rotation: worldRotationX });
            }

            var grabberScaleLBNCubePos = Vec3.sum(worldPosition, { x:-GRABBER_SCALE_CUBE_OFFSET, y:-GRABBER_SCALE_CUBE_OFFSET, z:-GRABBER_SCALE_CUBE_OFFSET });
            Overlays.editOverlay(grabberScaleLBNCube, { position:grabberScaleLBNCubePos });
            var grabberScaleRBNCubePos = Vec3.sum(worldPosition, { x:-GRABBER_SCALE_CUBE_OFFSET, y:-GRABBER_SCALE_CUBE_OFFSET, z:GRABBER_SCALE_CUBE_OFFSET });
            Overlays.editOverlay(grabberScaleRBNCube, { position:grabberScaleRBNCubePos });
            var grabberScaleLBFCubePos = Vec3.sum(worldPosition, { x:GRABBER_SCALE_CUBE_OFFSET, y:-GRABBER_SCALE_CUBE_OFFSET, z:-GRABBER_SCALE_CUBE_OFFSET });
            Overlays.editOverlay(grabberScaleLBFCube, { position:grabberScaleLBFCubePos });
            var grabberScaleRBFCubePos = Vec3.sum(worldPosition, { x:GRABBER_SCALE_CUBE_OFFSET, y:-GRABBER_SCALE_CUBE_OFFSET, z:GRABBER_SCALE_CUBE_OFFSET });
            Overlays.editOverlay(grabberScaleRBFCube, { position:grabberScaleRBFCubePos });
            var grabberScaleLTNCubePos = Vec3.sum(worldPosition, { x:-GRABBER_SCALE_CUBE_OFFSET, y:GRABBER_SCALE_CUBE_OFFSET, z:-GRABBER_SCALE_CUBE_OFFSET });
            Overlays.editOverlay(grabberScaleLTNCube, { position:grabberScaleLTNCubePos });
            var grabberScaleRTNCubePos = Vec3.sum(worldPosition, { x:-GRABBER_SCALE_CUBE_OFFSET, y:GRABBER_SCALE_CUBE_OFFSET, z:GRABBER_SCALE_CUBE_OFFSET });
            Overlays.editOverlay(grabberScaleRTNCube, { position:grabberScaleRTNCubePos });
            var grabberScaleLTFCubePos = Vec3.sum(worldPosition, { x:GRABBER_SCALE_CUBE_OFFSET, y:GRABBER_SCALE_CUBE_OFFSET, z:-GRABBER_SCALE_CUBE_OFFSET });
            Overlays.editOverlay(grabberScaleLTFCube, { position:grabberScaleLTFCubePos });
            var grabberScaleRTFCubePos = Vec3.sum(worldPosition, { x:GRABBER_SCALE_CUBE_OFFSET, y:GRABBER_SCALE_CUBE_OFFSET, z:GRABBER_SCALE_CUBE_OFFSET });
            Overlays.editOverlay(grabberScaleRTFCube, { position:grabberScaleRTFCubePos });

            Overlays.editOverlay(grabberScaleTREdge, { start: grabberScaleRTNCubePos, end:grabberScaleRTFCubePos });
            Overlays.editOverlay(grabberScaleTLEdge, { start: grabberScaleLTNCubePos, end:grabberScaleLTFCubePos });
            Overlays.editOverlay(grabberScaleTFEdge, { start: grabberScaleLTFCubePos, end:grabberScaleRTFCubePos });
            Overlays.editOverlay(grabberScaleTNEdge, { start: grabberScaleLTNCubePos, end:grabberScaleRTNCubePos });
            Overlays.editOverlay(grabberScaleBREdge, { start: grabberScaleRBNCubePos, end:grabberScaleRBFCubePos });
            Overlays.editOverlay(grabberScaleBLEdge, { start: grabberScaleLBNCubePos, end:grabberScaleLBFCubePos });
            Overlays.editOverlay(grabberScaleBFEdge, { start: grabberScaleLBFCubePos, end:grabberScaleRBFCubePos });
            Overlays.editOverlay(grabberScaleBNEdge, { start: grabberScaleLBNCubePos, end:grabberScaleRBNCubePos });
            Overlays.editOverlay(grabberScaleNREdge, { start: grabberScaleRTNCubePos, end:grabberScaleRBNCubePos });
            Overlays.editOverlay(grabberScaleNLEdge, { start: grabberScaleLTNCubePos, end:grabberScaleLBNCubePos });
            Overlays.editOverlay(grabberScaleFREdge, { start: grabberScaleRTFCubePos, end:grabberScaleRBFCubePos });
            Overlays.editOverlay(grabberScaleFLEdge, { start: grabberScaleLTFCubePos, end:grabberScaleLBFCubePos });
        }
    };
    Script.update.connect(that.updateHandlePositionRotation);

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
            print("WARNING: entitySelectionTool.setSpaceMode - Can't update SpaceMode. CurrentMode: " + spaceMode + " DesiredMode: " + newSpaceMode);
        }
        if (wantDebug) {
            print("====== SetSpaceMode called. <========");
        }
    };

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

        var isSingleSelection = (SelectionManager.selections.length === 1);
        if (isSingleSelection) {
        }// end of isSingleSelection

        that.setGrabberTranslateXVisible(!activeTool || isActiveTool(grabberTranslateXCone) || isActiveTool(grabberTranslateXCylinder));
        that.setGrabberTranslateYVisible(!activeTool || isActiveTool(grabberTranslateYCone) || isActiveTool(grabberTranslateYCylinder));
        that.setGrabberTranslateZVisible(!activeTool || isActiveTool(grabberTranslateZCone) || isActiveTool(grabberTranslateZCylinder));
        that.setGrabberRotatePitchVisible(!activeTool || isActiveTool(grabberRotatePitchRing));
        that.setGrabberRotateYawVisible(!activeTool || isActiveTool(grabberRotateYawRing));
        that.setGrabberRotateRollVisible(!activeTool || isActiveTool(grabberRotateRollRing));
        that.setGrabberStretchXVisible(!activeTool || isActiveTool(grabberStretchXSphere));
        that.setGrabberStretchYVisible(!activeTool || isActiveTool(grabberStretchYSphere));
        that.setGrabberStretchZVisible(!activeTool || isActiveTool(grabberStretchZSphere));
        that.setGrabberScaleVisible(!activeTool || isActiveTool(grabberScaleLBNCube) || isActiveTool(grabberScaleRBNCube) || isActiveTool(grabberScaleLBFCube) || isActiveTool(grabberScaleRBFCube)
                                                || isActiveTool(grabberScaleLTNCube) || isActiveTool(grabberScaleRTNCube) || isActiveTool(grabberScaleLTFCube) || isActiveTool(grabberScaleRTFCube));

        if (wantDebug) {
            print("====== Update Handles <=======");
        }
    };

    // FUNCTION: SET OVERLAYS VISIBLE
    that.setOverlaysVisible = function(isVisible) {
        for (var i = 0; i < allOverlays.length; i++) {
            Overlays.editOverlay(allOverlays[i], { visible: isVisible });
        }
    };

    // FUNCTION: SET GRABBER TRANSLATE VISIBLE
    that.setGrabberTranslateVisible = function(isVisible) {
        that.setGrabberTranslateXVisible(isVisible);
        that.setGrabberTranslateYVisible(isVisible);
        that.setGrabberTranslateZVisible(isVisible);
    };

    that.setGrabberTranslateXVisible = function(isVisible) {
        Overlays.editOverlay(grabberTranslateXCone, { visible: isVisible });
        Overlays.editOverlay(grabberTranslateXCylinder, { visible: isVisible });
    };

    that.setGrabberTranslateYVisible = function(isVisible) {
        Overlays.editOverlay(grabberTranslateYCone, { visible: isVisible });
        Overlays.editOverlay(grabberTranslateYCylinder, { visible: isVisible });
    };

    that.setGrabberTranslateZVisible = function(isVisible) {
        Overlays.editOverlay(grabberTranslateZCone, { visible: isVisible });
        Overlays.editOverlay(grabberTranslateZCylinder, { visible: isVisible });
    };

    // FUNCTION: SET GRABBER ROTATION VISIBLE
    that.setGrabberRotateVisible = function(isVisible) {
        that.setGrabberRotatePitchVisible(isVisible);
        that.setGrabberRotateYawVisible(isVisible);
        that.setGrabberRotateRollVisible(isVisible);
    };

    that.setGrabberRotatePitchVisible = function(isVisible) {
        Overlays.editOverlay(grabberRotatePitchRing, { visible: isVisible });
    };

    that.setGrabberRotateYawVisible = function(isVisible) {
        Overlays.editOverlay(grabberRotateYawRing, { visible: isVisible });
    };

    that.setGrabberRotateRollVisible = function(isVisible) {
        Overlays.editOverlay(grabberRotateRollRing, { visible: isVisible });
    };

    // FUNCTION: SET GRABBER STRETCH VISIBLE
    that.setGrabberStretchVisible = function(isVisible) {
        that.setGrabberStretchXVisible(isVisible);
        that.setGrabberStretchYVisible(isVisible);
        that.setGrabberStretchZVisible(isVisible);
    };

    that.setGrabberStretchXVisible = function(isVisible) {
        Overlays.editOverlay(grabberStretchXSphere, { visible: isVisible });
    };

    that.setGrabberStretchYVisible = function(isVisible) {
        Overlays.editOverlay(grabberStretchYSphere, { visible: isVisible });
    };

    that.setGrabberStretchZVisible = function(isVisible) {
        Overlays.editOverlay(grabberStretchZSphere, { visible: isVisible });
    };
    
    // FUNCTION: SET GRABBER SCALE VISIBLE
    that.setGrabberScaleVisible = function(isVisible) {
        Overlays.editOverlay(grabberScaleLBNCube, { visible: isVisible });
        Overlays.editOverlay(grabberScaleRBNCube, { visible: isVisible });
        Overlays.editOverlay(grabberScaleLBFCube, { visible: isVisible });
        Overlays.editOverlay(grabberScaleRBFCube, { visible: isVisible });
        Overlays.editOverlay(grabberScaleLTNCube, { visible: isVisible });
        Overlays.editOverlay(grabberScaleRTNCube, { visible: isVisible });
        Overlays.editOverlay(grabberScaleLTFCube, { visible: isVisible });
        Overlays.editOverlay(grabberScaleRTFCube, { visible: isVisible });
        Overlays.editOverlay(grabberScaleTREdge, { visible: isVisible });
        Overlays.editOverlay(grabberScaleTLEdge, { visible: isVisible });
        Overlays.editOverlay(grabberScaleTFEdge, { visible: isVisible });
        Overlays.editOverlay(grabberScaleTNEdge, { visible: isVisible });
        Overlays.editOverlay(grabberScaleBREdge, { visible: isVisible });
        Overlays.editOverlay(grabberScaleBLEdge, { visible: isVisible });
        Overlays.editOverlay(grabberScaleBFEdge, { visible: isVisible });
        Overlays.editOverlay(grabberScaleBNEdge, { visible: isVisible });
        Overlays.editOverlay(grabberScaleNREdge, { visible: isVisible });
        Overlays.editOverlay(grabberScaleNLEdge, { visible: isVisible });
        Overlays.editOverlay(grabberScaleFREdge, { visible: isVisible });
        Overlays.editOverlay(grabberScaleFLEdge, { visible: isVisible });
    };

    addGrabberTranslateTool(grabberTranslateXCone, "TRANSLATE_X", TRANSLATE_DIRECTION.X);
    addGrabberTranslateTool(grabberTranslateXCylinder, "TRANSLATE_X", TRANSLATE_DIRECTION.X);
    addGrabberTranslateTool(grabberTranslateYCone, "TRANSLATE_Y", TRANSLATE_DIRECTION.Y);
    addGrabberTranslateTool(grabberTranslateYCylinder, "TRANSLATE_Y", TRANSLATE_DIRECTION.Y);
    addGrabberTranslateTool(grabberTranslateZCone, "TRANSLATE_Z", TRANSLATE_DIRECTION.Z);
    addGrabberTranslateTool(grabberTranslateZCylinder, "TRANSLATE_Z", TRANSLATE_DIRECTION.Z);

    addGrabberRotateTool(grabberRotatePitchRing, "ROTATE_PITCH", ROTATE_DIRECTION.PITCH);
    addGrabberRotateTool(grabberRotateYawRing, "ROTATE_YAW", ROTATE_DIRECTION.YAW);
    addGrabberRotateTool(grabberRotateRollRing, "ROTATE_ROLL", ROTATE_DIRECTION.ROLL);

    addGrabberStretchTool(grabberStretchXSphere, "STRETCH_X", TRANSLATE_DIRECTION.X);
    addGrabberStretchTool(grabberStretchYSphere, "STRETCH_Y", TRANSLATE_DIRECTION.Y);
    addGrabberStretchTool(grabberStretchZSphere, "STRETCH_Z", TRANSLATE_DIRECTION.Z);

    addGrabberScaleTool(grabberScaleLBNCube, "SCALE_LBN", SCALE_DIRECTION.LBN);
    addGrabberScaleTool(grabberScaleRBNCube, "SCALE_RBN", SCALE_DIRECTION.RBN);
    addGrabberScaleTool(grabberScaleLBFCube, "SCALE_LBF", SCALE_DIRECTION.LBF);
    addGrabberScaleTool(grabberScaleRBFCube, "SCALE_RBF", SCALE_DIRECTION.RBF);
    addGrabberScaleTool(grabberScaleLTNCube, "SCALE_LTN", SCALE_DIRECTION.LTN);
    addGrabberScaleTool(grabberScaleRTNCube, "SCALE_RTN", SCALE_DIRECTION.RTN);
    addGrabberScaleTool(grabberScaleLTFCube, "SCALE_LTF", SCALE_DIRECTION.LTF);
    addGrabberScaleTool(grabberScaleRTFCube, "SCALE_RTF", SCALE_DIRECTION.RTF);

    // FUNCTION: CHECK MOVE
    that.checkMove = function() {
        if (SelectionManager.hasSelection()) {

            // FIXME - this cause problems with editing in the entity properties window
            // SelectionManager._update();

            if (!Vec3.equal(Camera.getPosition(), lastCameraPosition) ||
                !Quat.equal(Camera.getOrientation(), lastCameraOrientation)) {

                //that.updateRotationHandles();
            }
        }
    };

    that.checkControllerMove = function() {
        if (SelectionManager.hasSelection()) {
            var controllerPose = getControllerWorldLocation(activeHand, true);
            var hand = (activeHand === Controller.Standard.LeftHand) ? 0 : 1;
            if (controllerPose.valid && lastControllerPoses[hand].valid) {
                if (!Vec3.equal(controllerPose.position, lastControllerPoses[hand].position) ||
                    !Vec3.equal(controllerPose.rotation, lastControllerPoses[hand].rotation)) {
                    that.mouseMoveEvent({});
                }
            }
            lastControllerPoses[hand] = controllerPose;
        }
    };

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
        var interactiveOverlays = [HMD.tabletID, HMD.tabletScreenID, HMD.homeButtonID];
        for (var key in grabberTools) {
            if (grabberTools.hasOwnProperty(key)) {
                interactiveOverlays.push(key);
            }
        }

        // Start with unknown mode, in case no tool can handle this.
        activeTool = null;

        var results = testRayIntersect(pickRay, interactiveOverlays);
        if (results.intersects) {
            var hitOverlayID = results.overlayID;
            if ((hitOverlayID === HMD.tabletID) || (hitOverlayID === HMD.tabletScreenID) || (hitOverlayID === HMD.homeButtonID)) {
                // EARLY EXIT-(mouse clicks on the tablet should override the edit affordances)
                return false;
            }

            entityIconOverlayManager.setIconsSelectable(SelectionManager.selections, true);

            var hitTool = grabberTools[ hitOverlayID ];
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

    // NOTE: mousePressEvent and mouseMoveEvent from the main script should call us., so we don't hook these:
    //       Controller.mousePressEvent.connect(that.mousePressEvent);
    //       Controller.mouseMoveEvent.connect(that.mouseMoveEvent);
    Controller.mouseReleaseEvent.connect(that.mouseReleaseEvent);

    return that;
}());
