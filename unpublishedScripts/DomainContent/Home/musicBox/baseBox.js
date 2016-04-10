//
//
//  Created by The Content Team 4/10/216
//  Copyright 2016 High Fidelity, Inc.
//
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//

(function() {

    var _this;

    function BaseBox() {
        _this = this;
        return this;
    }

    BaseBox.prototype = {
        lid:null,
        startNearGrab: function(myID, params) {
            print('START GRAB ON BASE')
            _this.hand = params[0];
            _this.disableHand();
            var myProps = Entities.getEntityProperties(_this.entityID)
            var results = Entities.findEntities(myProps.position, 2);
            var lid = null;
            results.forEach(function(result) {
                var resultProps = Entities.getEntityProperties(result);
                if (resultProps.name.indexOf('music_box_lid') > -1) {
                    _this.lid = result;
                }
            });
        },

        preload: function(entityID) {
            this.entityID = entityID;
            print('PRELOAD BASE!!')
        },

        continueNearGrab: function() {
            print('GRAB ON DE BASE!')
            var myProps = Entities.getEntityProperties(_this.entityID)
            var results = Entities.findEntities(myProps.position, 2);
     
            results.forEach(function(result) {
                var resultProps = Entities.getEntityProperties(result);
                if (resultProps.name.indexOf('music_box_lid') > -1) {
                    _this.lid = result;
                }
            });
            if (_this.lid === null) {
                return;
            } else {
                Entities.callEntityMethod(_this.lid, 'updateHatRotation');
                Entities.callEntityMethod(_this.lid, 'updateKeyRotation');
            }
        },

        disableHand: function() {
            print('DISABLE HAND BASE')
            //disable the off hand in the grab script.
            var handToDisable = _this.hand === 'right' ? 'left' : 'right';
            print("disabling hand: " + handToDisable);
            Messages.sendLocalMessage('Hifi-Hand-Disabler', handToDisable);

            //tell the lid which hand it should use.
            //the lid should use the hand that is disabled.
            if (handToDisable === 'right') {
                Entities.callEntityMethod(_this.lid, 'setRightHand');
            }
            if (handToDisable === 'left') {
                Entities.callEntityMethod(_this.lid, 'setLeftHand');
            }

            print('DISABLED BASE HAND:: ' + handToDisable)
        },

        enableHand: function() {
            print('SHOULD ENABLE HAND FROM BASE')
            Messages.sendLocalMessage('Hifi-Hand-Disabler', "none")
            Entities.callEntityMethod(_this.lid, 'clearHand');
 
        },

        releaseGrab: function() {
            print('RELEASE GRAB ON BASE')
            _this.enableHand();
        },

        unload: function() {
            _this.enableHand();
        }
    }

    return new BaseBox
})