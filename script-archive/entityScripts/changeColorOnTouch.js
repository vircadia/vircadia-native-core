//
//  changeColorOnTouch.js
//  examples/entityScripts
//
//  Created by Brad Hefta-Gaub on 11/1/14.
//  Additions by James B. Pollack @imgntn on 9/23/2015
//  Copyright 2014 High Fidelity, Inc.
//
//  ATTENTION:  Requires you to run handControllerGrab.js
//  This is an example of an entity script which when assigned to a non-model entity like a box or sphere, will
//  change the color of the entity when you touch it. 
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt, pointInExtents, vec3equal, setEntityCustomData, getEntityCustomData */


(function() {

  function ChangeColorOnTouch () {
        this.oldColor = {};
        this.oldColorKnown = false;
    }

    ChangeColorOnTouch.prototype = {

        storeOldColor: function(entityID) {
            var oldProperties = Entities.getEntityProperties(entityID);
            this.oldColor = oldProperties.color;
            this.oldColorKnown = true;

            print("storing old color... this.oldColor=" + this.oldColor.red + "," + this.oldColor.green + "," + this.oldColor.blue);
        },

        preload: function(entityID) {
            print("preload");

            this.entityID = entityID;
            this.storeOldColor(entityID);
        },

        startTouch: function() {
            print("startTouch");

            if (!this.oldColorKnown) {
                this.storeOldColor(this.entityID);
            }

            Entities.editEntity(this.entityID, {color: { red: 0, green: 255, blue: 255 }});
        },

        continueTouch: function() {
            //unused here
            return;
        },

        stopTouch: function() {
            print("stopTouch");
            
            if (this.oldColorKnown) {
                print("leave restoring old color... this.oldColor=" + this.oldColor.red + "," + this.oldColor.green + "," + this.oldColor.blue);
                Entities.editEntity(this.entityID, {color: this.oldColor});
            }
        }


    };

    return new ChangeColorOnTouch();
});