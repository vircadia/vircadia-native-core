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
        isAppScaleWithHandles = false,
        dominantHand,

        // Primary objects
        Inputs,
        inputs = [],
        UI,
        ui,
        Editor,
        editors = [],
        LEFT_HAND = 0,
        RIGHT_HAND = 1,

        // Modules
        Hand,
        Handles,
        Highlights,
        Laser,
        Selection,
        ToolMenu,

        // Miscellaneous
        UPDATE_LOOP_TIMEOUT = 16,
        updateTimer = null,
        tablet,
        button,

        DEBUG = true;  // TODO: Set false.

    // Utilities
    Script.include("./utilities/utilities.js");

    // Modules
    Script.include("./modules/hand.js");
    Script.include("./modules/handles.js");
    Script.include("./modules/highlights.js");
    Script.include("./modules/laser.js");
    Script.include("./modules/selection.js");
    Script.include("./modules/toolMenu.js");


    function log(message) {
        print(APP_NAME + ": " + message);
    }

    function debug(side, message) {
        // Optional parameter: side.
        var hand = "",
            HAND_LETTERS = ["L", "R"];
        if (DEBUG) {
            if (side === 0 || side === 1) {
                hand = HAND_LETTERS[side] + " ";
            } else {
                message = side;
            }
            log(hand + message);
        }
    }


    Inputs = function (side) {
        // A hand plus a laser.

        var
            // Primary objects.
            hand,
            laser,

            intersection = { x: "hello" };

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

        if (!this instanceof Inputs) {
            return new Inputs();
        }

        return {
            setUIEntities: setUIEntities,
            hand: getHand,
            laser: getLaser,
            getIntersection: getIntersection,
            update: update,
            clear: clear,
            destroy: destroy
        };
    };


    UI = function (side) {
        // Tool menu and Create palette.

        var // Primary objects.
            toolMenu,

            // References.
            leftInputs,
            rightInputs;

        toolMenu = new ToolMenu(side);


        function setReferences(left, right) {
            leftInputs = left;
            rightInputs = right;
        }

        function setHand(side) {
            toolMenu.setHand(side);
        }

        function display() {
            var uiEntityIDs;

            toolMenu.display();

            uiEntityIDs = toolMenu.getEntityIDs();
            leftInputs.setUIEntities(uiEntityIDs);
            rightInputs.setUIEntities(uiEntityIDs);
        }

        function update() {
            // TODO
        }

        function clear() {
            leftInputs.setUIEntities([]);
            rightInputs.setUIEntities([]);
            toolMenu.clear();
        }

        function destroy() {
            if (toolMenu) {
                toolMenu.destroy();
                toolMenu = null;
            }
        }

        if (!this instanceof UI) {
            return new UI();
        }

        return {
            setReferences: setReferences,
            setHand: setHand,
            display: display,
            update: update,
            clear: clear,
            destroy: destroy
        };
    };


    Editor = function (side) {
        // An entity selection, entity highlights, and entity handles.

        var otherEditor,  // Other hand's Editor object.

            // Editor states.
            EDITOR_IDLE = 0,
            EDITOR_SEARCHING = 1,
            EDITOR_HIGHLIGHTING = 2,  // Highlighting an entity (not hovering a handle).
            EDITOR_GRABBING = 3,
            EDITOR_DIRECT_SCALING = 4,  // Scaling data are sent to other editor's EDITOR_GRABBING state.
            EDITOR_HANDLE_SCALING = 5,  // ""
            EDITOR_STATE_STRINGS = ["EDITOR_IDLE", "EDITOR_SEARCHING", "EDITOR_HIGHLIGHTING", "EDITOR_GRABBING",
                "EDITOR_DIRECT_SCALING", "EDITOR_HANDLE_SCALING"],
            editorState = EDITOR_IDLE,

            // State machine.
            STATE_MACHINE,
            highlightedEntityID = null,  // Root entity of highlighted entity set.
            wasAppScaleWithHandles = false,
            isOtherEditorEditingEntityID = false,
            hoveredOverlayID = null,

            // Primary objects.
            selection,
            highlights,
            handles,

            // Input objects.
            hand,
            laser,

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

        selection = new Selection(side);
        highlights = new Highlights(side);
        handles = new Handles(side);

        function setReferences(inputs, editor) {
            hand = inputs.hand();  // Object.
            laser = inputs.laser();  // Object.
            getIntersection = inputs.getIntersection;  // Function.
            otherEditor = editor;  // Object.

            laserOffset = laser.handOffset();  // Value.
        }

        function hoverHandle(overlayID) {
            // Highlights handle if overlayID is a handle, otherwise unhighlights currently highlighted handle if any.
            handles.hover(overlayID);
        }

        function isHandle(overlayID) {
            return handles.isHandle(overlayID);
        }

        function isEditing(rootEntityID) {
            // rootEntityID is an optional parameter.
            return editorState > EDITOR_HIGHLIGHTING
                && (rootEntityID === undefined || rootEntityID === selection.rootEntityID());
        }

        function isScaling() {
            return editorState === EDITOR_DIRECT_SCALING || editorState === EDITOR_HANDLE_SCALING;
        }

        function rootEntityID() {
            return selection.rootEntityID();
        }


        function startEditing() {
            var selectionPositionAndOrientation;

            initialHandOrientationInverse = Quat.inverse(hand.orientation());
            selectionPositionAndOrientation = selection.getPositionAndOrientation();
            initialHandToSelectionVector = Vec3.subtract(selectionPositionAndOrientation.position, hand.position());
            initialSelectionOrientation = selectionPositionAndOrientation.orientation;

            selection.startEditing();
        }

        function stopEditing() {
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
            hoveredOverlayID = intersection.overlayID;
            otherEditor.hoverHandle(hoveredOverlayID);
        }

        function updateEditorSearching() {
            if (isAppScaleWithHandles && intersection.overlayID !== hoveredOverlayID && otherEditor.isEditing()) {
                hoveredOverlayID = intersection.overlayID;
                otherEditor.hoverHandle(hoveredOverlayID);
            }
        }

        function exitEditorSearching() {
            otherEditor.hoverHandle(null);
        }

        function enterEditorHighlighting() {
            selection.select(highlightedEntityID);
            if (!isAppScaleWithHandles || !otherEditor.isEditing(highlightedEntityID)) {
                highlights.display(intersection.handIntersected, selection.selection(),
                    isAppScaleWithHandles || otherEditor.isEditing(highlightedEntityID));
            }
            isOtherEditorEditingEntityID = otherEditor.isEditing(highlightedEntityID);
            wasAppScaleWithHandles = isAppScaleWithHandles;
        }

        function updateEditorHighlighting() {
            selection.select(highlightedEntityID);
            if (!isAppScaleWithHandles || !otherEditor.isEditing(highlightedEntityID)) {
                highlights.display(intersection.handIntersected, selection.selection(),
                    isAppScaleWithHandles || otherEditor.isEditing(highlightedEntityID));
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
            selection.select(highlightedEntityID);  // For when transitioning from EDITOR_SEARCHING.
            if (intersection.laserIntersected) {
                laser.setLength(laser.length());
            } else {
                laser.disable();
            }
            if (isAppScaleWithHandles) {
                handles.display(highlightedEntityID, selection.boundingBox(), selection.count() > 1);
            }
            startEditing();
            wasAppScaleWithHandles = isAppScaleWithHandles;
        }

        function updateEditorGrabbing() {
            selection.select(highlightedEntityID);
            if (isAppScaleWithHandles) {
                handles.display(highlightedEntityID, selection.boundingBox(), selection.count() > 1);
            } else {
                handles.clear();
            }
        }

        function exitEditorGrabbing() {
            stopEditing();
            handles.clear();
            laser.clearLength();
            laser.enable();
        }

        function enterEditorDirectScaling() {
            selection.select(highlightedEntityID);  // In case need to transition to EDITOR_GRABBING.
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
            selection.select(highlightedEntityID);  // In case need to transition to EDITOR_GRABBING.
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
            }
        };

        function setState(state) {
            if (state !== editorState) {
                STATE_MACHINE[EDITOR_STATE_STRINGS[editorState]].exit();
                STATE_MACHINE[EDITOR_STATE_STRINGS[state]].enter();
                editorState = state;
            } else if (DEBUG) {
                log("ERROR: Null state transition: " + state + "!");
            }
        }

        function updateState() {
            STATE_MACHINE[EDITOR_STATE_STRINGS[editorState]].update();
        }


        function update() {
            var previousState = editorState,
                doUpdateState;

            intersection = getIntersection();

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
                if (hand.valid() && (!intersection.entityID || !intersection.editableEntity)
                        && !(intersection.overlayID && hand.triggerClicked() && otherEditor.isHandle(intersection.overlayID))) {
                    // No transition.
                    updateState();
                    break;
                }
                if (!hand.valid()) {
                    setState(EDITOR_IDLE);
                } else if (intersection.overlayID && hand.triggerClicked()
                        && otherEditor.isHandle(intersection.overlayID)) {
                    highlightedEntityID = otherEditor.rootEntityID();
                    setState(EDITOR_HANDLE_SCALING);
                } else if (intersection.entityID && intersection.editableEntity && !hand.triggerClicked()) {
                    highlightedEntityID = Entities.rootOf(intersection.entityID);
                    setState(EDITOR_HIGHLIGHTING);
                } else if (intersection.entityID && intersection.editableEntity && hand.triggerClicked()) {
                    highlightedEntityID = Entities.rootOf(intersection.entityID);
                    if (otherEditor.isEditing(highlightedEntityID)) {
                        if (!isAppScaleWithHandles) {
                            setState(EDITOR_DIRECT_SCALING);
                        }
                    } else {
                        setState(EDITOR_GRABBING);
                    }
                } else {
                    debug(side, "ERROR: Unexpected condition in EDITOR_SEARCHING!");
                }
                break;
            case EDITOR_HIGHLIGHTING:
                if (hand.valid()
                        && intersection.entityID && intersection.editableEntity
                        && !(hand.triggerClicked() && (!otherEditor.isEditing(highlightedEntityID) || !isAppScaleWithHandles))
                        && !(hand.triggerClicked() && intersection.overlayID && otherEditor.isHandle(intersection.overlayID))) {
                    // No transition.
                    doUpdateState = false;
                    if (otherEditor.isEditing(highlightedEntityID) !== isOtherEditorEditingEntityID) {
                        doUpdateState = true;
                    }
                    if (Entities.rootOf(intersection.entityID) !== highlightedEntityID) {
                        highlightedEntityID = Entities.rootOf(intersection.entityID);
                        doUpdateState = true;
                    }
                    if (isAppScaleWithHandles !== wasAppScaleWithHandles) {
                        wasAppScaleWithHandles = isAppScaleWithHandles;
                        doUpdateState = true;
                    }
                    if (doUpdateState) {
                        updateState();
                    }
                    break;
                }
                if (!hand.valid()) {
                    setState(EDITOR_IDLE);
                } else if (intersection.overlayID && hand.triggerClicked()
                        && otherEditor.isHandle(intersection.overlayID)) {
                    highlightedEntityID = otherEditor.rootEntityID();
                    setState(EDITOR_HANDLE_SCALING);
                } else if (intersection.entityID && intersection.editableEntity && hand.triggerClicked()) {
                    highlightedEntityID = Entities.rootOf(intersection.entityID);  // May be a different entityID.
                    if (otherEditor.isEditing(highlightedEntityID)) {
                        if (!isAppScaleWithHandles) {
                            setState(EDITOR_DIRECT_SCALING);
                        } else {
                            debug(side, "ERROR: Unexpected condition in EDITOR_HIGHLIGHTING! A");
                        }
                    } else {
                        setState(EDITOR_GRABBING);
                    }
                } else if (!intersection.entityID || !intersection.editableEntity) {
                    setState(EDITOR_SEARCHING);
                } else {
                    debug(side, "ERROR: Unexpected condition in EDITOR_HIGHLIGHTING! B");
                }
                break;
            case EDITOR_GRABBING:
                if (hand.valid() && hand.triggerClicked() && !hand.gripPressed()) {
                    // Don't test for intersection.intersected because when scaling with handles intersection may lag behind.
                    // No transition.
                    if (isAppScaleWithHandles !== wasAppScaleWithHandles) {
                        updateState();
                        wasAppScaleWithHandles = isAppScaleWithHandles;
                    }
                    break;
                }
                if (!hand.valid()) {
                    setState(EDITOR_IDLE);
                } else if (!hand.triggerClicked()) {
                    if (intersection.entityID && intersection.editableEntity) {
                        highlightedEntityID = Entities.rootOf(intersection.entityID);
                        setState(EDITOR_HIGHLIGHTING);
                    } else {
                        setState(EDITOR_SEARCHING);
                    }
                } else if (hand.gripPressed()) {
                    selection.deleteEntities();
                    setState(EDITOR_SEARCHING);
                } else {
                    debug(side, "ERROR: Unexpected condition in EDITOR_GRABBING!");
                }
                break;
            case EDITOR_DIRECT_SCALING:
                if (hand.valid() && hand.triggerClicked()
                        && (otherEditor.isEditing(highlightedEntityID) || otherEditor.isHandle(intersection.overlayID))) {
                    // Don't test for intersection.intersected because when scaling with handles intersection may lag behind.
                    // Don't test isAppScaleWithHandles because this will eventually be a UI element and so not able to be 
                    // changed while scaling with two hands.
                    // No transition.
                    updateState();
                    break;
                }
                if (!hand.valid()) {
                    setState(EDITOR_IDLE);
                } else if (!hand.triggerClicked()) {
                    if (!intersection.entityID || !intersection.editableEntity) {
                        setState(EDITOR_SEARCHING);
                    } else {
                        highlightedEntityID = Entities.rootOf(intersection.entityID);
                        setState(EDITOR_HIGHLIGHTING);
                    }
                } else if (!otherEditor.isEditing(highlightedEntityID)) {
                    // Grab highlightEntityID that was scaling and has already been set.
                    setState(EDITOR_GRABBING);
                }
                break;
            case EDITOR_HANDLE_SCALING:
                if (hand.valid() && hand.triggerClicked() && otherEditor.isEditing(highlightedEntityID)) {
                    // Don't test for intersection.intersected because when scaling with handles intersection may lag behind.
                    // Don't test isAppScaleWithHandles because this will eventually be a UI element and so not able to be 
                    // changed while scaling with two hands.
                    // No transition.
                    updateState();
                    break;
                }
                if (!hand.valid()) {
                    setState(EDITOR_IDLE);
                } else if (!hand.triggerClicked()) {
                    if (!intersection.entityID || !intersection.editableEntity) {
                        setState(EDITOR_SEARCHING);
                    } else {
                        highlightedEntityID = Entities.rootOf(intersection.entityID);
                        setState(EDITOR_HIGHLIGHTING);
                    }
                } else if (!otherEditor.isEditing(highlightedEntityID)) {
                    // Grab highlightEntityID that was scaling and has already been set.
                    setState(EDITOR_GRABBING);
                }
                break;
            }

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

        if (!this instanceof Editor) {
            return new Editor();
        }

        return {
            setReferences: setReferences,
            hoverHandle: hoverHandle,
            isHandle: isHandle,
            isEditing: isEditing,
            isScaling: isScaling,
            rootEntityID: rootEntityID,
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

        updateTimer = Script.setTimeout(update, UPDATE_LOOP_TIMEOUT);
    }

    function updateHandControllerGrab() {
        // Communicate app status to handControllerGrab.js.
        Settings.setValue(VR_EDIT_SETTING, isAppActive);
    }

    function onAppButtonClicked() {
        // Application tablet/toolbar button clicked.
        isAppActive = !isAppActive;
        updateHandControllerGrab();
        button.editProperties({ isActive: isAppActive });

        if (isAppActive) {
            ui.display();
            update();
        } else {
            Script.clearTimeout(updateTimer);
            updateTimer = null;
            inputs[LEFT_HAND].clear();
            inputs[RIGHT_HAND].clear();
            ui.clear();
            editors[LEFT_HAND].clear();
            editors[RIGHT_HAND].clear();
        }
    }

    function otherHand(hand) {
        return (hand + 1) % 2;
    }

    function onDominantHandChanged() {
        /*
        // TODO: API coming.
        dominantHand = TODO;
        */
        ui.setHand(otherHand(dominantHand));
    }


    function setUp() {
        updateHandControllerGrab();

        tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        if (!tablet) {
            return;
        }

        // Settings values.
        // TODO: API coming.
        dominantHand = RIGHT_HAND;
        /*
        dominantHand = TODO;
        */

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
        ui = new UI(otherHand(dominantHand));
        ui.setReferences(inputs[LEFT_HAND], inputs[RIGHT_HAND]);

        // Editor objects.
        editors[LEFT_HAND] = new Editor(LEFT_HAND);
        editors[RIGHT_HAND] = new Editor(RIGHT_HAND);
        editors[LEFT_HAND].setReferences(inputs[LEFT_HAND], editors[RIGHT_HAND]);
        editors[RIGHT_HAND].setReferences(inputs[RIGHT_HAND], editors[LEFT_HAND]);

        // Settings changes.
        /*
        // TODO: API coming.
        TODO.change.connect(onDominantHandChanged);
        */

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
