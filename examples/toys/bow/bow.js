//
//  bow.js
//
//  This script attaches to a bow that you can pick up with a hand controller.  Use your other hand and press the trigger to grab the line, and release the trigger to fire.
//
//  Created by James B. Pollack @imgntn on 10/19/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {

    var ARROW_MODEL_URL = "https://hifi-public.s3.amazonaws.com/models/bow/arrow.fbx";
    var ARROW_COLLISION_HULL_URL = "https://hifi-public.s3.amazonaws.com/models/bow/arrow_collision_hull.obj";
    var ARROW_SCRIPT_URL = Script.resolvePath('arrow.js');
    var ARROW_OFFSET = 0.25;
    var ARROW_FORCE = 0.25;
    var ARROW_DIMENSIONS = {
        x: 0.08,
        y: 0.02,
        z: 0.08
    };
    var ARROW_GRAVITY = {
        x: 0,
        y: -9.8,
        z: 0
    };

    var TOP_NOTCH_OFFSET = 0.5;
    var BOTTOM_NOTCH_OFFSET = 0.5;

    var LINE_DIMENSIONS = {
        x: 5,
        y: 5,
        z: 5
    };

    var DRAW_STRING_THRESHOLD = 0.80;

    var _this;

    function Bow() {
        _this = this;
        return;
    }

    // create bow
    // on pickup, wait for other hand to start trigger pull
    // then create strings that start at top and bottom of bow
    // extend them to the other hand position
    // on trigger release, 
    // create arrow 
    // shoot arrow with velocity relative to distance between hand position and bow
    // delete lines

    Bow.prototype = {
        isGrabbed: false,
        stringDrawn: false,
        hasArrow: false,
        stringData: {
            currentColor: {
                red: 0,
                green: 255,
                blue: 0
            }

        },

        preload: function(entityID) {
            this.entityID = entityID;
        },

        setLeftHand: function() {
            if (this.isGrabbed === true) {
                return false;
            }
            this.hand = 'left';
        },

        setRightHand: function() {
            if (this.isGrabbed === true) {
                return false;
            }
            this.hand = 'right';
        },

        drawStrings: function() {

            this.updateStringPositions();
            var lineVectors = this.getLocalLineVectors();

            Entities.editEntity(this.topString, {
                linePoints: [{
                    x: 0,
                    y: 0,
                    z: 0
                }, lineVectors[0]],
                lineWidth: 5,
                color: this.stringData.currentColor
            });

            Entities.editEntity(this.bottomString, {
                linePoints: [{
                    x: 0,
                    y: 0,
                    z: 0
                }, lineVectors[1]],
                lineWidth: 5,
                color: this.stringData.currentColor
            });

        },

        createStrings: function() {
            this.createTopString();
            this.createBottomString();
        },

        deleteStrings: function() {
            Entities.deleteEntity(this.topString);
            Entities.deleteEntity(this.bottomString);
        },

        createTopString: function() {
            var stringProperties = {
                type: 'Line',
                position: Vec3.sum(this.bowProperties.position, TOP_NOTCH_OFFSET),
                dimensions: LINE_DIMENSIONS
            };

            this.topString = Entities.addEntity(stringProperties);
        },

        createBottomString: function() {
            var stringProperties = {
                type: 'Line',
                position: Vec3.sum(this.bowProperties.position, BOTTOM_NOTCH_OFFSET),
                dimensions: LINE_DIMENSIONS
            };

            this.bottomString = Entities.addEntity(stringProperties);
        },

        updateStringPositions: function() {

            var upVector = Quat.getFront(this.bowProperties.rotation);
            var upOffset = Vec3.multiply(upVector, TOP_NOTCH_OFFSET);
            var downVector = Vec3.multiply(-1, Quat.getFront(this.bowProperties.rotation));
            var downOffset = Vec3.multiply(downVector, BOTTOM_NOTCH_OFFSET);

            this.topStringPosition = Vec3.sum(this.bowProperties.position, upOffset);
            this.bottomStringPosition = Vec3.sum(this.bowProperties.position, downOffset);

            Entities.editEntity(this.topString, {
                position: this.topStringPosition
            });
            Entities.editEntity(this.bottomString, {
                position: this.bottomStringPosition
            });


        },

        startNearGrab: function() {
            if (this.isGrabbed === true) {
                return false;
            }
            this.isGrabbed = true;
            this.initialHand = this.hand;
        },

        continueNearGrab: function() {
            this.bowProperties = Entities.getEntityProperties(this.entityID, ["position", "rotation"]);
            this.checkStringHand();
        },

        releaseGrab: function() {
            if (this.isGrabbed === true && this.hand === this.initialHand) {
                this.isGrabbed = false;
                this.stringDrawn = false;
                this.deleteStrings();
                this.hasArrow = false;
            }
        },

        checkStringHand: function() {
            //invert the hands because our string will be held with the opposite hand of the first one we pick up the bow with
            if (this.initialHand === 'left') {
                this.getStringHandPosition = MyAvatar.getRightPalmPosition;
                this.getStringHandRotation = MyAvatar.getRightPalmRotation;
                this.stringTriggerAction = Controller.findAction("RIGHT_HAND_CLICK");
            } else if (this.initialHand === 'right') {
                this.getStringHandPosition = MyAvatar.getLeftPalmPosition;
                this.getStringHandRotation = MyAvatar.getLeftPalmRotation;
                this.stringTriggerAction = Controller.findAction("LEFT_HAND_CLICK");
            }

            this.triggerValue = Controller.getActionValue(this.stringTriggerAction);

            if (this.triggerValue < DRAW_STRING_THRESHOLD && this.stringDrawn === true) {
                print('TRIGGER 1')
                    //let it fly
                this.stringDrawn = false;
                this.deleteStrings();
                this.hasArrow = false;
                //this.releaseArrow();

            } else if (this.triggerValue >= DRAW_STRING_THRESHOLD && this.stringDrawn === true) {
                print('TRIGGER 2')
                    //update
                this.stringData.handPosition = this.getStringHandPosition();
                this.stringData.handRotation = this.getStringHandRotation();
                this.drawStrings();
                this.updateArrowPosition();

            } else if (this.triggerValue >= DRAW_STRING_THRESHOLD && this.stringDrawn === false) {
                print('TRIGGER 3')
                    //create
                this.stringDrawn = true;
                this.createStrings();
                this.stringData.handPosition = this.getStringHandPosition();
                this.stringData.handRotation = this.getStringHandRotation();
                if (this.hasArrow === false) {
                    this.createArrow();
                    this.hasArrow = true;
                }
                this.drawStrings();

            }

        },

        getArrowPosition: function() {
 
            var arrowVector = Vec3.subtract(this.bowProperties.position,this.stringData.handPosition);
            arrowVector = Vec3.normalize(arrowVector);
            arrowVector = Vec3.multiply(arrowVector,ARROW_OFFSET);
            var arrowPosition = Vec3.sum(this.stringData.handPosition,arrowVector);
            return arrowPosition;
        },

        updateArrowPosition: function() {
          var arrowPosition = this.getArrowPosition();
          print('ARROW POSITION:::' + JSON.stringify(arrowPosition));
            Entities.editEntity(this.arrow, {
                position: arrowPosition,
                rotation: this.bowProperties.rotation
            });
        },

        createArrow: function() {
            print('CREATING ARROW');
            var arrowProperties = {
                name: 'Hifi-Arrow',
                type: 'Model',
                modelURL: ARROW_MODEL_URL,
                dimensions: ARROW_DIMENSIONS,
                position: this.getArrowPosition(),
                rotation: this.bowProperties.rotation,
                collisionsWillMove: false,
                ignoreForCollisions: true,
                script: ARROW_SCRIPT_URL,
                lifetime: 40
            };

            this.arrow = Entities.addEntity(arrowProperties);
        },

        releaseArrow: function() {

            var forwardVec = Quat.getFront(Quat.multiply(this.bowProperties.rotation, Quat.fromPitchYawRollDegrees(0, 180, 0)));
            forwardVec = Vec3.normalize(forwardVec);
            var handDistanceAtRelease = Vec3.length(this.bowProperties.position, this.stringData.handPosition);

            forwardVec = Vec3.multiply(forwardVec, ARROW_FORCE);

            var arrowProperties = {
                velocity: forwardVec,
                collisionsWillMove: true,
                gravity: ARROW_GRAVITY,
                lifetime: 20
            };

            Entities.editEntity(this.arrow, arrowProperties);
        },
        getLocalLineVectors: function() {
            var topVector = Vec3.subtract(this.stringData.handPosition, this.topStringPosition);
            var bottomVector = Vec3.subtract(this.stringData.handPosition, this.bottomStringPosition);
            return [topVector, bottomVector];
        }
    };

    return new Bow();
});