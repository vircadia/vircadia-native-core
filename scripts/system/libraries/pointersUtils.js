"use strict";

//  pointerUtils.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* jslint bitwise: true */

/* global Script, Pointers,
   DEFAULT_SEARCH_SPHERE_DISTANCE, COLORS_GRAB_SEARCHING_HALF_SQUEEZE, COLORS_GRAB_SEARCHING_FULL_SQUEEZE,
   COLORS_GRAB_DISTANCE_HOLD, TRIGGER_ON_VALUE,
   Pointer:true, PointerManager:true
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Pointer = function(hudLayer, pickType, pointerData) {
    this.SEARCH_SPHERE_SIZE = 0.0132;
    this.dim = {x: this.SEARCH_SPHERE_SIZE, y: this.SEARCH_SPHERE_SIZE, z: this.SEARCH_SPHERE_SIZE};
    this.halfPath = {
        type: "line3d",
        color: COLORS_GRAB_SEARCHING_HALF_SQUEEZE,
        visible: true,
        alpha: 1,
        solid: true,
        glow: 1.0,
        ignoreRayIntersection: true, // always ignore this
        drawInFront: !hudLayer, // Even when burried inside of something, show it.
        drawHUDLayer: hudLayer,
    };
    this.halfEnd = {
        type: "sphere",
        dimensions: this.dim,
        solid: true,
        color: COLORS_GRAB_SEARCHING_HALF_SQUEEZE,
        alpha: 0.9,
        ignoreRayIntersection: true,
        drawInFront: !hudLayer, // Even when burried inside of something, show it.
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
        ignoreRayIntersection: true, // always ignore this
        drawInFront: !hudLayer, // Even when burried inside of something, show it.
        drawHUDLayer: hudLayer,
    };
    this.fullEnd = {
        type: "sphere",
        dimensions: this.dim,
        solid: true,
        color: COLORS_GRAB_SEARCHING_FULL_SQUEEZE,
        alpha: 0.9,
        ignoreRayIntersection: true,
        drawInFront: !hudLayer, // Even when burried inside of something, show it.
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
        ignoreRayIntersection: true, // always ignore this
        drawInFront: !hudLayer, // Even when burried inside of something, show it.
        drawHUDLayer: hudLayer,
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
    this.visible = false;
    this.locked = false;
    this.allwaysOn = false;
    this.hand = pointerData.hand;
    delete pointerData.hand;

    function createPointer(pickType, pointerData) {
        var pointerID = Pointers.createPointer(pickType, pointerData);
        Pointers.setRenderState(pointerID, "");
        Pointers.enablePointer(pointerID);
        return pointerID;
    }

    this.enable = function() {
        Pointers.enablePointer(this.pointerID);
    };

    this.disable = function() {
        Pointers.disablePointer(this.pointerID);
    };

    this.removePointer = function() {
        Pointers.removePointer(this.pointerID);
    };

    this.makeVisible = function() {
        this.visible = true;
    };

    this.makeInvisible = function() {
        this.visible = false;
    };

    this.lockEnd = function(lockData) {
        if (lockData !== undefined) {
            if (this.visible && !this.locked && lockData.targetID !== null) {
                var targetID = lockData.targetID;
                var targetIsOverlay = lockData.isOverlay;
                if (lockData.offset === undefined) {
                    Pointers.setLockEndUUID(this.pointerID, targetID, targetIsOverlay);
                } else {
                    Pointers.setLockEndUUID(this.pointerID, targetID, targetIsOverlay, lockData.offset);
                }
                this.locked = targetID;
            }
        } else if (this.locked) {
            Pointers.setLockEndUUID(this.pointerID, null, false);
            this.locked = false;
        }
    };

    this.updateRenderState = function(triggerClicks, triggerValues) {
        var mode = "";
        if (this.visible) {
            if (this.locked) {
                mode = "hold";
            } else if (triggerClicks[this.hand]) {
                mode = "full";
            } else if (triggerValues[this.hand] > TRIGGER_ON_VALUE || this.allwaysOn) {
                mode = "half";
            }
        }

        Pointers.setRenderState(this.pointerID, mode);
    };
    pointerData.renderStates = this.renderStates;
    pointerData.defaultRenderStates = this.defaultRenderStates;
    this.pointerID = createPointer(pickType, pointerData);
};


PointerManager = function() {
    this.pointers = [];

    this.createPointer = function(hudLayer, pickType, pointerData) {
        var pointer = new Pointer(hudLayer, pickType, pointerData);
        this.pointers.push(pointer);
        return pointer.pointerID;
    };

    this.makePointerVisible = function(laserParams) {
        var index = laserParams.hand;
        if (index < this.pointers.length && index >= 0) {
            this.pointers[index].makeVisible();
            this.pointers[index].allwaysOn = laserParams.allwaysOn;
        }
    };

    this.makePointerInvisible = function(laserParams) {
        var index = laserParams.hand;
        if (index < this.pointers.length && index >= 0) {
            this.pointers[index].makeInvisible();
        }
    };

    this.lockPointerEnd = function(laserParams, lockData) {
        var index = laserParams.hand;
        if (index < this.pointers.length && index >= 0) {
            this.pointers[index].lockEnd(lockData);
        }
    };

    this.updatePointersRenderState = function(triggerClicks, triggerValues) {
        for (var index = 0; index < this.pointers.length; index++) {
            this.pointers[index].updateRenderState(triggerClicks, triggerValues);
        }
    };

    this.removePointers = function() {
        for (var index = 0; index < this.pointers.length; index++) {
            this.pointers[index].removePointer();
        }
        this.pointers = [];
    };
};
