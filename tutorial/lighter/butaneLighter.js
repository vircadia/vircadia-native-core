//
//  Created by Thijs Wenker on September 14, 2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var _this;

    function getResourceURL(file) {
        return 'atp:/' + file;
    };

    const LIGHTER_ON_SOUND_URL = getResourceURL('tutorial_sounds/lighter_on.wav');
    const BUTANE_SOUND_URL = getResourceURL('tutorial_sounds/butane.wav');

    // TODO: fix this in the client, changing the sound volume while a sound is playing doesn't seem to work right now
    const DYNAMIC_SOUND_VOLUME = false;
    const BUTANE_MIN_SOUND_VOLUME = 0.05;

    const FLAME_LENGTH = 0.05;

    const BUTANE_SOUND_SETTINGS = {
        volume: 0.4,
        loop: true,
        playbackGap: 1000,
        playbackGapRange: 1000
    };

    const LIGHTER_ON_SOUND_SETTINGS = {
        volume: 0.5,
        loop: false
    };
    
    function RemoteLogSender(channel, identifier) {
        this.channel = channel;
        this.identifier = identifier;
    }

    RemoteLogSender.prototype = {
        channel: null,
        identifier: null,
        debug: function(message) {
            Messages.sendLocalMessage(this.channel, JSON.stringify({
                message: '[DEBUG ' + this.identifier + '] ' + message
            }));
        }
    };

    var remoteLogSender = null;

    function debugPrint(message) {
        if (remoteLogSender !== null) {
            remoteLogSender.debug(message);
        }
    }
    
    ButaneLighter = function() {
        _this = this;
        _this.triggerValue = 0.0; // be sure to set this value in the constructor, otherwise it will be a shared value
        _this.isFiring = false;
    }
    
    ButaneLighter.prototype = {
        entityID: null,
        lighterOnSound: null,
        butaneSound: null,
        lighterOnSoundInjector: null,
        butaneSoundInjector: null,
        butaneSoundInjectorOptions: null,
        lighterParticleEntity: null,
        buttonBeingPressed: null,
        triggerValue: null,
        isFiring: null,
        getLighterParticleEntity: function() {
            var result = null;
            Entities.getChildrenIDs(_this.entityID).forEach(function(entity) {
                var name = Entities.getEntityProperties(entity, ['name']).name;
                if (name === 'lighter_particle') {
                    result = entity;
                }
            });
            return result;
        },
        preload: function(entityID) {
            _this.entityID = entityID;
            _this.lighterOnSound = SoundCache.getSound(LIGHTER_ON_SOUND_URL);
            _this.butaneSound = SoundCache.getSound(BUTANE_SOUND_URL);
            var properties = Entities.getEntityProperties(_this.entityID, ['userData']);
            try {
                var userData = JSON.parse(properties.userData);
                if (userData['debug'] !== undefined && userData['debug']['sessionUUID'] === MyAvatar.sessionUUID &&
                    userData['debug']['channel'] !== undefined)
                {
                    remoteLogSender = new RemoteLogSender(userData['debug']['channel'], _this.entityID);
                    remoteLogSender.debug('Debugger initialized');
                }
            } catch (e) {
                //failed to detect if we have a debugger
            }
            debugPrint(_this.getLighterParticleEntity());
        },
        startEquip: function(entityID, args) {
            _this.lighterParticleEntity = _this.getLighterParticleEntity();
        },
        continueEquip: function(entityID, args) {
            _this.triggerValue = Controller.getValue(args[0] === 'left' ? Controller.Standard.LT : Controller.Standard.RT);
            if (_this.triggerValue > 0.2) {
                if (!_this.isFiring) {
                    _this.startFiring();
                }
                _this.tryToIgnite();
                _this.updateButaneSound()
                return;
            }
            _this.stopFiring();
        },
        startFiring: function() {
            if (_this.isFiring) {
                return;
            }
            _this.isFiring = true;
            if (_this.lighterOnSound.downloaded) {
                // We don't want to override the default volume setting, so lets clone the default SETTINGS object
                var lighterOnOptions = JSON.parse(JSON.stringify(LIGHTER_ON_SOUND_SETTINGS));
                lighterOnOptions['position'] = Entities.getEntityProperties(_this.entityID, ['position']).position;
                _this.lighterOnSoundInjector = Audio.playSound(_this.lighterOnSound, lighterOnOptions);
            }
            if (_this.butaneSound.downloaded) {
                _this.butaneSoundInjectorOptions = JSON.parse(JSON.stringify(BUTANE_SOUND_SETTINGS));
                _this.butaneSoundInjectorOptions['position'] = Entities.getEntityProperties(_this.lighterParticleEntity, ['position']).position;
                if (DYNAMIC_SOUND_VOLUME) {
                    _this.butaneSoundInjectorOptions['volume'] = BUTANE_MIN_SOUND_VOLUME;
                }
                _this.butaneSoundInjector = Audio.playSound(_this.butaneSound, _this.butaneSoundInjectorOptions);
            }
            Entities.editEntity(_this.lighterParticleEntity, {isEmitting: _this.isFiring});

        },
        stopFiring: function() {
            if (!_this.isFiring) {
                return;
            }
            _this.isFiring = false;
            Entities.editEntity(_this.lighterParticleEntity, {isEmitting: _this.isFiring});
            _this.stopButaneSound();
        },
        tryToIgnite: function() {
            var flameProperties = Entities.getEntityProperties(_this.lighterParticleEntity, ['position', 'rotation']);
            var pickRay = {
                origin: flameProperties.position,
                direction: Quat.inverse(Quat.getFront(flameProperties.rotation))
            }
            var intersection = Entities.findRayIntersection(pickRay, true, [], [_this.entityID, _this.lighterParticleEntity]);
            if (intersection.intersects && intersection.distance <= FLAME_LENGTH && intersection.properties.script !== '') {
                Entities.callEntityMethod(intersection.properties.id, 'onLit', [_this.triggerValue]);
                debugPrint('Light it up! found: ' + intersection.properties.id);
            }
        },
        releaseEquip: function(entityID, args) {
            _this.stopFiring();
            // reset trigger value;
            _this.triggerValue = 0.0;
        },
        updateButaneSound: function() {
            if (_this.butaneSoundInjector !== null && _this.butaneSoundInjector.isPlaying()) {
                _this.butaneSoundInjectorOptions = _this.butaneSoundInjector.options;
                _this.butaneSoundInjectorOptions['position'] = Entities.getEntityProperties(_this.entityID, ['position']).position;
                if (DYNAMIC_SOUND_VOLUME) {
                    _this.butaneSoundInjectorOptions['volume'] = ((BUTANE_SOUND_SETTINGS.volume - BUTANE_MIN_SOUND_VOLUME) *
                        _this.triggerValue) + BUTANE_MIN_SOUND_VOLUME;
                }
                _this.butaneSoundInjector.options = _this.butaneSoundInjectorOptions;
            }
        },
        stopButaneSound: function() {
            if (_this.butaneSoundInjector !== null && _this.butaneSoundInjector.isPlaying()) {
                _this.butaneSoundInjector.stop();
            }
            _this.butaneSoundInjector = null;
        },
        unload: function() {
            _this.stopButaneSound();
        },
    };
    return new ButaneLighter();
})
