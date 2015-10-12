//
//  whiteBoardEntityScript.js
//  examples/painting/whiteboard
//
//  Created by Eric Levin on 10/12/15.
//  Copyright 2015 High Fidelity, Inc.
//

//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, pointInExtents, vec3equal, setEntityCustomData, getEntityCustomData */

/*global Whiteboard */

(function() {

    var _this;
    Whiteboard = function() {
        _this = this;
    };

    Whiteboard.prototype = {

        setRightHand: function() {
            this.hand = 'RIGHT';
        },

        setLeftHand: function() {
            this.hand = 'LEFT';
        },

        startFarGrabNonColliding: function() {
            this.whichHand = this.hand;
        },

        continueFarGrabbingNonColliding: function() {},

        preload: function(entityID) {
            this.entityID = entityID;
            this.position = Entities.getEntityProperties(this.entityID, "position").position;
        },

    };

    // entity scripts always need to return a newly constructed object of our type
    return new Whiteboard();
});