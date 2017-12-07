//
//  shapes.js
//
//  Created by David Rowe on 27 Jun 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global Feedback, History, Preload */

(function () {

    "use strict";

    var APP_NAME = "SHAPES",
        APP_ICON_INACTIVE = Script.resolvePath("./assets/shapes-i.svg"),
        APP_ICON_ACTIVE = Script.resolvePath("./assets/shapes-a.svg"),
        APP_ICON_DISABLED = Script.resolvePath("./assets/shapes-d.svg"),
        ENABLED_CAPTION_COLOR_OVERRIDE = "#ffffff",
        DISABLED_CAPTION_COLOR_OVERRIDE = "#888888",
        START_DELAY = 2000, // ms

        // Application state
        isAppActive,
        dominantHand,

        // Tool state
        TOOL_NONE = 0,
        TOOL_SCALE = 1,
        TOOL_CLONE = 2,
        TOOL_GROUP = 3,
        TOOL_GROUP_BOX = 4,
        TOOL_COLOR = 5,
        TOOL_PICK_COLOR = 6,
        TOOL_PHYSICS = 7,
        TOOL_DELETE = 8,
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
        SelectionManager,
        ToolIcon,
        ToolsMenu,

        // Miscellaneous
        UPDATE_LOOP_TIMEOUT = 16,
        updateTimer = null,
        tablet,
        button,
        DOMAIN_CHANGED_MESSAGE = "Toolbar-DomainChanged",

        DEBUG = false;

    // Utilities
    Script.include("./utilities/utilities.js");

    // Modules
    Script.include("./modules/createPalette.js");
    Script.include("./modules/feedback.js");
    Script.include("./modules/groups.js");
    Script.include("./modules/hand.js");
    Script.include("./modules/handles.js");
    Script.include("./modules/highlights.js");
    Script.include("./modules/history.js");
    Script.include("./modules/laser.js");
    Script.include("./modules/preload.js");
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
        var NUMBER_OF_HANDS = 2;
        return (hand + 1) % NUMBER_OF_HANDS;
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


        if (!(this instanceof Inputs)) {
            return new Inputs();
        }

        hand = new Hand(side);
        laser = new Laser(side);


        function setUIOverlays(overlayIDs) {
            laser.setUIOverlays(overlayIDs);
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
            var laserIntersection,
                handIntersection;

            hand.update();
            if (hand.valid()) {
                laser.update(hand);
                // Use intersections in order to achieve entity manipulation while inside an entity:
                // - Use laser overlay intersection if there is one (for UI).
                // - Otherwise use hand overlay if there is one (for UI).
                // - Otherwise use laser entity intersection if there is one (for entity manipulation).
                //   Except if hand intersection is for same entity.
                // - Otherwise use hand entity intersection if there is one (for entity manipulation).
                laserIntersection = laser.intersection();
                if (laserIntersection.intersects && laserIntersection.overlayID !== null) {
                    intersection = laserIntersection;
                } else {
                    handIntersection = hand.intersection();
                    if (handIntersection.intersects && handIntersection.overlayID !== null) {
                        intersection = handIntersection;
                    } else if (laserIntersection.intersects && laserIntersection.entityID !== handIntersection.entityID) {
                        intersection = laserIntersection;
                    } else {
                        intersection = handIntersection;
                    }
                }
            } else {
                intersection = {};
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
            setUIOverlays: setUIOverlays,
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

            getIntersection; // Function.

        if (!(this instanceof UI)) {
            return new UI();
        }

        toolIcon = new ToolIcon(otherHand(side));
        createPalette = new CreatePalette(side, leftInputs, rightInputs, uiCommandCallback);
        toolsMenu = new ToolsMenu(side, leftInputs, rightInputs, uiCommandCallback);

        Preload.load(toolIcon.assetURLs());
        Preload.load(createPalette.assetURLs());
        Preload.load(toolsMenu.assetURLs());

        getIntersection = side === LEFT_HAND ? rightInputs.intersection : leftInputs.intersection;


        function setHand(newSide) {
            side = newSide;
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

        function setUIOverlays() {
            var uiOverlayIDs = [].concat(toolsMenu.overlayIDs(), createPalette.overlayIDs());
            leftInputs.setUIOverlays(side === RIGHT_HAND ? uiOverlayIDs : []);
            rightInputs.setUIOverlays(side === LEFT_HAND ? uiOverlayIDs : []);
        }

        function display() {
            toolsMenu.display();
            createPalette.display();
            setUIOverlays();
            isDisplaying = true;
        }

        function update() {
            var intersection;

            if (isDisplaying) {
                intersection = getIntersection();
                toolsMenu.update(intersection, grouping.groupsCount(), grouping.entitiesCount());
                createPalette.update(intersection.overlayID);
            }
        }

        function doPickColor(color) {
            toolsMenu.doCommand("setColorFromPick", color);
        }

        function clear() {
            leftInputs.setUIOverlays([]);
            rightInputs.setUIOverlays([]);
            toolIcon.clear();
            toolsMenu.clear();
            createPalette.clear();
            isDisplaying = false;
        }

        function setVisible(visible) {
            toolsMenu.setVisible(visible);
            createPalette.setVisible(visible);
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
            setVisible: setVisible,
            updateUIOverlays: setUIOverlays,
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
            otherEditor, // Other hand's Editor object.
            hand,
            laser,

            // Editor states.
            EDITOR_IDLE = 0,
            EDITOR_SEARCHING = 1,
            EDITOR_HIGHLIGHTING = 2, // Highlighting an entity (not hovering a handle).
            EDITOR_GRABBING = 3,
            EDITOR_DIRECT_SCALING = 4, // Scaling data are sent to other editor's EDITOR_GRABBING state.
            EDITOR_HANDLE_SCALING = 5, // ""
            EDITOR_CLONING = 6,
            EDITOR_GROUPING = 7,
            EDITOR_STATE_STRINGS = ["EDITOR_IDLE", "EDITOR_SEARCHING", "EDITOR_HIGHLIGHTING", "EDITOR_GRABBING",
                "EDITOR_DIRECT_SCALING", "EDITOR_HANDLE_SCALING", "EDITOR_CLONING", "EDITOR_GROUPING"],
            editorState = EDITOR_IDLE,

            // State machine.
            STATE_MACHINE,
            intersectedEntityID = null, // Intersected entity of highlighted entity set.
            rootEntityID = null, // Root entity of highlighted entity set.
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
            isDirectScaling = false, // Modifies EDITOR_GRABBING state.
            isHandleScaling = false, // ""
            initialTargetsSeparation,
            initialtargetsDirection,
            otherTargetPosition,
            handleUnitScaleAxis,
            handleScaleDirections,
            handleTargetOffset,
            initialHandToTargetOffset,
            initialHandleDistance,
            laserOffset,
            MIN_SCALE = 0.001,
            MIN_SCALE_HANDLE_DISTANCE = 0.0001,

            getIntersection, // Function.
            intersection,
            isUIVisible = true;


        if (!(this instanceof Editor)) {
            return new Editor();
        }

        selection = new SelectionManager(side);
        highlights = new Highlights(side);
        handles = new Handles(side);

        function setReferences(inputs, editor) {
            hand = inputs.hand(); // Object.
            laser = inputs.laser(); // Object.
            getIntersection = inputs.intersection; // Function.
            otherEditor = editor; // Object.

            laserOffset = laser.handOffset(); // Value.

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

        function setHandleOverlays(overlayIDs) {
            hand.setHandleOverlays(overlayIDs);
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

        function isCameraOutsideEntity(entityID, testPosition) {
            var cameraPosition,
                pickRay,
                PRECISION_PICKING = true,
                NO_EXCLUDE_IDS = [],
                VISIBLE_ONLY = true,
                intersection;

            cameraPosition = Camera.position;
            pickRay = {
                origin: cameraPosition,
                direction: Vec3.normalize(Vec3.subtract(testPosition, cameraPosition)),
                length: Vec3.distance(testPosition, cameraPosition)
            };
            intersection = Entities.findRayIntersection(pickRay, PRECISION_PICKING, [entityID], NO_EXCLUDE_IDS, VISIBLE_ONLY);
            return !intersection.intersects || intersection.distance < pickRay.length;
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
                return hand.palmPosition();
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
            if (isDirectScaling) {
                selection.finishDirectScaling();
                isDirectScaling = false;
            }
        }

        function startHandleScaling(targetPosition, overlayID) {
            // Called on grabbing hand by scaling hand.
            var initialTargetPosition,
                selectionPositionAndOrientation,
                scaleAxis;

            // Keep grabbed handle highlighted and hide other handles.
            handles.grab(overlayID);

            isScalingWithHand = intersection.handIntersected;

            // Grab center of selection.
            otherTargetPosition = targetPosition;
            initialTargetPosition = selection.boundingBox().center;
            initialHandToTargetOffset = Vec3.subtract(initialTargetPosition, hand.position());

            // Initial handle offset from center of selection.
            selectionPositionAndOrientation = selection.getPositionAndOrientation();
            handleUnitScaleAxis = handles.scalingAxis(overlayID); // Unit vector in direction of scaling.
            handleScaleDirections = handles.scalingDirections(overlayID); // Which axes to scale the selection on.
            scaleAxis = Vec3.multiplyQbyV(selectionPositionAndOrientation.orientation, handleUnitScaleAxis);
            handleTargetOffset = handles.handleOffset(overlayID)
                + Vec3.dot(Vec3.subtract(otherTargetPosition, Overlays.getProperty(overlayID, "position")), scaleAxis);
            initialHandleDistance = Math.abs(Vec3.dot(Vec3.subtract(otherTargetPosition, initialTargetPosition), scaleAxis));
            initialHandleDistance -= handleTargetOffset;
            initialHandleDistance = Math.max(initialHandleDistance, MIN_SCALE_HANDLE_DISTANCE);

            // Start scaling.
            selection.startHandleScaling(initialTargetPosition);
            handles.startScaling();
            isHandleScaling = true;
        }

        function updateHandleScaling(targetPosition) {
            // Called on grabbing hand by scaling hand.
            otherTargetPosition = targetPosition;
        }

        function stopHandleScaling() {
            // Called on grabbing hand by scaling hand.
            if (isHandleScaling) {
                handles.finishScaling();
                selection.finishHandleScaling();
                handles.grab(null); // Stop highlighting grabbed handle and resume displaying all handles.
                isHandleScaling = false;
            }
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
                selectionOrientation,
                scaleAxis,
                handleDistance,
                scale,
                scale3D,
                selectionPositionAndOrientation;

            // Orient selection per grabbing hand.
            deltaHandOrientation = Quat.multiply(hand.orientation(), initialHandOrientationInverse);
            selectionOrientation = Quat.multiply(deltaHandOrientation, initialSelectionOrientation);

            // Position selection per grabbing hand.
            targetPosition = Vec3.sum(hand.position(), Vec3.multiplyQbyV(deltaHandOrientation, initialHandToTargetOffset));

            // Desired distance of handle from other hand
            scaleAxis = Vec3.multiplyQbyV(selection.getPositionAndOrientation().orientation, handleUnitScaleAxis);
            handleDistance = Vec3.dot(Vec3.subtract(otherTargetPosition, targetPosition), scaleAxis);
            handleDistance -= handleTargetOffset;
            handleDistance = Math.max(handleDistance, MIN_SCALE_HANDLE_DISTANCE);

            // Scale selection relative to initial dimensions.
            scale = handleDistance / initialHandleDistance;
            scale = Math.max(scale, MIN_SCALE);
            scale3D = Vec3.multiply(scale, handleScaleDirections);
            scale3D = {
                x: handleScaleDirections.x !== 0 ? scale3D.x : 1,
                y: handleScaleDirections.y !== 0 ? scale3D.y : 1,
                z: handleScaleDirections.z !== 0 ? scale3D.z : 1
            };

            // Scale.
            handles.scale(scale3D);
            selection.handleScale(scale3D, targetPosition, selectionOrientation);

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
            if (!intersection.laserIntersected && !isUIVisible) {
                laser.disable();
            }
            if (toolSelected !== TOOL_SCALE || !otherEditor.isEditing(rootEntityID)) {
                highlights.display(intersection.handIntersected, selection.selection(),
                    toolSelected === TOOL_COLOR || toolSelected === TOOL_PICK_COLOR ? selection.intersectedEntityIndex() : null,
                    null,
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
                    toolSelected === TOOL_COLOR || toolSelected === TOOL_PICK_COLOR ? selection.intersectedEntityIndex() : null,
                    null,
                    toolSelected === TOOL_SCALE || otherEditor.isEditing(rootEntityID)
                        ? highlights.SCALE_COLOR : highlights.HIGHLIGHT_COLOR);
                if (!intersection.laserIntersected && !isUIVisible) {
                    laser.disable();
                } else {
                    laser.enable();
                }
            } else {
                highlights.clear();
                laser.enable();
            }
            isOtherEditorEditingEntityID = otherEditor.isEditing(rootEntityID);
        }

        function exitEditorHighlighting() {
            highlights.clear();
            isOtherEditorEditingEntityID = false;
            laser.enable();
        }

        function enterEditorGrabbing() {
            selection.select(intersectedEntityID); // For when transitioning from EDITOR_SEARCHING.
            if (intersection.laserIntersected) {
                laser.setLength(laser.length());
            } else {
                laser.disable();
            }
            if (toolSelected === TOOL_SCALE) {
                handles.display(rootEntityID, selection.boundingBox(), selection.count() > 1, selection.is2D());
                otherEditor.setHandleOverlays(handles.overlays());
            }
            startEditing();
            wasScaleTool = toolSelected === TOOL_SCALE;
        }

        function updateEditorGrabbing() {
            selection.select(intersectedEntityID);
            if (toolSelected === TOOL_SCALE) {
                handles.display(rootEntityID, selection.boundingBox(), selection.count() > 1, selection.is2D());
                otherEditor.setHandleOverlays(handles.overlays());
            } else {
                handles.clear();
                otherEditor.setHandleOverlays([]);
            }
        }

        function exitEditorGrabbing() {
            stopDirectScaling();
            stopHandleScaling();
            finishEditing();
            handles.clear();
            otherEditor.setHandleOverlays([]);
            laser.clearLength();
            laser.enable();
        }

        function enterEditorDirectScaling() {
            selection.select(intersectedEntityID); // In case need to transition to EDITOR_GRABBING.
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
            selection.select(intersectedEntityID); // In case need to transition to EDITOR_GRABBING.
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
            Feedback.play(side, Feedback.CLONE_ENTITY);
            selection.select(intersectedEntityID); // For when transitioning from EDITOR_SEARCHING.
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
                highlights.display(false, selection.selection(), null, null, highlights.GROUP_COLOR);
            }
            if (toolSelected === TOOL_GROUP_BOX) {
                if (!grouping.includes(rootEntityID)) {
                    Feedback.play(side, Feedback.SELECT_ENTITY);
                    grouping.toggle(selection.selection());
                    grouping.selectInBox();
                } else {
                    Feedback.play(side, Feedback.GENERAL_ERROR);
                }
            } else {
                Feedback.play(side, Feedback.SELECT_ENTITY);
                grouping.toggle(selection.selection());
            }
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
                Feedback.play(side, Feedback.DROP_TOOL);
                toolSelected = TOOL_NONE;
                grouping.clear();
                ui.clearTool();
                ui.updateUIOverlays();
            }
        }


        function update() {
            var isTriggerPressed,
                showUI,
                previousState = editorState,
                doUpdateState,
                color;

            intersection = getIntersection();
            isTriggerClicked = hand.triggerClicked();
            isGripClicked = hand.gripClicked();
            isTriggerPressed = hand.triggerPressed();

            // Hide UI if hand is intersecting entity and camera is outside entity, or if hand is intersecting stretch handle.
            if (side !== dominantHand) {
                showUI = !intersection.handIntersected || (intersection.entityID !== null
                    && !isCameraOutsideEntity(intersection.entityID, intersection.intersection));
                if (showUI !== isUIVisible) {
                    isUIVisible = !isUIVisible;
                    ui.setVisible(isUIVisible);
                }
            }

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
                            && !(intersection.overlayID && !wasTriggerClicked && isTriggerClicked
                                && otherEditor.isHandle(intersection.overlayID))
                            && !(intersection.entityID && (intersection.editableEntity || toolSelected === TOOL_PICK_COLOR)
                                && (wasTriggerClicked || !isTriggerClicked) && !isAutoGrab
                                && (isTriggerPressed 
                                    || isCameraOutsideEntity(intersection.entityID, intersection.intersection)))
                            && !(intersection.entityID && (intersection.editableEntity || toolSelected === TOOL_PICK_COLOR)
                                && (!wasTriggerClicked || isAutoGrab) && isTriggerClicked)) {
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
                    } else if (intersection.entityID && (intersection.editableEntity || toolSelected === TOOL_PICK_COLOR)
                            && (wasTriggerClicked || !isTriggerClicked) && !isAutoGrab
                            && (isTriggerPressed || isCameraOutsideEntity(intersection.entityID, intersection.intersection))) {
                        intersectedEntityID = intersection.entityID;
                        rootEntityID = Entities.rootOf(intersectedEntityID);
                        setState(EDITOR_HIGHLIGHTING);
                    } else if (intersection.entityID && (intersection.editableEntity || toolSelected === TOOL_PICK_COLOR)
                            && (!wasTriggerClicked || isAutoGrab) && isTriggerClicked) {
                        intersectedEntityID = intersection.entityID;
                        rootEntityID = Entities.rootOf(intersectedEntityID);
                        if (isAutoGrab) {
                            setState(EDITOR_GRABBING);
                        } else if (otherEditor.isEditing(rootEntityID)) {
                            if (toolSelected !== TOOL_SCALE) {
                                setState(EDITOR_DIRECT_SCALING);
                            }
                        } else if (toolSelected === TOOL_CLONE) {
                            setState(EDITOR_CLONING);
                        } else if (toolSelected === TOOL_GROUP || toolSelected === TOOL_GROUP_BOX) {
                            setState(EDITOR_GROUPING);
                        } else if (toolSelected === TOOL_COLOR) {
                            setState(EDITOR_HIGHLIGHTING);
                            if (selection.applyColor(colorToolColor, false)) {
                                Feedback.play(side, Feedback.APPLY_PROPERTY);
                            } else {
                                Feedback.play(side, Feedback.APPLY_ERROR);
                            }
                        } else if (toolSelected === TOOL_PICK_COLOR) {
                            color = selection.getColor(intersection.entityID);
                            if (color) {
                                colorToolColor = color;
                                ui.doPickColor(colorToolColor);
                            } else {
                                Feedback.play(side, Feedback.APPLY_ERROR);
                            }
                            toolSelected = TOOL_COLOR;
                            ui.setToolIcon(ui.COLOR_TOOL);
                        } else if (toolSelected === TOOL_PHYSICS) {
                            setState(EDITOR_HIGHLIGHTING);
                            selection.applyPhysics(physicsToolPhysics);
                        } else if (toolSelected === TOOL_DELETE) {
                            setState(EDITOR_HIGHLIGHTING);
                            Feedback.play(side, Feedback.DELETE_ENTITY);
                            selection.deleteEntities();
                            setState(EDITOR_SEARCHING);
                        } else {
                            log(side, "ERROR: Editor: Unexpected condition A in EDITOR_SEARCHING!");
                        }
                    } else {
                        log(side, "ERROR: Editor: Unexpected condition B in EDITOR_SEARCHING!");
                    }
                    break;
                case EDITOR_HIGHLIGHTING:
                    if (hand.valid()
                            && intersection.entityID && (intersection.editableEntity || toolSelected === TOOL_PICK_COLOR)
                                && (isTriggerPressed || isCameraOutsideEntity(intersection.entityID, intersection.intersection))
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
                    } else if (intersection.entityID && (intersection.editableEntity || toolSelected === TOOL_PICK_COLOR)
                            && !wasTriggerClicked && isTriggerClicked) {
                        intersectedEntityID = intersection.entityID; // May be a different entityID.
                        rootEntityID = Entities.rootOf(intersectedEntityID);
                        if (otherEditor.isEditing(rootEntityID)) {
                            if (toolSelected !== TOOL_SCALE) {
                                setState(EDITOR_DIRECT_SCALING);
                            } else {
                                log(side, "ERROR: Editor: Unexpected condition A in EDITOR_HIGHLIGHTING!");
                            }
                        } else if (toolSelected === TOOL_CLONE) {
                            setState(EDITOR_CLONING);
                        } else if (toolSelected === TOOL_GROUP || toolSelected === TOOL_GROUP_BOX) {
                            setState(EDITOR_GROUPING);
                        } else if (toolSelected === TOOL_COLOR) {
                            if (selection.applyColor(colorToolColor, false)) {
                                Feedback.play(side, Feedback.APPLY_PROPERTY);
                            } else {
                                Feedback.play(side, Feedback.APPLY_ERROR);
                            }
                        } else if (toolSelected === TOOL_PICK_COLOR) {
                            color = selection.getColor(intersection.entityID);
                            if (color) {
                                colorToolColor = color;
                                ui.doPickColor(colorToolColor);
                                toolSelected = TOOL_COLOR;
                                ui.setToolIcon(ui.COLOR_TOOL);
                            } else {
                                Feedback.play(side, Feedback.APPLY_ERROR);
                            }
                        } else if (toolSelected === TOOL_PHYSICS) {
                            selection.applyPhysics(physicsToolPhysics);
                        } else if (toolSelected === TOOL_DELETE) {
                            Feedback.play(side, Feedback.DELETE_ENTITY);
                            selection.deleteEntities();
                            setState(EDITOR_SEARCHING);
                        } else {
                            setState(EDITOR_GRABBING);
                        }

                    } else if (!intersection.entityID || !intersection.editableEntity
                            || (!isTriggerPressed && !isCameraOutsideEntity(intersection.entityID, intersection.intersection))) {
                        setState(EDITOR_SEARCHING);
                    } else {
                        log(side, "ERROR: Editor: Unexpected condition B in EDITOR_HIGHLIGHTING!");
                    }
                    break;
                case EDITOR_GRABBING:
                    if (hand.valid() && isTriggerClicked && !isGripClicked) {
                        // Don't test  intersection.intersected because when scaling with handles intersection may lag behind.
                        // No transition.
                        if ((toolSelected === TOOL_SCALE) !== wasScaleTool) {
                            updateState();
                            wasScaleTool = toolSelected === TOOL_SCALE;
                        }
                        // updateTool();  Don't updateTool() because grip button is used to delete grabbed entity.
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
                            Feedback.play(side, Feedback.DELETE_ENTITY);
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
                        // Don't test intersection.intersected because when scaling with handles intersection may lag behind.
                        // Don't test toolSelected === TOOL_SCALE because this is a UI element and so not able to be changed 
                        // while scaling with two hands.
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
                    // Immediate transition out of state after updating group data during state entry.
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
            isAutoGrab = isAutoGrab && isTriggerClicked && !isGripClicked;

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
            otherEditor.setHandleOverlays([]);
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
            setHandleOverlays: setHandleOverlays,
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
            selectInBoxSelection, // Selection of all entities selected.
            groupSelection, // New group to add to selection.
            exludedLeftRootEntityID = null,
            exludedrightRootEntityID = null,
            excludedRootEntityIDs = [],
            hasHighlights = false,
            hasSelectionChanged = false,
            isSelectInBox = false;

        if (!(this instanceof Grouping)) {
            return new Grouping();
        }

        groups = new Groups();
        highlights = new Highlights();
        selectInBoxSelection = new SelectionManager();
        groupSelection = new SelectionManager();

        function getAllChildrenIDs(entityID) {
            var childrenIDs = [],
                ENTITY_TYPE = "entity";

            function traverseEntityTree(id) {
                var children,
                    i,
                    length;
                children = Entities.getChildrenIDs(id);
                for (i = 0, length = children.length; i < length; i++) {
                    if (Entities.getNestableType(children[i]) === ENTITY_TYPE) {
                        childrenIDs.push(children[i]);
                        traverseEntityTree(children[i]);
                    }
                }
            }

            traverseEntityTree(entityID);
            return childrenIDs;
        }

        function isInsideBoundingBox(entityID, boundingBox) {
            // Are all 8 corners of entityID's bounding box inside boundingBox?
            var entityProperties,
                cornerPosition,
                boundingBoxInverseRotation,
                boundingBoxHalfDimensions,
                isInside = true,
                i,
                CORNER_REGISTRATION_OFFSETS = [
                    { x: 0, y: 0, z: 0 },
                    { x: 0, y: 0, z: 1 },
                    { x: 0, y: 1, z: 0 },
                    { x: 0, y: 1, z: 1 },
                    { x: 1, y: 0, z: 0 },
                    { x: 1, y: 0, z: 1 },
                    { x: 1, y: 1, z: 0 },
                    { x: 1, y: 1, z: 1 }
                ],
                NUM_CORNERS = 8;

            entityProperties = Entities.getEntityProperties(entityID, ["position", "rotation", "dimensions",
                "registrationPoint"]);

            // Convert entity coordinates into boundingBox coordinates.
            boundingBoxInverseRotation = Quat.inverse(boundingBox.orientation);
            entityProperties.position = Vec3.multiplyQbyV(boundingBoxInverseRotation,
                Vec3.subtract(entityProperties.position, boundingBox.center));
            entityProperties.rotation = Quat.multiply(boundingBoxInverseRotation, entityProperties.rotation);

            // Check all 8 corners of entity's bounding box are inside the given bounding box.
            boundingBoxHalfDimensions = Vec3.multiply(0.5, boundingBox.dimensions);
            i = 0;
            while (isInside && i < NUM_CORNERS) {
                cornerPosition = Vec3.sum(entityProperties.position, Vec3.multiplyQbyV(entityProperties.rotation,
                    Vec3.multiplyVbyV(Vec3.subtract(CORNER_REGISTRATION_OFFSETS[i], entityProperties.registrationPoint),
                        entityProperties.dimensions)));
                isInside = Math.abs(cornerPosition.x) <= boundingBoxHalfDimensions.x
                    && Math.abs(cornerPosition.y) <= boundingBoxHalfDimensions.y
                    && Math.abs(cornerPosition.z) <= boundingBoxHalfDimensions.z;
                i++;
            }

            return isInside;
        }

        function toggle(selection) {
            groups.toggle(selection);
            if (isSelectInBox) {
                // When selecting in a box, toggle() is only called to add entities to the selection.
                if (selectInBoxSelection.count() === 0) {
                    selectInBoxSelection.select(selection[0].id);
                } else {
                    selectInBoxSelection.append(selection[0].id);
                }
            }
            if (groups.groupsCount() === 0) {
                hasHighlights = false;
                highlights.clear();
            } else {
                hasHighlights = true;
                hasSelectionChanged = true;
            }
        }

        function selectInBox() {
            // Add any entities or groups of entities wholly within bounding box of current selection.
            // Must be wholly within otherwise selection could grow uncontrollably.
            var boundingBox,
                entityIDs,
                checkedEntityIDs = [],
                entityID,
                rootID,
                groupIDs,
                doIncludeGroup,
                i,
                lengthI,
                j,
                lengthJ;

            if (selectInBoxSelection.count() > 1) {
                boundingBox = selectInBoxSelection.boundingBox();
                entityIDs = Entities.findEntities(boundingBox.center, Vec3.length(boundingBox.dimensions) / 2);
                for (i = 0, lengthI = entityIDs.length; i < lengthI; i++) {
                    entityID = entityIDs[i];
                    if (checkedEntityIDs.indexOf(entityID) === -1) {
                        rootID = Entities.rootOf(entityID);
                        if (!selectInBoxSelection.contains(entityID) && Entities.hasEditableRoot(rootID)) {
                            groupIDs = [rootID].concat(getAllChildrenIDs(rootID));
                            doIncludeGroup = true;
                            j = 0;
                            lengthJ = groupIDs.length;
                            while (doIncludeGroup && j < lengthJ) {
                                doIncludeGroup = isInsideBoundingBox(groupIDs[j], boundingBox);
                                j++;
                            }
                            checkedEntityIDs = checkedEntityIDs.concat(groupIDs);
                            if (doIncludeGroup) {
                                groupSelection.select(rootID);
                                groups.toggle(groupSelection.selection());
                                groupSelection.clear();
                                selectInBoxSelection.append(rootID);
                                hasSelectionChanged = true;
                            }
                        } else {
                            checkedEntityIDs.push(rootID);
                        }
                    }
                }
            }
        }

        function startSelectInBox() {
            // Start automatically selecting entities in bounding box of current selection.
            var rootEntityIDs,
                i,
                length;

            isSelectInBox = true;

            // Select entities current groups combined.
            rootEntityIDs = groups.rootEntityIDs();
            if (rootEntityIDs.length > 0) {
                selectInBoxSelection.select(rootEntityIDs[0]);
                for (i = 1, length = rootEntityIDs.length; i < length; i++) {
                    selectInBoxSelection.append(rootEntityIDs[i]);
                }
            }

            // Add any enclosed entities.
            selectInBox();

            // Show bounding box overlay plus any newly selected entities.
            hasSelectionChanged = true;
        }

        function stopSelectInBox() {
            // Stop automatically selecting entities within bounding box of current selection.

            // Hide bounding box overlay.
            selectInBoxSelection.clear();
            hasSelectionChanged = true;

            isSelectInBox = false;
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
            // Update highlights displayed, excluding entities highlighted by left or right hands.
            var hasExludedRootEntitiesChanged,
                boundingBox;

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

            boundingBox = isSelectInBox && selectInBoxSelection.count() > 1 ? selectInBoxSelection.boundingBox() : null;
            highlights.display(false, groups.selection(excludedRootEntityIDs), null, boundingBox, highlights.GROUP_COLOR);
            hasSelectionChanged = false;
        }

        function clear() {
            if (isSelectInBox) {
                stopSelectInBox();
            }
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
            if (selectInBoxSelection) {
                selectInBoxSelection.destroy();
                selectInBoxSelection = null;
            }
            if (groupSelection) {
                groupSelection.destroy();
                groupSelection = null;
            }
        }

        return {
            toggle: toggle,
            startSelectInBox: startSelectInBox,
            selectInBox: selectInBox,
            stopSelectInBox: stopSelectInBox,
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

    function updateControllerDispatcher() {
        // Communicate app status to controllerDispatcher.js.
        var DISABLE_HANDS = "both",
            ENABLE_HANDS = "none";
        Messages.sendLocalMessage("Hifi-InVREdit-Disabler", isAppActive ? DISABLE_HANDS : ENABLE_HANDS);
    }

    function onUICommand(command, parameter) {
        switch (command) {
            case "scaleTool":
                Feedback.play(dominantHand, Feedback.EQUIP_TOOL);
                grouping.clear();
                toolSelected = TOOL_SCALE;
                ui.setToolIcon(ui.SCALE_TOOL);
                ui.updateUIOverlays();
                break;
            case "cloneTool":
                Feedback.play(dominantHand, Feedback.EQUIP_TOOL);
                grouping.clear();
                toolSelected = TOOL_CLONE;
                ui.setToolIcon(ui.CLONE_TOOL);
                ui.updateUIOverlays();
                break;
            case "groupTool":
                Feedback.play(dominantHand, Feedback.EQUIP_TOOL);
                toolSelected = TOOL_GROUP;
                ui.setToolIcon(ui.GROUP_TOOL);
                ui.updateUIOverlays();
                break;
            case "colorTool":
                Feedback.play(dominantHand, Feedback.EQUIP_TOOL);
                grouping.clear();
                toolSelected = TOOL_COLOR;
                ui.setToolIcon(ui.COLOR_TOOL);
                colorToolColor = parameter;
                ui.updateUIOverlays();
                break;
            case "pickColorTool":
                if (parameter) {
                    grouping.clear();
                    toolSelected = TOOL_PICK_COLOR;
                    ui.updateUIOverlays();
                } else {
                    Feedback.play(dominantHand, Feedback.EQUIP_TOOL);
                    grouping.clear();
                    toolSelected = TOOL_COLOR;
                    ui.updateUIOverlays();
                }
                break;
            case "physicsTool":
                Feedback.play(dominantHand, Feedback.EQUIP_TOOL);
                grouping.clear();
                toolSelected = TOOL_PHYSICS;
                ui.setToolIcon(ui.PHYSICS_TOOL);
                ui.updateUIOverlays();
                break;
            case "deleteTool":
                Feedback.play(dominantHand, Feedback.EQUIP_TOOL);
                grouping.clear();
                toolSelected = TOOL_DELETE;
                ui.setToolIcon(ui.DELETE_TOOL);
                ui.updateUIOverlays();
                break;
            case "clearTool":
                Feedback.play(dominantHand, Feedback.DROP_TOOL);
                grouping.clear();
                toolSelected = TOOL_NONE;
                ui.clearTool();
                ui.updateUIOverlays();
                break;

            case "groupButton":
                Feedback.play(dominantHand, Feedback.APPLY_PROPERTY);
                grouping.group();
                grouping.clear();
                toolSelected = TOOL_NONE;
                ui.clearTool();
                ui.updateUIOverlays();
                break;
            case "ungroupButton":
                Feedback.play(dominantHand, Feedback.APPLY_PROPERTY);
                grouping.ungroup();
                grouping.clear();
                toolSelected = TOOL_NONE;
                ui.clearTool();
                ui.updateUIOverlays();
                break;
            case "toggleGroupSelectionBoxTool":
                toolSelected = parameter ? TOOL_GROUP_BOX : TOOL_GROUP;
                if (toolSelected === TOOL_GROUP_BOX) {
                    grouping.startSelectInBox();
                } else {
                    grouping.stopSelectInBox();
                }
                break;
            case "clearGroupSelectionTool":
                if (grouping.groupsCount() > 0) {
                    Feedback.play(dominantHand, Feedback.SELECT_ENTITY);
                }
                grouping.clear();
                if (toolSelected === TOOL_GROUP_BOX) {
                    grouping.startSelectInBox();
                }
                break;

            case "setColor":
                if (toolSelected === TOOL_PICK_COLOR) {
                    toolSelected = TOOL_COLOR;
                    ui.setToolIcon(ui.COLOR_TOOL);
                }
                colorToolColor = parameter;
                break;

            case "setGravityOn":
                // Dynamic is true if the entity has gravity or is grabbable.
                if (parameter) {
                    physicsToolPhysics.gravity = { x: 0, y: physicsToolGravity, z: 0 };
                    physicsToolPhysics.dynamic = true;
                } else {
                    physicsToolPhysics.gravity = Vec3.ZERO;
                    physicsToolPhysics.dynamic = physicsToolPhysics.userData.grabbableKey.grabbable === true;
                }
                break;
            case "setGrabOn":
                // Dynamic is true if the entity has gravity or is grabbable.
                physicsToolPhysics.userData.grabbableKey.grabbable = parameter;
                physicsToolPhysics.dynamic = parameter
                    || (physicsToolPhysics.gravity && Vec3.length(physicsToolPhysics.gravity) > 0);
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
                    if (physicsToolPhysics.dynamic === true) { // Only apply if gravity is turned on.
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
            case "setFriction":
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

            case "undoAction":
                if (History.hasUndo()) {
                    Feedback.play(dominantHand, Feedback.UNDO_ACTION);
                    History.undo();
                } else {
                    Feedback.play(dominantHand, Feedback.GENERAL_ERROR);
                }
                break;
            case "redoAction":
                if (History.hasRedo()) {
                    Feedback.play(dominantHand, Feedback.REDO_ACTION);
                    History.redo();
                } else {
                    Feedback.play(dominantHand, Feedback.GENERAL_ERROR);
                }
                break;

            default:
                log("ERROR: Unexpected command in onUICommand(): " + command + ", " + parameter);
        }
    }

    function startApp() {
        ui.display();
        update(); // Start main update loop.
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
        var NOTIFICATIONS_MESSAGE_CHANNEL = "Hifi-Notifications",
            EDIT_ERROR = 4, // Per notifications.js.
            INSUFFICIENT_PERMISSIONS_ERROR_MSG =
                "You do not have the necessary permissions to edit on this domain."; // Same as edit.js.

        // Application tablet/toolbar button clicked.
        if (!isAppActive && !(Entities.canRez() || Entities.canRezTmp())) {
            Feedback.play(dominantHand, Feedback.GENERAL_ERROR);
            Messages.sendLocalMessage(NOTIFICATIONS_MESSAGE_CHANNEL, JSON.stringify({
                message: INSUFFICIENT_PERMISSIONS_ERROR_MSG,
                notificationType: EDIT_ERROR
            }));
            return;
        }

        isAppActive = !isAppActive;
        updateControllerDispatcher();
        button.editProperties({ isActive: isAppActive });

        if (isAppActive) {
            startApp();
        } else {
            stopApp();
        }
    }

    function onDomainChanged() {
        // Fires when domain starts or domain changes; does not fire when domain stops.
        var hasRezPermissions = Entities.canRez() || Entities.canRezTmp();
        if (isAppActive && !hasRezPermissions) {
            isAppActive = false;
            updateControllerDispatcher();
            stopApp();
        }
        button.editProperties({
            icon: hasRezPermissions ? APP_ICON_INACTIVE : APP_ICON_DISABLED,
            captionColor: hasRezPermissions ? ENABLED_CAPTION_COLOR_OVERRIDE : DISABLED_CAPTION_COLOR_OVERRIDE,
            isActive: isAppActive
        });
    }

    function onCanRezChanged() {
        // canRez or canRezTmp changed.
        var hasRezPermissions = Entities.canRez() || Entities.canRezTmp();
        if (isAppActive && !hasRezPermissions) {
            isAppActive = false;
            updateControllerDispatcher();
            stopApp();
        }
        button.editProperties({
            icon: hasRezPermissions ? APP_ICON_INACTIVE : APP_ICON_DISABLED,
            captionColor: hasRezPermissions ? ENABLED_CAPTION_COLOR_OVERRIDE : DISABLED_CAPTION_COLOR_OVERRIDE,
            isActive: isAppActive
        });
    }

    function onMessageReceived(channel) {
        // Hacky but currently the only way of detecting server stopping or restarting. Also occurs if changing domains.
        // TODO: Remove this when Window.domainChanged or other signal is emitted when you disconnect from a domain.
        if (channel === DOMAIN_CHANGED_MESSAGE) {
            // Happens a little while after server goes away.
            if (isAppActive && !location.isConnected) {
                // Interface deletes all overlays when domain connection is lost; restart app to work around this.
                stopApp();
                startApp();
            }
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

    function onSkeletonChanged() {
        if (isAppActive) {
            // Close the app because the new avatar may have different joint numbers meaning that the UI would be attached 
            // incorrectly. Let the user reopen the app because it can take some time for the new avatar to load.
            isAppActive = false;
            updateControllerDispatcher();
            button.editProperties({ isActive: false });
            stopApp();
        }
    }


    function setUp() {
        var hasRezPermissions;

        tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        if (!tablet) {
            App.log("ERROR: Tablet not found! App not started.");
            return;
        }

        // Application state.
        isAppActive = false;
        updateControllerDispatcher();
        dominantHand = MyAvatar.getDominantHand() === "left" ? LEFT_HAND : RIGHT_HAND;

        // Tablet/toolbar button.
        hasRezPermissions = Entities.canRez() || Entities.canRezTmp();
        button = tablet.addButton({
            icon: hasRezPermissions ? APP_ICON_INACTIVE : APP_ICON_DISABLED,
            captionColor: hasRezPermissions ? ENABLED_CAPTION_COLOR_OVERRIDE : DISABLED_CAPTION_COLOR_OVERRIDE,
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

        // Changes.
        Window.domainChanged.connect(onDomainChanged);
        Entities.canRezChanged.connect(onCanRezChanged);
        Entities.canRezTmpChanged.connect(onCanRezChanged);
        Messages.subscribe(DOMAIN_CHANGED_MESSAGE);
        Messages.messageReceived.connect(onMessageReceived);
        MyAvatar.dominantHandChanged.connect(onDominantHandChanged);
        MyAvatar.skeletonChanged.connect(onSkeletonChanged);
    }

    function tearDown() {
        if (!tablet) {
            return;
        }

        if (updateTimer) {
            Script.clearTimeout(updateTimer);
        }

        Window.domainChanged.disconnect(onDomainChanged);
        Entities.canRezChanged.disconnect(onCanRezChanged);
        Entities.canRezTmpChanged.disconnect(onCanRezChanged);
        Messages.messageReceived.disconnect(onMessageReceived);
        // Messages.unsubscribe(DOMAIN_CHANGED_MESSAGE);  Do not unsubscribe because edit.js also subscribes and 
        // Messages.subscribe works script engine-wide which would mess things up if they're both run in the same engine.
        MyAvatar.dominantHandChanged.disconnect(onDominantHandChanged);
        MyAvatar.skeletonChanged.disconnect(onSkeletonChanged);

        isAppActive = false;
        updateControllerDispatcher();

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

    Script.setTimeout(setUp, START_DELAY); // Delay start so that Entities.canRez() work; button is enabled correctly.
    Script.scriptEnding.connect(tearDown);
}());
