//
//  detectGrabExample.js
//  examples/entityScripts
//
//  Created by Eric Levin on 9/21/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This is an example of an entity script which when assigned to an entity, will detect when the entity is being grabbed by the hydraGrab script
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {

    var _this;


    // this is the "constructor" for the entity as a JS object we don't do much here, but we do want to remember
    // our this object, so we can access it in cases where we're called without a this (like in the case of various global signals)
    LightSwitch = function() {
        _this = this;

        this.lightStateKey = "lightStateKey";
        this.resetKey = "resetMe";

    };

    LightSwitch.prototype = {


        startNearGrab: function() {
            print("TOGGLE LIGHT")
            this.lightState = getEntityCustomData(this.lightStateKey, this.entityID, defaultLightData);
            if (this.lightState.on === true) {
                //Delete the all the sconce lights
                var entities = Entities.findEntities(MyAvatar.position, 100);
                entities.forEach(function(entity){
                    var resetData = getEntityCustomData(this.resetKey, entity, {})
                });
            }
            // var position = Entities.getEntityProperties(this.entityID, "position").position;
            // Audio.playSound(clickSound, {
            //     position: position,
            //     volume: 0.05
            // });

        },

        createLights: function() {
            print("CREATE LIGHTS *******************")
            this.sconceLight1 = Entities.addEntity({
                type: "Light",
                position: {
                    x: 543.62,
                    y: 496.24,
                    z: 511.23
                },
                name: "Sconce 1 Light",
                dimensions: {
                    x: 2.545,
                    y: 2.545,
                    z: 2.545
                },
                cutoff: 90,
                color: {
                    red: 217,
                    green: 146,
                    blue: 24
                }
            });

            setEntityCustomData(this.resetKey, this.sconceLight1, {
                resetMe: true,
                lightType: "sconceLight"
            });
        },

        // clickReleaseOnEntity: function(entityId, mouseEvent) {
        //     print("CLIIICK ON MOUSE")
        //     if (!mouseEvent.isLeftButton) {
        //       return;
        //     }
        // },

        // preload() will be called when the entity has become visible (or known) to the interface
        // it gives us a chance to set our local JavaScript object up. In this case it means:
            preload: function(entityID) {
            this.entityID = entityID;
            var defaultLightData= {
                on: false
            };
            this.lightState = getEntityCustomData(this.lightStateKey, this.entityID, defaultLightData);

            //If light is off, then we create two new lights- at the position of the sconces
            if (this.lightState.on === false) {
                this.createLights();
            }

        },
    };

    // entity scripts always need to return a newly constructed object of our type
    return new LightSwitch();
})