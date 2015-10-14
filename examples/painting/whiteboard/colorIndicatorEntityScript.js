//
//  colorIndicatorEntityScript.js
//  examples/painting/whiteboard
//
//  Created by Eric Levin on 9/21/15.
//  Copyright 2015 High Fidelity, Inc.
//

//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */
/*global ColorIndicator */

(function() {

    var _this;
    ColorIndicator = function() {
        _this = this;
    };

    ColorIndicator.prototype = {

        changeColor: function() {
            var newColor = Entities.getEntityProperties(this.whiteboard, "userData").currentColor;
            Entities.editEntity(this.EntityID, {
                color: newColor
            });
        },

        preload: function(entityID) {
            this.entityID = entityID;
            var props = Entities.getEntityProperties(this.entityID, "userData");
            this.position = props.position;
            this.whiteboard = JSON.parse(props.userData).whiteboard;
        },

    };

    // entity scripts always need to return a newly constructed object of our type
    return new ColorIndicator();
});