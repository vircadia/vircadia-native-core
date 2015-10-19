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

        var STRING_OFFSET = {
            x: 0,
            y: 0,
            z: 0
        };
        var

        var _this;


        function Recorder() {
            _this = this;
            return;
        }

        Bow.prototype = {
            isGrabbed: false,
            init: function() {


            },
            preload: function(entityID) {
                this.entityID = entityID;
            }
            keepStringWithBow: function(bowLocation) {
                var offset = {

                }
            },

            setLeftHand: function() {
                this.hand = 'left';
            },

            setRightHand: function() {
                this.hand = 'right';
            },

            startNearGrab: function() {

                var grabLocation = Entites.getEntityProperties(this.entityID,"position");
                this.isGrabbed = true;
                this.initialHand = this.hand;
            },

            continueNearGrab: function() {

            },

            releaseGrab: function() {
                if (this.isGrabbed === true && this.hand === this.initialHand) {

                    this.isGrabbed = false;
                }
            },

            return new Bow;
        })

    function deleteInterval() {
        Script.clearInterval(recorderInterval);
        Entities.deletingEntity.disconnect(deleteInterval);
    }
    Entities.deletingEntity.connect(deleteInterval);