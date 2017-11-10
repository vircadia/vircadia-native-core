"use strict";

//  pointerUtils.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* jslint bitwise: true */

/* global Script, Entities, Overlays, Controller, Vec3, Quat, getControllerWorldLocation, RayPick,
   controllerDispatcherPlugins:true, controllerDispatcherPluginsNeedSort:true,
   LEFT_HAND, RIGHT_HAND, NEAR_GRAB_PICK_RADIUS, DEFAULT_SEARCH_SPHERE_DISTANCE, DISPATCHER_PROPERTIES,
   getGrabPointSphereOffset, HMD, MyAvatar, Messages, findHandChildEntities, Pointers, PickType, COLORS_GRAB_SEARCHING_HALF_SQUEEZE
   COLORS_GRAB_SEARCHING_FULL_SQUEEZE, COLORS_GRAB_DISTANCE_HOLD, Picks, TRIGGER_ON_VALUE
*/


Script.include("/~/system/libraries/controllerDispatcherUtils.js");
function Pointer(hudLayer, pickType, pointerData) {
    this.SEARCH_SPHERE_SIZE = 0.0132;
    this.dim = {x: this.SEARCH_SPHERE_SIZE, y: this.SEARCH_SPHERE_SIZE, z: this.SEARCH_SPHERE_SIZE};
    this.halfPath = {
        type: "line3d",
        color: COLORS_GRAB_SEARCHING_HALF_SQUEEZE,
        visible: true,
        alpha: 1,
        solid: true,
        glow: 1.0,
        lineWidth: 5,
        ignoreRayIntersection: true, // always ignore this
        drawInFront: true, // Even when burried inside of something, show it.
        drawHUDLayer: hudLayer,
        parentID: MyAvatar.SELF_ID
    };
    this.halfEnd = {
        type: "sphere",
        dimensions: this.dim,
        solid: true,
        color: COLORS_GRAB_SEARCHING_HALF_SQUEEZE,
        alpha: 0.9,
        ignoreRayIntersection: true,
        drawInFront: true, // Even when burried inside of something, show it.
        drawHUDLayer: hudLayer,
        visible: true
    };
    this.fullPath = {
        type: "line3d",
        color: COLORS_GRAB_SEARCHING_FULL_SQUEEZE,
        visible: true,
        alpha: 1,
        solid: true,
        glow: 1.0,
        lineWidth: 5,
        ignoreRayIntersection: true, // always ignore this
        drawInFront: true, // Even when burried inside of something, show it.
        drawHUDLayer: hudLayer,
        parentID: MyAvatar.SELF_ID
    };
    this.fullEnd = {
        type: "sphere",
        dimensions: this.dim,
        solid: true,
        color: COLORS_GRAB_SEARCHING_FULL_SQUEEZE,
        alpha: 0.9,
        ignoreRayIntersection: true,
        drawInFront: true, // Even when burried inside of something, show it.
        drawHUDLayer: hudLayer,
        visible: true
    };
    this.holdPath = {
        type: "line3d",
        color: COLORS_GRAB_DISTANCE_HOLD,
        visible: true,
        alpha: 1,
        solid: true,
        glow: 1.0,
        lineWidth: 5,
        ignoreRayIntersection: true, // always ignore this
        drawInFront: true, // Even when burried inside of something, show it.
        drawHUDLayer: hudLayer,
        parentID: MyAvatar.SELF_ID
    };

    this.renderStates = [
        {name: "half", path: this.halfPath, end: this.halfEnd},
        {name: "full", path: this.fullPath, end: this.fullEnd},
        {name: "hold", path: this.holdPath}
    ];

    this.defaultRenderStates = [
        {name: "half", distance: DEFAULT_SEARCH_SPHERE_DISTANCE, path: this.halfPath},
        {name: "full", distance: DEFAULT_SEARCH_SPHERE_DISTANCE, path: this.fullPath},
        {name: "hold", distance: DEFAULT_SEARCH_SPHERE_DISTANCE, path: this.holdPath}
    ];


    this.pointerID = null;
    this.triggers = [];
    this.joint = null;
    this.visible = false;
    this.locked = false;

    function createPointer(pickType, pointerData) {
        if (this.pointerID) {
            Pointers.removePointer(this.pointerID);
        }
        this.pointerID = Pointers.createPointer(pickType, pointerData);
        Pointers.setRenderState(this.pointerID, "");
        Pointers.enablePointer(this.pointerID);
    }

    this.enable = function() {
    };

    this.disable = function() {
    };

    this.makeVisible = function() {
        this.visible = true;
    };

    this.makeInvisible = function() {
        this.visible = false;
    };

    this.lockEnd = function(lockData) {
        if (lockData !== undefined) {
            var hand = lockData.hand;
            if (this.visible) {
                var targetID = lockData.targetID;
                var targetIsOverlay = lockData.isOverlay;
                Pointers.setLockEndUUID(this.pointer, targetID, targetIsOverlay);
                this.locked = targetID;
            }
        } else {
            this.locked = false;
        }
    };

    this.updateRenderState = function(triggerClicked, triggerValue) {
        var mode = "";
        if (this.visible) {
            if (this.locked) {
                mode = "hold";
            } else if (triggerClicked) {
                mode = "full";
            } else if (triggerValue > TRIGGER_ON_VALUE) {
                mode = "half";
            }
        }

        Pointers.setRenderState(this.pointerID, mode);
    };
    pointerData.renderStates = this.renderStates;
    pointerData.defualtRenderStates = this.defualtRenderStates;
    createPointer(pickType, pointerData);
}


function PointerManager() {
    this.pointers = [];

    this.createPointer = function(hudLayer, pickType, pointerData) {
        var pointer = new Pointer(hudLayer, pickType, pointerData);
        this.pointers.push(pointer);
    };

    this.makePointerVisible = function(index) {
        if (index < this.pointers.length) {
            this.pointers[index].makeVisible();
        }
    };

    this.makePointerInvisible = function(index) {
        if (index < this.pointers.length) {
            this.pointers[index].makeInvisible();
        }
    };

    this.lockPointerEnd = function(index, lockData) {
        if (index < this.pointers.length) {
            this.pointers[index].lockEnd(lockData);
        }
    };

    this.updatePointersRenderState = function(triggerClicked, triggerValue) {
        for (var index = 0; index < this.pointers.length; index++) {
            this.pointers[index].updateRenderState(triggerClicked, triggerValue);
        }
    };
}
