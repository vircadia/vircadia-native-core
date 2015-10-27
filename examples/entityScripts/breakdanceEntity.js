//
//  breakdanceEntity.js
//  examples/entityScripts
//
//  Created by Brad Hefta-Gaub on 9/3/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This is an example of an entity script which when assigned to an entity, will start the breakdance game if you grab and hold the entity
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    Script.include("../breakdanceCore.js");
    Script.include("../libraries/utils.js");

    var _this;

    // this is the "constructor" for the entity as a JS object we don't do much here, but we do want to remember
    // our this object, so we can access it in cases where we're called without a this (like in the case of various global signals)
    BreakdanceEntity = function() {
        _this = this;
        print("BreakdanceEntity constructor");
    };

    BreakdanceEntity.prototype = {

        // update() will be called regulary, because we've hooked the update signal in our preload() function
        // we will check out userData for the grabData. In the case of the hydraGrab script, it will tell us
        // if we're currently being grabbed and if the person grabbing us is the current interfaces avatar.
        // we will watch this for state changes and print out if we're being grabbed or released when it changes.
        update: function() {
            //print("BreakdanceEntity.update() _this.entityID:" + _this.entityID);
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
                //print("BreakdanceEntity.update() [I'm being grabbed] _this.entityID:" + _this.entityID);
                if (!_this.beingGrabbed) {
                    print("I'm was grabbed... _this.entityID:" + _this.entityID);

                    // remember we're being grabbed so we can detect being released
                    _this.beingGrabbed = true;
                    var props = Entities.getEntityProperties(entityID);
                    var puppetPosition = getPuppetPosition(props);
                    breakdanceStart(props.modelURL, puppetPosition);
                } else {
                    breakdanceUpdate();
                }

            } else if (_this.beingGrabbed) {

                // if we are not being grabbed, and we previously were, then we were just released, remember that
                // and print out a message
                _this.beingGrabbed = false;
                print("I'm was released... _this.entityID:" + _this.entityID);
                breakdanceEnd();
            }
        },

        // preload() will be called when the entity has become visible (or known) to the interface
        // it gives us a chance to set our local JavaScript object up. In this case it means:
        //   * remembering our entityID, so we can access it in cases where we're called without an entityID
        //   * connecting to the update signal so we can check our grabbed state
        preload: function(entityID) {
            this.entityID = entityID;
            print("BreakdanceEntity.preload() this.entityID:" + this.entityID);
            Script.update.connect(this.update);
        },

        // unload() will be called when our entity is no longer available. It may be because we were deleted,
        // or because we've left the domain or quit the application. In all cases we want to unhook our connection
        // to the update signal
        unload: function(entityID) {
            print("BreakdanceEntity.unload() this.entityID:" + this.entityID);
            Script.update.disconnect(this.update);
        },

    };

    // entity scripts always need to return a newly constructed object of our type
    return new BreakdanceEntity();
})
