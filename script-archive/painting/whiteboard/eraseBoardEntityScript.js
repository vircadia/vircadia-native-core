//
//  eraseBoardEntityScript.js
//  examples/painting/whiteboard
//
//  Created by Eric Levin on 9/21/15.
//  Copyright 2015 High Fidelity, Inc.
//

//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */
/*global BoardEraser */

(function() {

    var _this;
    BoardEraser = function() {
        _this = this;
    };

    BoardEraser.prototype = {

        startFarTrigger: function() {
            this.eraseBoard();
        },

        clickReleaseOnEntity: function() {
            this.eraseBoard();
        },

        eraseBoard: function() {
            Entities.callEntityMethod(this.whiteboard, "eraseBoard");
        },

        preload: function(entityID) {
            this.entityID = entityID;
            var props = Entities.getEntityProperties(this.entityID, ["userData"]);
            this.whiteboard = JSON.parse(props.userData).whiteboard;
        },

    };

    // entity scripts always need to return a newly constructed object of our type
    return new BoardEraser();
});
