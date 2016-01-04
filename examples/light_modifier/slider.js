//
//  slider.js
//
//  Created by James Pollack @imgntn on 12/15/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Entity script that sends a scaled value to a light based on its distance from the start of its constraint axis.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {

    var AXIS_SCALE = 1;
    var COLOR_MAX = 255;
    var INTENSITY_MAX = 0.05;
    var CUTOFF_MAX = 360;
    var EXPONENT_MAX = 1;

    function Slider() {
        return this;
    }

    Slider.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;
            var entityProperties = Entities.getEntityProperties(this.entityID, "userData");
            var parsedUserData = JSON.parse(entityProperties.userData);
            this.userData = parsedUserData.lightModifierKey;
        },
        startNearGrab: function() {
            this.setInitialProperties();
        },
        startDistantGrab: function() {
            this.setInitialProperties();
        },
        setInitialProperties: function() {
            this.initialProperties = Entities.getEntityProperties(this.entityID);
        },
        continueNearGrab: function() {
            //  this.continueDistantGrab();
        },
        continueDistantGrab: function() {
            this.setSliderValueBasedOnDistance();
        },
        setSliderValueBasedOnDistance: function() {
            var currentPosition = Entities.getEntityProperties(this.entityID, "position").position;

            var distance = Vec3.distance(this.userData.axisStart, currentPosition);

            if (this.userData.sliderType === 'color_red' || this.userData.sliderType === 'color_green' || this.userData.sliderType === 'color_blue') {
                this.sliderValue = this.scaleValueBasedOnDistanceFromStart(distance, 0, COLOR_MAX);
            }
            if (this.userData.sliderType === 'intensity') {
                this.sliderValue = this.scaleValueBasedOnDistanceFromStart(distance, 0, INTENSITY_MAX);
            }
            if (this.userData.sliderType === 'cutoff') {
                this.sliderValue = this.scaleValueBasedOnDistanceFromStart(distance, 0, CUTOFF_MAX);
            }
            if (this.userData.sliderType === 'exponent') {
                this.sliderValue = this.scaleValueBasedOnDistanceFromStart(distance, 0, EXPONENT_MAX);
            };

            this.sendValueToSlider();
        },
        releaseGrab: function() {
            Entities.editEntity(this.entityID, {
                velocity: {
                    x: 0,
                    y: 0,
                    z: 0
                },
                angularVelocity: {
                    x: 0,
                    y: 0,
                    z: 0
                }
            })

            this.sendValueToSlider();
        },
        scaleValueBasedOnDistanceFromStart: function(value, min2, max2) {
            var min1 = 0;
            var max1 = AXIS_SCALE;
            var min2 = min2;
            var max2 = max2;
            return min2 + (max2 - min2) * ((value - min1) / (max1 - min1));
        },
        sendValueToSlider: function() {
            var _t = this;
            var message = {
                lightID: _t.userData.lightID,
                sliderType: _t.userData.sliderType,
                sliderValue: _t.sliderValue
            }
            Messages.sendMessage('Hifi-Slider-Value-Reciever', JSON.stringify(message));
            if (_t.userData.sliderType === 'cutoff') {
                Messages.sendMessage('entityToolUpdates', 'callUpdate');
            }
        }
    };

    return new Slider();
});