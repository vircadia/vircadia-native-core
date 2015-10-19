//
//  bow.js
//
//  This script creates a bow that you can pick up with a hand controller.  Use your other hand and press the trigger to grab the line, and release the trigger to fire.
//  Created by James B. Pollack @imgntn on 10/10/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
        var ARROW_MODEL_URL = '';
        var TOP_NOTCH_OFFSET = {
            x: 0,
            y: 0,
            z: 0
        };

        var BOTTOM_NOTCH_OFFSET = {
            x: 0,
            y: 0,
            z: 0
        };

        var DRAW_STRING_THRESHOLD = 0.1;

        var _this;

        function Bow() {
            _this = this;
            return;
        }

        Bow.prototype = {
            isGrabbed: false,
            stringDrawn: false,
            stringData: {
                top: {},
                bottom: {}
            },

            preload: function(entityID) {
                this.entityID = entityID;
            },

            setLeftHand: function() {
                this.hand = 'left';
            },

            setRightHand: function() {
                this.hand = 'right';
            },

            drawStrings: function() {

                Entities.editEntity(this.topString, {
                    linePoints: this.stringData.top.points,
                    normals: this.stringData.top.normals,
                    strokeWidths: this.stringData.top.strokeWidths,
                    color: stringData.currentColor
                });

                Entities.editEntity(this.bottomString, {
                    linePoints: this.stringData.bottom.points,
                    normals: this.stringData.bottom.normals,
                    strokeWidths: this.stringData.bottom.strokeWidths,
                    color: stringData.currentColor
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
                    type: 'Polyline'
                    position: Vec3.sum(this.position, this.TOP_NOTCH_OFFSET);
                };

                this.topString = Entities.addEntity(stringProperties);
            },

            createBottomString: function() {

                var stringProperties = {
                    type: 'Polyline'
                    position: Vec3.sum(this.position, this.TOP_NOTCH_OFFSET);
                };

                this.bottomString = Entities.addEntity(stringProperties);
            },

            startNearGrab: function() {
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
                }
            },

            checkStringHand: function() {
                //invert the hands because our string will be held with the opposite hand of the first one we pick up the bow with
                if (this.hand === 'left') {
                    this.getStringHandPosition = MyAvatar.getRightPalmPosition;
                    this.getStringHandRotation = MyAvatar.getRightPalmRotation;
                    this.stringTriggerAction = Controller.findAction("RIGHT_HAND_CLICK");
                } else if (this.hand === 'right') {
                    this.getStringHandPosition = MyAvatar.getLeftPalmPosition;
                    this.getStringHandRotation = MyAvatar.getLeftPalmRotation;
                    this.stringTriggerAction = Controller.findAction("LEFT_HAND_CLICK");
                }

                this.triggerValue = Controller.getActionValue(handClick);

                if (this.triggerValue < DRAW_STRING_THRESHOLD && this.stringDrawn === true) {

                    this.releaseArrow();
                    this.deleteStrings();
                } else if (this.triggerValue >= DRAW_STRING_THRESHOLD && this.stringDrawn === false) {
                    this.stringData.handPosition = this.getStringHandPosition();
                    this.stringData.handRotation = this.getStringHandRotation();
                    this.drawStrings();
                }


            },
            releaseArrow: function() {
                var handDistanceAtRelease = Vec3.length(this.bowProperties.position, this.stringData.handPosition);
                var releaseDirection = this.stringData.handRotation;
                var releaseVecotr = 'something';
                var arrowProperties = {
                    type: 'Model',
                    dimensions: ARROW_DIMENSIONS,
                    position: Vec3.sum(this.bowProperties.position, ARROW_OFFSET),
                    velocity: relesaeVector,
                };
                this.arrow = Entities.addEntity(arrowProperties);
            }

                return new Bow;
        })