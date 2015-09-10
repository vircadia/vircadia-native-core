//
//  flashligh.js
//  examples/entityScripts
//
//  Created by Sam Gateau on 9/9/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This is a toy script that can be added to the Flashlight model entity:
//  "https://hifi-public.s3.amazonaws.com/models/props/flashlight.fbx"
//  that creates a spotlight attached with the flashlight model while the entity is grabbed
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    
    function debugPrint(message) {
        //print(message);
    }

    Script.include("../../libraries/utils.js");

    var _this;

    // this is the "constructor" for the entity as a JS object we don't do much here, but we do want to remember
    // our this object, so we can access it in cases where we're called without a this (like in the case of various global signals)
    Flashlight = function() { 
        _this = this;
        _this._hasSpotlight = false;
        _this._spotlight = null;
    };


    GRAB_FRAME_USER_DATA_KEY = "grabFrame";

    // These constants define the Flashlight model Grab Frame 
    var MODEL_GRAB_FRAME = { 
        relativePosition: {x: 0, y: -0.1, z: 0},
        relativeRotation: Quat.angleAxis(180, {x: 1, y: 0, z: 0})
    };

    // These constants define the Spotlight position and orientation relative to the model 
    var MODEL_LIGHT_POSITION = {x: 0, y: 0, z: 0};
    var MODEL_LIGHT_ROTATION = Quat.angleAxis (-90, {x: 1, y: 0, z: 0});

    // Evaluate the world light entity position and orientation from the model ones
    function evalLightWorldTransform(modelPos, modelRot) {
        return {p: Vec3.sum(modelPos, Vec3.multiplyQbyV(modelRot, MODEL_LIGHT_POSITION)),
                q: Quat.multiply(modelRot, MODEL_LIGHT_ROTATION)};
    };

    Flashlight.prototype = {



        // update() will be called regulary, because we've hooked the update signal in our preload() function
        // we will check out userData for the grabData. In the case of the hydraGrab script, it will tell us
        // if we're currently being grabbed and if the person grabbing us is the current interfaces avatar.
        // we will watch this for state changes and print out if we're being grabbed or released when it changes.
        update: function() {
            var GRAB_USER_DATA_KEY = "grabKey";

            // because the update() signal doesn't have a valid this, we need to use our memorized _this to access our entityID
            var entityID = _this.entityID;

            // we want to assume that if there is no grab data, then we are not being grabbed
            var defaultGrabData = { activated: false, avatarId: null };

            // this handy function getEntityCustomData() is available in utils.js and it will return just the specific section
            // of user data we asked for. If it's not available it returns our default data.
            var grabData = getEntityCustomData(GRAB_USER_DATA_KEY, entityID, defaultGrabData);


            // if the grabData says we're being grabbed, and the owner ID is our session, then we are being grabbed by this interface
            if (grabData.activated && grabData.avatarId == MyAvatar.sessionUUID) {

                // remember we're being grabbed so we can detect being released
                _this.beingGrabbed = true;

                var modelProperties = Entities.getEntityProperties(entityID);
                var lightTransform = evalLightWorldTransform(modelProperties.position, modelProperties.rotation);

                // Create the spot light driven by this model if we don;t have one yet
                // Or make sure to keep it's position in sync
                if (!_this._hasSpotlight) {

                    _this._spotlight = Entities.addEntity({
                        type: "Light",
                        position: lightTransform.p,
                        rotation: lightTransform.q,
                        isSpotlight: true,
                        dimensions: { x: 2, y: 2, z: 20 },
                        color: { red: 255, green: 255, blue: 255 },
                        intensity: 2,
                        exponent: 0.3,
                        cutoff: 20
                    });
                    _this._hasSpotlight = true;

                    _this._startModelPosition = modelProperties.position;
                    _this._startModelRotation = modelProperties.rotation;

                    debugPrint("Flashlight:: creating a spotlight");
                } else {
                    // Updating the spotlight
                    Entities.editEntity(_this._spotlight, {position: lightTransform.p, rotation: lightTransform.q});

                    debugPrint("Flashlight:: updating the spotlight");
                }

                debugPrint("I'm being grabbed...");

            } else if (_this.beingGrabbed) {

                if (_this._hasSpotlight) {
                    Entities.deleteEntity(_this._spotlight);
                    debugPrint("Destroying flashlight spotlight...");
                }
                _this._hasSpotlight = false;
                _this._spotlight = null;

                // Reset model to initial position
                Entities.editEntity(_this.entityID, {position: _this._startModelPosition, rotation: _this._startModelRotation,
                                                     velocity: {x: 0, y: 0, z: 0}, angularVelocity: {x: 0, y: 0, z: 0}});

                // if we are not being grabbed, and we previously were, then we were just released, remember that
                // and print out a message
                _this.beingGrabbed = false;
                debugPrint("I'm was released...");
            }
        },

        // preload() will be called when the entity has become visible (or known) to the interface
        // it gives us a chance to set our local JavaScript object up. In this case it means:
        //   * remembering our entityID, so we can access it in cases where we're called without an entityID
        //   * connecting to the update signal so we can check our grabbed state
        preload: function(entityID) {
            _this.entityID = entityID;
        
            var modelProperties = Entities.getEntityProperties(entityID);
            _this._startModelPosition = modelProperties.position;
            _this._startModelRotation = modelProperties.rotation;

            // Make sure the Flashlight entity has a correct grab frame setup
            var userData = getEntityUserData(entityID);
            debugPrint(JSON.stringify(userData));
            if (!userData.grabFrame) {
                setEntityCustomData(GRAB_FRAME_USER_DATA_KEY, entityID, MODEL_GRAB_FRAME);
                debugPrint(JSON.stringify(MODEL_GRAB_FRAME));
                debugPrint("Assigned the grab frmae for the Flashlight entity");
            }

            Script.update.connect(this.update);        
        },

        // unload() will be called when our entity is no longer available. It may be because we were deleted,
        // or because we've left the domain or quit the application. In all cases we want to unhook our connection
        // to the update signal
        unload: function(entityID) {

            if (_this._hasSpotlight) {
                Entities.deleteEntity(_this._spotlight);
            }
            _this._hasSpotlight = false;
            _this._spotlight = null;

            Script.update.disconnect(this.update);
        },
    };

    // entity scripts always need to return a newly constructed object of our type
    return new Flashlight();
})
