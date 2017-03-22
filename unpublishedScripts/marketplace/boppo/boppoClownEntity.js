//
//  boppoClownEntity.js
//
//  Created by Thijs Wenker on 3/15/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* globals LookAtTarget */

(function() {
    var SFX_PREFIX = 'https://hifi-content.s3-us-west-1.amazonaws.com/caitlyn/production/elBoppo/sfx/';
    var CHANNEL_PREFIX = 'io.highfidelity.boppo_server_';
    var PUNCH_SOUNDS = [
        'punch_1.wav',
        'punch_2.wav'
    ];
    var PUNCH_COOLDOWN = 300;

    Script.include('lookAtEntity.js');

    var createBoppoClownEntity = function() {
        var _this,
            _entityID,
            _boppoUserData,
            _lookAtTarget,
            _punchSounds = [],
            _lastPlayedPunch = {};

        var getOwnBoppoUserData = function() {
            try {
                return JSON.parse(Entities.getEntityProperties(_entityID, ['userData']).userData).Boppo;
            } catch (e) {
                // e
            }
            return {};
        };

        var BoppoClownEntity = function () {
            _this = this;
            PUNCH_SOUNDS.forEach(function(punch) {
                _punchSounds.push(SoundCache.getSound(SFX_PREFIX + punch));
            });
        };

        BoppoClownEntity.prototype = {
            preload: function(entityID) {
                _entityID = entityID;
                _boppoUserData = getOwnBoppoUserData();
                _lookAtTarget = new LookAtTarget(_entityID);
            },
            collisionWithEntity: function(boppoEntity, collidingEntity, collisionInfo) {
                if (collisionInfo.type === 0 &&
                    Entities.getEntityProperties(collidingEntity, ['name']).name.indexOf('Boxing Glove ') === 0) {

                    if (_lastPlayedPunch[collidingEntity] === undefined ||
                        Date.now() - _lastPlayedPunch[collidingEntity] > PUNCH_COOLDOWN) {

                        // If boxing glove detected here:
                        Messages.sendMessage(CHANNEL_PREFIX + _boppoUserData.gameParentID, 'hit');

                        _lookAtTarget.lookAtByAction();
                        var randomPunchIndex = Math.floor(Math.random() * _punchSounds.length);
                        Audio.playSound(_punchSounds[randomPunchIndex], {
                            position: collisionInfo.contactPoint
                        });
                        _lastPlayedPunch[collidingEntity] = Date.now();
                    }
                }
            }

        };

        return new BoppoClownEntity();
    };

    return createBoppoClownEntity();
});
