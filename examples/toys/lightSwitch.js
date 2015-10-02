//
//  lightSwitch.js.js
//  examples/entityScripts
//
//  Created by Eric Levin on 10/2/15.
//  Copyright 2015 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, pointInExtents, vec3equal, setEntityCustomData, getEntityCustomData */
//per script

/*global  LightSwitch */

(function () {

    var LightSwitch = function () {
        this.switchSound = SoundCache.getSound("https://hifi-public.s3.amazonaws.com/sounds/Switches%20and%20sliders/lamp_switch_2.wav");
        print("SHNUUUUR")

    };

    LightSwitch.prototype = {

        clickReleaseOnEntity: function (entityID, mouseEvent) {
            if (!mouseEvent.isLeftButton) {
                return;
            }
            this.toggleLights();
        },

        toggleLights: function () {
            print("TOGGLE LIGHTS");
        },

        startNearGrabNonColliding: function () {
            this.toggleLights();
        },

        preload: function (entityID) {
            this.entityID = entityID;
            //The light switch is static, so just cache its position once
            this.position = Entities.getEntityProperties(this.entityID, "position").position;
        },
    };

    // entity scripts always need to return a newly constructed object of our type
    return new LightSwitch();
}());