//
//  vr-edit.js
//
//  Created by David Rowe on 27 Jun 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {

    "use strict";

    var APP_NAME = "VR EDIT",  // TODO: App name.
        APP_ICON_INACTIVE = "icons/tablet-icons/edit-i.svg",  // TODO: App icons.
        APP_ICON_ACTIVE = "icons/tablet-icons/edit-a.svg",
        VR_EDIT_SETTING = "io.highfidelity.isVREditing",  // Note: This constant is duplicated in utils.js.

        // Application state
        isAppActive = false,
        dominantHand,

        // Tool state
        TOOL_NONE = 0,
        TOOL_SCALE = 1,
        TOOL_CLONE = 2,
        TOOL_GROUP = 3,
        TOOL_COLOR = 4,
        TOOL_PICK_COLOR = 5,
        TOOL_PHYSICS = 6,
        TOOL_DELETE = 7,
        toolSelected = TOOL_NONE,
        colorToolColor = { red: 128, green: 128, blue: 128 },
        physicsToolPhysics = { userData: { grabbableKey: {} } },
        EARTH_GRAVITY = -9.80665,
        physicsToolGravity = EARTH_GRAVITY,

        // Primary objects
        App,
        Inputs,
        inputs = [],
        UI,
        ui,
        Editor,
        editors = [],
        LEFT_HAND = 0,
        RIGHT_HAND = 1,
        Grouping,
        grouping,

        // Modules
        CreatePalette,
        Groups,
        Hand,
        Handles,
        Highlights,
        Laser,
        Selection,
        ToolIcon,
        ToolsMenu,

        // Miscellaneous
        UPDATE_LOOP_TIMEOUT = 16,
        updateTimer = null,
        tablet,
        button,

        DEBUG = true;  // TODO: Set false.

    // Utilities
    Script.include("./utilities/utilities.js");

    // Modules
    Script.include("./modules/createPalette.js");
    Script.include("./modules/groups.js");
    Script.include("./modules/hand.js");
    Script.include("./modules/handles.js");
    Script.include("./modules/highlights.js");
    Script.include("./modules/laser.js");
    Script.include("./modules/selection.js");
    Script.include("./modules/toolIcon.js");
    Script.include("./modules/toolsMenu.js");
    Script.include("./modules/uit.js");


    function log(side, message) {
        // Optional parameter: side.
        var hand = "",
            HAND_LETTERS = ["L", "R"];
        if (side === 0 || side === 1) {
            hand = HAND_LETTERS[side] + " ";
        } else {
            message = side;
        }
        print(APP_NAME + ": " + hand + message);
    }

    function debug(side, message) {
        // Optional parameter: side.
        if (DEBUG) {
            log(side, message);
        }
    }

    function otherHand(hand) {
        return (hand + 1) % 2;
    }

    App = {
        log: log,
        debug: debug,
        DEBUG: DEBUG
    };


    Inputs = function (side) {
        // A hand plus a laser.

        var
            // Primary objects.
            hand,
            laser,

            intersection = {};


        if (!this instanceof Inputs) {
            return new Inputs();
        }

        hand = new Hand(side);
        laser = new Laser(side);


        function setUIEntities(entityIDs) {
            laser.setUIEntities(entityIDs);
        }

        function getHand() {
            return hand;
        }

        function getLaser() {
            return laser;
        }

        function getIntersection() {
            return intersection;
        }

        function update() {
            // Hand update.
            hand.update();
            intersection = hand.intersection();

            // Laser update.
            // Displays laser if hand has no intersection and trigger is pressed.
            if (hand.valid()) {
                laser.update(hand);
                if (!intersection.intersects) {
                    intersection = laser.intersection();
                }
            }
        }

        function clear() {
            hand.clear();
            laser.clear();
        }

        function destroy() {
            if (hand) {
                hand.destroy();
                hand = null;
            }
            if (laser) {
                laser.destroy();
                laser = null;
            }
        }

        return {
            setUIEntities: setUIEntities,
            hand: getHand,
            laser: getLaser,
            intersection: getIntersection,
            update: update,
            clear: clear,
            destroy: destroy
        };
    };


    UI = function (side, leftInputs, rightInputs, uiCommandCallback) {
        // Tool menu and Create palette.

        var // Primary objects.
            toolsMenu,
            toolIcon,
            createPalette,

            isDisplaying = false,

            getIntersection;  // Function.


        if (!this instanceof UI) {
            return new UI();
        }

        toolIcon = new ToolIcon(otherHand(side));
        toolsMenu = new ToolsMenu(side, leftInputs, rightInputs, uiCommandCallback);
        createPalette = new CreatePalette(side, leftInputs, rightInputs, uiCommandCallback);

        getIntersection = side === LEFT_HAND ? rightInputs.intersection : leftInputs.intersection;


        function setHand(side) {
            toolIcon.setHand(otherHand(side));
            toolsMenu.setHand(side);
            createPalette.setHand(side);
            getIntersection = side === LEFT_HAND ? rightInputs.intersection : leftInputs.intersection;
        }

        function setToolIcon(icon) {
            toolIcon.display(toolsMenu.iconInfo(icon));
        }

        function clearTool() {
            toolIcon.clear();
            toolsMenu.clearTool();
        }

        function setUIEntities() {
            var uiEntityIDs = [].concat(toolsMenu.entityIDs(), createPalette.entityIDs());
            leftInputs.setUIEntities(side === RIGHT_HAND ? uiEntityIDs : []);
            rightInputs.setUIEntities(side === LEFT_HAND ? uiEntityIDs : []);
        }

        function display() {
            toolsMenu.display();
            createPalette.display();
            setUIEntities();
            isDisplaying = true;
        }

        function update() {
            var intersection;

            if (isDisplaying) {
                intersection = getIntersection();
                toolsMenu.update(intersection, grouping.groupsCount(), grouping.entitiesCount());
                createPalette.update(intersection.overlayID);
                toolIcon.update();
            }
        }

        function doPickColor(color) {
            toolsMenu.doCommand("setColorFromPick", color);
        }

        function clear() {
            leftInputs.setUIEntities([]);
            rightInputs.setUIEntities([]);
            toolIcon.clear();
            toolsMenu.clear();
            createPalette.clear();

            isDisplaying = false;
        }

        function destroy() {
            if (createPalette) {
                createPalette.destroy();
                createPalette = null;
            }
            if (toolsMenu) {
                toolsMenu.destroy();
                toolsMenu = null;
            }
            if (toolIcon) {
                toolIcon.destroy();
                toolIcon = null;
            }
        }

        return {
            setHand: setHand,
            setToolIcon: setToolIcon,
            clearTool: clearTool,
            COLOR_TOOL: toolsMenu.COLOR_TOOL,
            SCALE_TOOL: toolsMenu.SCALE_TOOL,
            CLONE_TOOL: toolsMenu.CLONE_TOOL,
            GROUP_TOOL: toolsMenu.GROUP_TOOL,
            PHYSICS_TOOL: toolsMenu.PHYSICS_TOOL,
            DELETE_TOOL: toolsMenu.DELETE_TOOL,
            display: display,
            updateUIEntities: setUIEntities,
            doPickColor: doPickColor,
            update: update,
            clear: clear,
            destroy: destroy
        };
    };


    Editor = function (side) {
        // An entity selection, entity highlights, and entity handles.

        var
            // Primary objects.
            selection,
            highlights,
            handles,

            // References.
            otherEditor,  // Other hand's Editor object.
            hand,
            laser,

            // Editor states.
            EDITOR_IDLE = 0,
            EDITOR_SEARCHING = 1,
            EDITOR_HIGHLIGHTING = 2,  // Highlighting an entity (not hovering a handle).
            EDITOR_GRABBING = 3,
            EDITOR_DIRECT_SCALING = 4,  // Scaling data are sent to other editor's EDITOR_GRABBING state.
            EDITOR_HANDLE_SCALING = 5,  // ""
            EDITOR_CLONING = 6,
            EDITOR_GROUPING = 7,
            EDITOR_STATE_STRINGS = ["EDITOR_IDLE", "EDITOR_SEARCHING", "EDITOR_HIGHLIGHTING", "EDITOR_GRABBING",
                "EDITOR_DIRECT_SCALING", "EDITOR_HANDLE_SCALING", "EDITOR_CLONING", "EDITOR_GROUPING"],
            editorState = EDITOR_IDLE,

            // State machine.
            STATE_MACHINE,
            intersectedEntityID = null,  // Intersected entity of highlighted entity set.
            rootEntityID = null,  // Root entity of highlighted entity set.
            wasScaleTool = false,
            isOtherEditorEditingEntityID = false,
            isTriggerClicked = false,
            wasTriggerClicked = false,
            isGripClicked = false,
            wasGripClicked = false,
            hoveredOverlayID = null,
            isAutoGrab = false,

            // Position values.
            initialHandOrientationInverse,
            initialHandToSelectionVector,
            initialSelectionOrientation,

            // Scaling values.
            isScalingWithHand = false,
            isDirectScaling = false,  // Modifies EDITOR_GRABBING state.
            isHandleScaling = false,  // ""
            initialTargetsSeparation,
            initialtargetsDirection,
            initialTargetToBoundingBoxCenter,
            otherTargetPosition,
            handleUnitScaleAxis,
            handleScaleDirections,
            handleHandOffset,
            initialHandleDistance,
            initialHandleOrientationInverse,
            initialHandleRegistrationOffset,
            initialSelectionOrientationInverse,
            laserOffset,
            MIN_SCALE = 0.001,

            getIntersection,  // Function.
            intersection;


        if (!this instanceof Editor) {
            return new Editor();
        }

        selection = new Selection(side);
        highlights = new Highlights(side);
        handles = new Handles(side);

        function setReferences(inputs, editor) {
            hand = inputs.hand();  // Object.
            laser = inputs.laser();  // Object.
            getIntersection = inputs.intersection;  // Function.
            otherEditor = editor;  // Object.

            laserOffset = laser.handOffset();  // Value.

            highlights.setHandHighlightRadius(hand.getNearGrabRadius());
        }


        function hoverHandle(overlayID) {
            // Highlights handle if overlayID is a handle, otherwise unhighlights currently highlighted handle if any.
            handles.hover(overlayID);
        }

        function enableAutoGrab() {
            // Used to grab entity created from Create palette.
            isAutoGrab = true;
        }

        function isHandle(overlayID) {
            return handles.isHandle(overlayID);
        }

        function isEditing(aRootEntityID) {
            // aRootEntityID is an optional parameter.
            return editorState > EDITOR_HIGHLIGHTING
                && (aRootEntityID === undefined || aRootEntityID === rootEntityID);
        }

        function isScaling() {
            return editorState === EDITOR_DIRECT_SCALING || editorState === EDITOR_HANDLE_SCALING;
        }

        function getIntersectedEntityID() {
            return intersectedEntityID;
        }

        function getRootEntityID() {
            return rootEntityID;
        }


        function startEditing() {
            var selectionPositionAndOrientation;

            initialHandOrientationInverse = Quat.inverse(hand.orientation());
            selectionPositionAndOrientation = selection.getPositionAndOrientation();
            initialHandToSelectionVector = Vec3.subtract(selectionPositionAndOrientation.position, hand.position());
            initialSelectionOrientation = selectionPositionAndOrientation.orientation;

            selection.startEditing();
        }

        function finishEditing() {
            selection.finishEditing();
        }


        function getScaleTargetPosition() {
            if (isScalingWithHand) {
                return side === LEFT_HAND ? MyAvatar.getLeftPalmPosition() : MyAvatar.getRightPalmPosition();
            }
            return Vec3.sum(Vec3.sum(hand.position(), Vec3.multiplyQbyV(hand.orientation(), laserOffset)),
                Vec3.multiply(laser.length(), Quat.getUp(hand.orientation())));
        }

        function startDirectScaling(targetPosition) {
            // Called on grabbing hand by scaling hand.
            var initialTargetPosition,
                initialTargetsCenter;

            isScalingWithHand = intersection.handIntersected;

            otherTargetPosition = targetPosition;
            initialTargetPosition = getScaleTargetPosition();
            initialTargetsCenter = Vec3.multiply(0.5, Vec3.sum(initialTargetPosition, otherTargetPosition));
            initialTargetsSeparation = Vec3.distance(initialTargetPosition, otherTargetPosition);
            initialtargetsDirection = Vec3.subtract(otherTargetPosition, initialTargetPosition);

            selection.startDirectScaling(initialTargetsCenter);
            isDirectScaling = true;
        }

        function updateDirectScaling(targetPosition) {
            // Called on grabbing hand by scaling hand.
            otherTargetPosition = targetPosition;
        }

        function stopDirectScaling() {
            // Called on grabbing hand by scaling hand.
            selection.finishDirectScaling();
            isDirectScaling = false;
        }

        function startHandleScaling(targetPosition, overlayID) {
            // Called on grabbing hand by scaling hand.
            var initialTargetPosition,
                boundingBox,
                selectionPositionAndOrientation,
                scaleAxis,
                handDistance;

            isScalingWithHand = intersection.handIntersected;

            otherTargetPosition = targetPosition;

            // Keep grabbed handle highlighted and hide other handles.
            handles.grab(overlayID);

            // Vector from target to bounding box center.
            initialTargetPosition = getScaleTargetPosition();
            boundingBox = selection.boundingBox();
            initialTargetToBoundingBoxCenter = Vec3.subtract(boundingBox.center, initialTargetPosition);

            // Selection information.
            selectionPositionAndOrientation = selection.getPositionAndOrientation();
            initialSelectionOrientationInverse = Quat.inverse(selectionPositionAndOrientation.orientation);

            // Handle information.
            initialHandleOrientationInverse = Quat.inverse(hand.orientation());
            handleUnitScaleAxis = handles.scalingAxis(overlayID);  // Unit vector in direction of scaling.
            handleScaleDirections = handles.scalingDirections(overlayID);  // Which axes to scale the selection on.
            initialHandleDistance = Vec3.length(Vec3.multiplyVbyV(boundingBox.dimensions, handleScaleDirections)) / 2;
            initialHandleRegistrationOffset = Vec3.multiplyQbyV(initialSelectionOrientationInverse,
                Vec3.subtract(selectionPositionAndOrientation.position, boundingBox.center));

            // Distance from hand to handle in direction of handle.
            scaleAxis = Vec3.multiplyQbyV(selectionPositionAndOrientation.orientation, handleUnitScaleAxis);
            handDistance = Math.abs(Vec3.dot(Vec3.subtract(otherTargetPosition, boundingBox.center), scaleAxis));
            handleHandOffset = handDistance - initialHandleDistance;

            selection.startHandleScaling();
            handles.startScaling();
            isHandleScaling = true;
        }

        function updateHandleScaling(targetPosition) {
            // Called on grabbing hand by scaling hand.
            otherTargetPosition = targetPosition;
        }

        function stopHandleScaling() {
            // Called on grabbing hand by scaling hand.
            handles.finishScaling();
            selection.finishHandleScaling();
            handles.grab(null);  // Stop highlighting grabbed handle and resume displaying all handles.
            isHandleScaling = false;
        }


        function applyGrab() {
            // Sets position and orientation of selection per grabbing hand.
            var deltaOrientation,
                selectionPosition,
                selectionOrientation;

            deltaOrientation = Quat.multiply(hand.orientation(), initialHandOrientationInverse);
            selectionPosition = Vec3.sum(hand.position(), Vec3.multiplyQbyV(deltaOrientation, initialHandToSelectionVector));
            selectionOrientation = Quat.multiply(deltaOrientation, initialSelectionOrientation);

            selection.setPositionAndOrientation(selectionPosition, selectionOrientation);
        }

        function applyDirectScale() {
            // Scales, rotates, and positions selection per changing length, orientation, and position of vector between hands.
            var targetPosition,
                targetsSeparation,
                scale,
                rotation,
                center,
                selectionPositionAndOrientation;

            // Scale selection.
            targetPosition = getScaleTargetPosition();
            targetsSeparation = Vec3.distance(targetPosition, otherTargetPosition);
            scale = targetsSeparation / initialTargetsSeparation;
            scale = Math.max(scale, MIN_SCALE);

            rotation = Quat.rotationBetween(initialtargetsDirection, Vec3.subtract(otherTargetPosition, targetPosition));
            center = Vec3.multiply(0.5, Vec3.sum(targetPosition, otherTargetPosition));
            selection.directScale(scale, rotation, center);

            // Update grab offset.
            selectionPositionAndOrientation = selection.getPositionAndOrientation();
            initialHandOrientationInverse = Quat.inverse(hand.orientation());
            initialHandToSelectionVector = Vec3.subtract(selectionPositionAndOrientation.position, hand.position());
            initialSelectionOrientation = selectionPositionAndOrientation.orientation;
        }

        function applyHandleScale() {
            // Scales selection per changing position of scaling hand; positions and orients per grabbing hand.
            var targetPosition,
                deltaHandOrientation,
                deltaHandleOrientation,
                selectionPosition,
                selectionOrientation,
                boundingBoxCenter,
                scaleAxis,
                handleDistance,
                scale,
                scale3D,
                selectionPositionAndOrientation;

            // Orient selection per grabbing hand.
            deltaHandOrientation = Quat.multiply(hand.orientation(), initialHandOrientationInverse);
            selectionOrientation = Quat.multiply(deltaHandOrientation, initialSelectionOrientation);

            // Desired distance of handle from center of bounding box.
            targetPosition = getScaleTargetPosition();
            deltaHandleOrientation = Quat.multiply(hand.orientation(), initialHandleOrientationInverse);
            boundingBoxCenter = Vec3.sum(targetPosition,
                Vec3.multiplyQbyV(deltaHandleOrientation, initialTargetToBoundingBoxCenter));
            scaleAxis = Vec3.multiplyQbyV(selection.getPositionAndOrientation().orientation, handleUnitScaleAxis);
            handleDistance = Math.abs(Vec3.dot(Vec3.subtract(otherTargetPosition, boundingBoxCenter), scaleAxis));
            handleDistance -= handleHandOffset;
            handleDistance = Math.max(handleDistance, MIN_SCALE);

            // Scale selection relative to initial dimensions.
            scale = handleDistance / initialHandleDistance;
            scale3D = Vec3.multiply(scale, handleScaleDirections);
            scale3D = {
                x: handleScaleDirections.x !== 0 ? scale3D.x : 1,
                y: handleScaleDirections.y !== 0 ? scale3D.y : 1,
                z: handleScaleDirections.z !== 0 ? scale3D.z : 1
            };

            // Reposition selection per scale.
            selectionPosition = Vec3.sum(boundingBoxCenter,
                Vec3.multiplyQbyV(selectionOrientation, Vec3.multiplyVbyV(scale3D, initialHandleRegistrationOffset)));

            // Scale.
            handles.scale(scale3D);
            selection.handleScale(scale3D, selectionPosition, selectionOrientation);

            // Update grab offset.
            selectionPositionAndOrientation = selection.getPositionAndOrientation();
            initialHandOrientationInverse = Quat.inverse(hand.orientation());
            initialHandToSelectionVector = Vec3.subtract(selectionPositionAndOrientation.position, hand.position());
            initialSelectionOrientation = selectionPositionAndOrientation.orientation;
        }


        function enterEditorIdle() {
            laser.clear();
            selection.clear();
        }

        function exitEditorIdle() {
            // Nothing to do.
        }

        function enterEditorSearching() {
            selection.clear();
            intersectedEntityID = null;
            rootEntityID = null;
            hoveredOverlayID = intersection.overlayID;
            otherEditor.hoverHandle(hoveredOverlayID);
        }

        function updateEditorSearching() {
            if (toolSelected === TOOL_SCALE && intersection.overlayID !== hoveredOverlayID && otherEditor.isEditing()) {
                hoveredOverlayID = intersection.overlayID;
                otherEditor.hoverHandle(hoveredOverlayID);
            }
        }

        function exitEditorSearching() {
            otherEditor.hoverHandle(null);
        }

        function enterEditorHighlighting() {
            selection.select(intersectedEntityID);
            if (toolSelected !== TOOL_SCALE || !otherEditor.isEditing(rootEntityID)) {
                highlights.display(intersection.handIntersected, selection.selection(),
                    toolSelected === TOOL_COLOR ? selection.intersectedEntityIndex() : null,
                    toolSelected === TOOL_SCALE || otherEditor.isEditing(rootEntityID)
                        ? highlights.SCALE_COLOR : highlights.HIGHLIGHT_COLOR);
            }
            isOtherEditorEditingEntityID = otherEditor.isEditing(rootEntityID);
            wasScaleTool = toolSelected === TOOL_SCALE;
        }

        function updateEditorHighlighting() {
            selection.select(intersectedEntityID);
            if (toolSelected !== TOOL_SCALE || !otherEditor.isEditing(rootEntityID)) {
                highlights.display(intersection.handIntersected, selection.selection(),
                    toolSelected === TOOL_COLOR ? selection.intersectedEntityIndex() : null,
                    toolSelected === TOOL_SCALE || otherEditor.isEditing(rootEntityID)
                        ? highlights.SCALE_COLOR : highlights.HIGHLIGHT_COLOR);
            } else {
                highlights.clear();
            }
            isOtherEditorEditingEntityID = !isOtherEditorEditingEntityID;
        }

        function exitEditorHighlighting() {
            highlights.clear();
            isOtherEditorEditingEntityID = false;
        }

        function enterEditorGrabbing() {
            selection.select(intersectedEntityID);  // For when transitioning from EDITOR_SEARCHING.
            if (intersection.laserIntersected) {
                laser.setLength(laser.length());
            } else {
                laser.disable();
            }
            if (toolSelected === TOOL_SCALE) {
                handles.display(rootEntityID, selection.boundingBox(), selection.count() > 1);
            }
            startEditing();
            wasScaleTool = toolSelected === TOOL_SCALE;
        }

        function updateEditorGrabbing() {
            selection.select(intersectedEntityID);
            if (toolSelected === TOOL_SCALE) {
                handles.display(rootEntityID, selection.boundingBox(), selection.count() > 1);
            } else {
                handles.clear();
            }
        }

        function exitEditorGrabbing() {
            finishEditing();
            handles.clear();
            laser.clearLength();
            laser.enable();
        }

        function enterEditorDirectScaling() {
            selection.select(intersectedEntityID);  // In case need to transition to EDITOR_GRABBING.
            isScalingWithHand = intersection.handIntersected;
            if (intersection.laserIntersected) {
                laser.setLength(laser.length());
            } else {
                laser.disable();
            }
            otherEditor.startDirectScaling(getScaleTargetPosition());
        }

        function updateEditorDirectScaling() {
            otherEditor.updateDirectScaling(getScaleTargetPosition());
        }

        function exitEditorDirectScaling() {
            otherEditor.stopDirectScaling();
            laser.clearLength();
            laser.enable();
        }

        function enterEditorHandleScaling() {
            selection.select(intersectedEntityID);  // In case need to transition to EDITOR_GRABBING.
            isScalingWithHand = intersection.handIntersected;
            if (intersection.laserIntersected) {
                laser.setLength(laser.length());
            } else {
                laser.disable();
            }
            otherEditor.startHandleScaling(getScaleTargetPosition(), intersection.overlayID);
        }

        function updateEditorHandleScaling() {
            otherEditor.updateHandleScaling(getScaleTargetPosition());
        }

        function exitEditorHandleScaling() {
            otherEditor.stopHandleScaling();
            laser.clearLength();
            laser.enable();
        }

        function enterEditorCloning() {
            selection.select(intersectedEntityID);  // For when transitioning from EDITOR_SEARCHING.
            selection.cloneEntities();
            intersectedEntityID = selection.intersectedEntityID();
            rootEntityID = selection.rootEntityID();
            intersectedEntityID = rootEntityID;
        }

        function exitEditorCloning() {
            // Nothing to do.
        }

        function enterEditorGrouping() {
            if (!grouping.includes(rootEntityID)) {
                highlights.display(false, selection.selection(), null, highlights.GROUP_COLOR);
            }
            grouping.toggle(selection.selection());
        }

        function exitEditorGrouping() {
            highlights.clear();
        }

        STATE_MACHINE = {
            EDITOR_IDLE: {
                enter: enterEditorIdle,
                update: null,
                exit: exitEditorIdle
            },
            EDITOR_SEARCHING: {
                enter: enterEditorSearching,
                update: updateEditorSearching,
                exit: exitEditorSearching
            },
            EDITOR_HIGHLIGHTING: {
                enter: enterEditorHighlighting,
                update: updateEditorHighlighting,
                exit: exitEditorHighlighting
            },
            EDITOR_GRABBING: {
                enter: enterEditorGrabbing,
                update: updateEditorGrabbing,
                exit: exitEditorGrabbing
            },
            EDITOR_DIRECT_SCALING: {
                enter: enterEditorDirectScaling,
                update: updateEditorDirectScaling,
                exit: exitEditorDirectScaling
            },
            EDITOR_HANDLE_SCALING: {
                enter: enterEditorHandleScaling,
                update: updateEditorHandleScaling,
                exit: exitEditorHandleScaling
            },
            EDITOR_CLONING: {
                enter: enterEditorCloning,
                update: null,
                exit: exitEditorCloning
            },
            EDITOR_GROUPING: {
                enter: enterEditorGrouping,
                update: null,
                exit: exitEditorGrouping
            }
        };

        function setState(state) {
            if (state !== editorState) {
                STATE_MACHINE[EDITOR_STATE_STRINGS[editorState]].exit();
                STATE_MACHINE[EDITOR_STATE_STRINGS[state]].enter();
                editorState = state;
            } else {
                log(side, "ERROR: Editor: Null state transition: " + state + "!");
            }
        }

        function updateState() {
            STATE_MACHINE[EDITOR_STATE_STRINGS[editorState]].update();
        }


        function updateTool() {
            if (!wasGripClicked && isGripClicked && (toolSelected !== TOOL_NONE)) {
                toolSelected = TOOL_NONE;
                grouping.clear();
                ui.clearTool();
                ui.updateUIEntities();
            }
        }


        function update() {
            var previousState = editorState,
                doUpdateState,
                color;

            intersection = getIntersection();
            isTriggerClicked = hand.triggerClicked();
            isGripClicked = hand.gripClicked();

            // State update.
            switch (editorState) {
            case EDITOR_IDLE:
                if (!hand.valid()) {
                    // No transition.
                    break;
                }
                setState(EDITOR_SEARCHING);
                break;
            case EDITOR_SEARCHING:
                if (hand.valid()
                        && (!intersection.entityID || !intersection.editableEntity)
                        && !(intersection.overlayID && !wasTriggerClicked && isTriggerClicked
                            && otherEditor.isHandle(intersection.overlayID))) {
                    // No transition.
                    updateState();
                    updateTool();
                    break;
                }
                if (!hand.valid()) {
                    setState(EDITOR_IDLE);
                } else if (intersection.overlayID && !wasTriggerClicked && isTriggerClicked
                        && otherEditor.isHandle(intersection.overlayID)) {
                    intersectedEntityID = otherEditor.intersectedEntityID();
                    rootEntityID = otherEditor.rootEntityID();
                    setState(EDITOR_HANDLE_SCALING);
                } else if (intersection.entityID && intersection.editableEntity
                        && (wasTriggerClicked || !isTriggerClicked) && !isAutoGrab) {
                    intersectedEntityID = intersection.entityID;
                    rootEntityID = Entities.rootOf(intersectedEntityID);
                    setState(EDITOR_HIGHLIGHTING);
                } else if (intersection.entityID && intersection.editableEntity
                        && (!wasTriggerClicked || isAutoGrab) && isTriggerClicked) {
                    intersectedEntityID = intersection.entityID;
                    rootEntityID = Entities.rootOf(intersectedEntityID);
                    if (otherEditor.isEditing(rootEntityID)) {
                        if (toolSelected !== TOOL_SCALE) {
                            setState(EDITOR_DIRECT_SCALING);
                        }
                    } else if (toolSelected === TOOL_CLONE) {
                        setState(EDITOR_CLONING);
                    } else if (toolSelected === TOOL_GROUP) {
                        setState(EDITOR_GROUPING);
                    } else if (toolSelected === TOOL_COLOR) {
                        setState(EDITOR_HIGHLIGHTING);
                        selection.applyColor(colorToolColor, false);
                    } else if (toolSelected === TOOL_PICK_COLOR) {
                        color = selection.getColor(intersection.entityID);
                        if (color) {
                            colorToolColor = color;
                            ui.doPickColor(colorToolColor);
                        }
                        toolSelected = TOOL_COLOR;
                        ui.setToolIcon(ui.COLOR_TOOL);
                    } else if (toolSelected === TOOL_PHYSICS) {
                        setState(EDITOR_HIGHLIGHTING);
                        selection.applyPhysics(physicsToolPhysics);
                    } else if (toolSelected === TOOL_DELETE) {
                        setState(EDITOR_HIGHLIGHTING);
                        selection.deleteEntities();
                        setState(EDITOR_SEARCHING);
                    } else {
                        setState(EDITOR_GRABBING);
                    }
                } else {
                    log(side, "ERROR: Editor: Unexpected condition in EDITOR_SEARCHING!");
                }
                break;
            case EDITOR_HIGHLIGHTING:
                if (hand.valid()
                        && intersection.entityID && intersection.editableEntity
                        && !(!wasTriggerClicked && isTriggerClicked
                            && (!otherEditor.isEditing(rootEntityID) || toolSelected !== TOOL_SCALE))
                        && !(!wasTriggerClicked && isTriggerClicked && intersection.overlayID
                            && otherEditor.isHandle(intersection.overlayID))) {
                    // No transition.
                    doUpdateState = false;
                    if (otherEditor.isEditing(rootEntityID) !== isOtherEditorEditingEntityID) {
                        doUpdateState = true;
                    }
                    if (Entities.rootOf(intersection.entityID) !== rootEntityID) {
                        intersectedEntityID = intersection.entityID;
                        rootEntityID = Entities.rootOf(intersectedEntityID);
                        doUpdateState = true;
                    }
                    if ((toolSelected === TOOL_SCALE) !== wasScaleTool) {
                        wasScaleTool = toolSelected === TOOL_SCALE;
                        doUpdateState = true;
                    }
                    if (toolSelected === TOOL_COLOR && intersection.entityID !== intersectedEntityID) {
                        intersectedEntityID = intersection.entityID;
                        doUpdateState = true;
                    }
                    // TODO: For development testing, update intersectedEntityID so that physics can be applied to it.
                    if ((toolSelected === TOOL_COLOR || toolSelected === TOOL_PHYSICS)
                            && intersection.entityID !== intersectedEntityID) {
                        intersectedEntityID = intersection.entityID;
                        doUpdateState = true;
                    }
                    if (doUpdateState) {
                        updateState();
                    }
                    updateTool();
                    break;
                }
                if (!hand.valid()) {
                    setState(EDITOR_IDLE);
                } else if (intersection.overlayID && !wasTriggerClicked && isTriggerClicked
                        && otherEditor.isHandle(intersection.overlayID)) {
                    intersectedEntityID = otherEditor.intersectedEntityID();
                    rootEntityID = otherEditor.rootEntityID();
                    setState(EDITOR_HANDLE_SCALING);
                } else if (intersection.entityID && intersection.editableEntity && !wasTriggerClicked && isTriggerClicked) {
                    intersectedEntityID = intersection.entityID;  // May be a different entityID.
                    rootEntityID = Entities.rootOf(intersectedEntityID);
                    if (otherEditor.isEditing(rootEntityID)) {
                        if (toolSelected !== TOOL_SCALE) {
                            setState(EDITOR_DIRECT_SCALING);
                        } else {
                            log(side, "ERROR: Editor: Unexpected condition A in EDITOR_HIGHLIGHTING!");
                        }
                    } else if (toolSelected === TOOL_CLONE) {
                        setState(EDITOR_CLONING);
                    } else if (toolSelected === TOOL_GROUP) {
                        setState(EDITOR_GROUPING);
                    } else if (toolSelected === TOOL_COLOR) {
                        selection.applyColor(colorToolColor, false);
                    } else if (toolSelected === TOOL_PICK_COLOR) {
                        color = selection.getColor(intersection.entityID);
                        if (color) {
                            colorToolColor = color;
                            ui.doPickColor(colorToolColor);
                        }
                        toolSelected = TOOL_COLOR;
                        ui.setToolIcon(ui.COLOR_TOOL);
                    } else if (toolSelected === TOOL_PHYSICS) {
                        selection.applyPhysics(physicsToolPhysics);
                    } else if (toolSelected === TOOL_DELETE) {
                        selection.deleteEntities();
                        setState(EDITOR_SEARCHING);
                    } else {
                        setState(EDITOR_GRABBING);
                    }
                } else if (!intersection.entityID || !intersection.editableEntity) {
                    setState(EDITOR_SEARCHING);
                } else {
                    log(side, "ERROR: Editor: Unexpected condition B in EDITOR_HIGHLIGHTING!");
                }
                break;
            case EDITOR_GRABBING:
                if (hand.valid() && isTriggerClicked && !isGripClicked) {
                    // Don't test for intersection.intersected because when scaling with handles intersection may lag behind.
                    // No transition.
                    if ((toolSelected === TOOL_SCALE) !== wasScaleTool) {
                        updateState();
                        wasScaleTool = toolSelected === TOOL_SCALE;
                    }
                    //updateTool();  Don't updateTool() because grip button is used to delete grabbed entity.
                    break;
                }
                if (!hand.valid()) {
                    setState(EDITOR_IDLE);
                } else if (!isTriggerClicked) {
                    if (intersection.entityID && intersection.editableEntity) {
                        intersectedEntityID = intersection.entityID;
                        rootEntityID = Entities.rootOf(intersectedEntityID);
                        setState(EDITOR_HIGHLIGHTING);
                    } else {
                        setState(EDITOR_SEARCHING);
                    }
                } else if (isGripClicked) {
                    if (!wasGripClicked) {
                        selection.deleteEntities();
                        setState(EDITOR_SEARCHING);
                    }
                } else {
                    log(side, "ERROR: Editor: Unexpected condition in EDITOR_GRABBING!");
                }
                break;
            case EDITOR_DIRECT_SCALING:
                if (hand.valid() && isTriggerClicked
                        && (otherEditor.isEditing(rootEntityID) || otherEditor.isHandle(intersection.overlayID))) {
                    // Don't test for intersection.intersected because when scaling with handles intersection may lag behind.
                    // Don't test toolSelected === TOOL_SCALE because this is a UI element and so not able to be changed while 
                    // scaling with two hands.
                    // No transition.
                    updateState();
                    // updateTool();  Don't updateTool() because this hand is currently using the scaling tool.
                    break;
                }
                if (!hand.valid()) {
                    setState(EDITOR_IDLE);
                } else if (!isTriggerClicked) {
                    if (!intersection.entityID || !intersection.editableEntity) {
                        setState(EDITOR_SEARCHING);
                    } else {
                        intersectedEntityID = intersection.entityID;
                        rootEntityID = Entities.rootOf(intersectedEntityID);
                        setState(EDITOR_HIGHLIGHTING);
                    }
                } else if (!otherEditor.isEditing(rootEntityID)) {
                    // Grab highlightEntityID that was scaling and has already been set.
                    setState(EDITOR_GRABBING);
                } else {
                    log(side, "ERROR: Editor: Unexpected condition in EDITOR_DIRECT_SCALING!");
                }
                break;
            case EDITOR_HANDLE_SCALING:
                if (hand.valid() && isTriggerClicked && otherEditor.isEditing(rootEntityID)) {
                    // Don't test for intersection.intersected because when scaling with handles intersection may lag behind.
                    // Don't test toolSelected === TOOL_SCALE because this is a UI element and so not able to be changed while 
                    // scaling with two hands.
                    // No transition.
                    updateState();
                    updateTool();
                    break;
                }
                if (!hand.valid()) {
                    setState(EDITOR_IDLE);
                } else if (!isTriggerClicked) {
                    if (!intersection.entityID || !intersection.editableEntity) {
                        setState(EDITOR_SEARCHING);
                    } else {
                        intersectedEntityID = intersection.entityID;
                        rootEntityID = Entities.rootOf(intersectedEntityID);
                        setState(EDITOR_HIGHLIGHTING);
                    }
                } else if (!otherEditor.isEditing(rootEntityID)) {
                    // Grab highlightEntityID that was scaling and has already been set.
                    setState(EDITOR_GRABBING);
                } else {
                    log(side, "ERROR: Editor: Unexpected condition in EDITOR_HANDLE_SCALING!");
                }
                break;
            case EDITOR_CLONING:
                // Immediate transition out of state after cloning entities during state entry.
                if (hand.valid() && isTriggerClicked) {
                    setState(EDITOR_GRABBING);
                } else if (!hand.valid()) {
                    setState(EDITOR_IDLE);
                } else if (!isTriggerClicked) {
                    if (intersection.entityID && intersection.editableEntity) {
                        intersectedEntityID = intersection.entityID;
                        rootEntityID = Entities.rootOf(intersectedEntityID);
                        setState(EDITOR_HIGHLIGHTING);
                    } else {
                        setState(EDITOR_SEARCHING);
                    }
                } else {
                    log(side, "ERROR: Editor: Unexpected condition in EDITOR_CLONING!");
                }
                break;
            case EDITOR_GROUPING:
                if (hand.valid() && isTriggerClicked) {
                    // No transition.
                    break;
                }
                if (!hand.valid()) {
                    setState(EDITOR_IDLE);
                } else {
                    setState(EDITOR_SEARCHING);
                }
                break;
            }

            wasTriggerClicked = isTriggerClicked;
            wasGripClicked = isGripClicked;
            isAutoGrab = isAutoGrab && isTriggerClicked;

            if (DEBUG && editorState !== previousState) {
                debug(side, EDITOR_STATE_STRINGS[editorState]);
            }
        }

        function apply() {
            switch (editorState) {
            case EDITOR_GRABBING:
                if (isDirectScaling) {
                    applyDirectScale();
                } else if (isHandleScaling) {
                    applyHandleScale();
                } else {
                    applyGrab();
                }
                break;
            }
        }

        function clear() {
            if (editorState !== EDITOR_IDLE) {
                setState(EDITOR_IDLE);
            }

            selection.clear();
            highlights.clear();
            handles.clear();
        }

        function destroy() {
            if (selection) {
                selection.destroy();
                selection = null;
            }
            if (highlights) {
                highlights.destroy();
                highlights = null;
            }
            if (handles) {
                handles.destroy();
                handles = null;
            }
        }

        return {
            setReferences: setReferences,
            hoverHandle: hoverHandle,
            enableAutoGrab: enableAutoGrab,
            isHandle: isHandle,
            isEditing: isEditing,
            isScaling: isScaling,
            intersectedEntityID: getIntersectedEntityID,
            rootEntityID: getRootEntityID,
            startDirectScaling: startDirectScaling,
            updateDirectScaling: updateDirectScaling,
            stopDirectScaling: stopDirectScaling,
            startHandleScaling: startHandleScaling,
            updateHandleScaling: updateHandleScaling,
            stopHandleScaling: stopHandleScaling,
            update: update,
            apply: apply,
            clear: clear,
            destroy: destroy
        };
    };


    Grouping = function () {
        // Grouping highlights and functions.

        var groups,
            highlights,
            exludedLeftRootEntityID = null,
            exludedrightRootEntityID = null,
            excludedRootEntityIDs = [],
            hasHighlights = false,
            hasSelectionChanged = false;

        if (!this instanceof Grouping) {
            return new Grouping();
        }

        groups = new Groups();
        highlights = new Highlights();

        function toggle(selection) {
            groups.toggle(selection);
            if (groups.groupsCount() === 0) {
                hasHighlights = false;
                highlights.clear();
            } else {
                hasHighlights = true;
                hasSelectionChanged = true;
            }
        }

        function includes(rootEntityID) {
            return groups.includes(rootEntityID);
        }

        function groupsCount() {
            return groups.groupsCount();
        }

        function entitiesCount() {
            return groups.entitiesCount();
        }

        function group() {
            groups.group();
        }

        function ungroup() {
            groups.ungroup();
        }

        function update(leftRootEntityID, rightRootEntityID) {
            var hasExludedRootEntitiesChanged;

            hasExludedRootEntitiesChanged = leftRootEntityID !== exludedLeftRootEntityID
                || rightRootEntityID !== exludedrightRootEntityID;

            if (!hasHighlights || (!hasSelectionChanged && !hasExludedRootEntitiesChanged)) {
                return;
            }

            if (hasExludedRootEntitiesChanged) {
                excludedRootEntityIDs = [];
                if (leftRootEntityID) {
                    excludedRootEntityIDs.push(leftRootEntityID);
                }
                if (rightRootEntityID) {
                    excludedRootEntityIDs.push(rightRootEntityID);
                }
                exludedLeftRootEntityID = leftRootEntityID;
                exludedrightRootEntityID = rightRootEntityID;
            }

            highlights.display(false, groups.selection(excludedRootEntityIDs), null, highlights.GROUP_COLOR);

            hasSelectionChanged = false;
        }

        function clear() {
            groups.clear();
            highlights.clear();
        }

        function destroy() {
            if (groups) {
                groups.destroy();
                groups = null;
            }
            if (highlights) {
                highlights.destroy();
                highlights = null;
            }
        }

        return {
            toggle: toggle,
            includes: includes,
            groupsCount: groupsCount,
            entitiesCount: entitiesCount,
            group: group,
            ungroup: ungroup,
            update: update,
            clear: clear,
            destroy: destroy
        };
    };


    function update() {
        // Main update loop.
        updateTimer = null;

        // Update inputs - hands and lasers.
        inputs[LEFT_HAND].update();
        inputs[RIGHT_HAND].update();

        // UI has first dibs on handling inputs.
        ui.update();

        // Each hand's edit action depends on the state of the other hand, so update the states first then apply actions.
        editors[LEFT_HAND].update();
        editors[RIGHT_HAND].update();
        editors[LEFT_HAND].apply();
        editors[RIGHT_HAND].apply();

        // Grouping display.
        grouping.update(editors[LEFT_HAND].rootEntityID(), editors[RIGHT_HAND].rootEntityID());

        updateTimer = Script.setTimeout(update, UPDATE_LOOP_TIMEOUT);
    }

    function updateHandControllerGrab() {
        // Communicate app status to handControllerGrab.js.
        Settings.setValue(VR_EDIT_SETTING, isAppActive);
    }

    function onUICommand(command, parameter) {
        switch (command) {
        case "scaleTool":
            grouping.clear();
            toolSelected = TOOL_SCALE;
            ui.setToolIcon(ui.SCALE_TOOL);
            ui.updateUIEntities();
            break;
        case "cloneTool":
            grouping.clear();
            toolSelected = TOOL_CLONE;
            ui.setToolIcon(ui.CLONE_TOOL);
            ui.updateUIEntities();
            break;
        case "groupTool":
            toolSelected = TOOL_GROUP;
            ui.setToolIcon(ui.GROUP_TOOL);
            ui.updateUIEntities();
            break;
        case "colorTool":
            grouping.clear();
            toolSelected = TOOL_COLOR;
            ui.setToolIcon(ui.COLOR_TOOL);
            colorToolColor = parameter;
            ui.updateUIEntities();
            break;
        case "pickColorTool":
            grouping.clear();
            toolSelected = TOOL_PICK_COLOR;
            ui.setToolIcon(ui.COLOR_TOOL);
            ui.updateUIEntities();
            break;
        case "physicsTool":
            grouping.clear();
            toolSelected = TOOL_PHYSICS;
            ui.setToolIcon(ui.PHYSICS_TOOL);
            ui.updateUIEntities();
            break;
        case "deleteTool":
            grouping.clear();
            toolSelected = TOOL_DELETE;
            ui.setToolIcon(ui.DELETE_TOOL);
            ui.updateUIEntities();
            break;
        case "clearTool":
            ui.clearTool();
            ui.updateUIEntities();
            break;

        case "groupButton":
            grouping.group();
            break;
        case "ungroupButton":
            grouping.ungroup();
            break;

        case "setColor":
            if (toolSelected === TOOL_PICK_COLOR) {
                toolSelected = TOOL_COLOR;
                ui.setToolIcon(ui.COLOR_TOOL);
            }
            colorToolColor = parameter;
            break;

        case "setGravityOn":
            if (parameter) {
                physicsToolPhysics.gravity = { x: 0, y: physicsToolGravity, z: 0 };
                physicsToolPhysics.dynamic = true;
            } else {
                physicsToolPhysics.gravity = Vec3.ZERO;
                physicsToolPhysics.dynamic = false;
            }
            break;
        case "setGrabOn":
            physicsToolPhysics.userData.grabbableKey.grabbable = parameter;
            break;
        case "setCollideOn":
            if (parameter) {
                physicsToolPhysics.collisionless = false;
                physicsToolPhysics.collidesWith = "static,dynamic,kinematic,myAvatar,otherAvatar";
            } else {
                physicsToolPhysics.collisionless = true;
                physicsToolPhysics.collidesWith = "";
            }
            break;

        case "setGravity":
            if (parameter !== undefined) {
                // Power range 0.0, 0.5, 1.0 maps to -50.0, -9.80665, 50.0.
                physicsToolGravity = 82.36785162 * Math.pow(2.214065901, parameter) - 132.36785;
                if (physicsToolPhysics.dynamic === true) {  // Only apply if gravity is turned on.
                    physicsToolPhysics.gravity = { x: 0, y: physicsToolGravity, z: 0 };
                }
            }
            break;
        case "setBounce":
            if (parameter !== undefined) {
                // Linear range from 0.0, 0.5, 1.0 maps to 0.0, 0.5, 1.0;
                physicsToolPhysics.restitution = parameter;
            }
            break;
        case "setDamping":
            if (parameter !== undefined) {
                // Power range 0.0, 0.5, 1.0 maps to 0, 0.39, 1.0.
                physicsToolPhysics.damping = 0.69136364 * Math.pow(2.446416831, parameter) - 0.691364;
                // Power range 0.0, 0.5, 1.0 maps to 0, 0.3935, 1.0.
                physicsToolPhysics.angularDamping = 0.72695892 * Math.pow(2.375594, parameter) - 0.726959;
                // Linear range from 0.0, 0.5, 1.0 maps to 0.0, 0.5, 1.0;
                physicsToolPhysics.friction = parameter;
            }
            break;
        case "setDensity":
            if (parameter !== undefined) {
                // Power range 0.0, 0.5, 1.0 maps to 100, 1000, 10000.
                physicsToolPhysics.density = Math.pow(10, 2 + 2 * parameter);
            }
            break;

        case "autoGrab":
            if (dominantHand === LEFT_HAND) {
                editors[LEFT_HAND].enableAutoGrab();
            } else {
                editors[RIGHT_HAND].enableAutoGrab();
            }
            break;

        default:
            log("ERROR: Unexpected command in onUICommand(): " + command + ", " + parameter);
        }
    }

    function startApp() {
        ui.display();
        update();
    }

    function stopApp() {
        Script.clearTimeout(updateTimer);
        updateTimer = null;
        inputs[LEFT_HAND].clear();
        inputs[RIGHT_HAND].clear();
        ui.clear();
        grouping.clear();
        editors[LEFT_HAND].clear();
        editors[RIGHT_HAND].clear();
        toolSelected = TOOL_NONE;
    }

    function onAppButtonClicked() {
        // Application tablet/toolbar button clicked.
        isAppActive = !isAppActive;
        updateHandControllerGrab();
        button.editProperties({ isActive: isAppActive });

        if (isAppActive) {
            startApp();
        } else {
            stopApp();
        }
    }

    function onDominantHandChanged(hand) {
        dominantHand = hand === "left" ? LEFT_HAND : RIGHT_HAND;

        if (isAppActive) {
            // Stop operations.
            stopApp();
        }

        // Swap UI hands.
        ui.setHand(otherHand(dominantHand));

        if (isAppActive) {
            // Resume operations.
            startApp();
        }
    }


    function setUp() {
        updateHandControllerGrab();

        tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        if (!tablet) {
            return;
        }

        // Settings values.
        dominantHand = MyAvatar.getDominantHand() === "left" ? LEFT_HAND : RIGHT_HAND;

        // Tablet/toolbar button.
        button = tablet.addButton({
            icon: APP_ICON_INACTIVE,
            activeIcon: APP_ICON_ACTIVE,
            text: APP_NAME,
            isActive: isAppActive
        });
        if (button) {
            button.clicked.connect(onAppButtonClicked);
        }

        // Input objects.
        inputs[LEFT_HAND] = new Inputs(LEFT_HAND);
        inputs[RIGHT_HAND] = new Inputs(RIGHT_HAND);

        // UI object.
        ui = new UI(otherHand(dominantHand), inputs[LEFT_HAND], inputs[RIGHT_HAND], onUICommand);

        // Editor objects.
        editors[LEFT_HAND] = new Editor(LEFT_HAND);
        editors[RIGHT_HAND] = new Editor(RIGHT_HAND);
        editors[LEFT_HAND].setReferences(inputs[LEFT_HAND], editors[RIGHT_HAND]);
        editors[RIGHT_HAND].setReferences(inputs[RIGHT_HAND], editors[LEFT_HAND]);

        // Grouping object.
        grouping = new Grouping();

        // Settings changes.
        MyAvatar.dominantHandChanged.connect(onDominantHandChanged);

        // Start main update loop.
        if (isAppActive) {
            update();
        }
    }

    function tearDown() {
        if (updateTimer) {
            Script.clearTimeout(updateTimer);
        }

        isAppActive = false;
        updateHandControllerGrab();

        if (!tablet) {
            return;
        }

        if (button) {
            button.clicked.disconnect(onAppButtonClicked);
            tablet.removeButton(button);
            button = null;
        }

        if (grouping) {
            grouping.destroy();
            grouping = null;
        }

        if (editors[LEFT_HAND]) {
            editors[LEFT_HAND].destroy();
            editors[LEFT_HAND] = null;
        }
        if (editors[RIGHT_HAND]) {
            editors[RIGHT_HAND].destroy();
            editors[RIGHT_HAND] = null;
        }

        if (ui) {
            ui.destroy();
            ui = null;
        }

        if (inputs[LEFT_HAND]) {
            inputs[LEFT_HAND].destroy();
            inputs[LEFT_HAND] = null;
        }
        if (inputs[RIGHT_HAND]) {
            inputs[RIGHT_HAND].destroy();
            inputs[RIGHT_HAND] = null;
        }

        tablet = null;
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());
