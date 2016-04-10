//
//
//  Created by The Content Team 4/10/216
//  Copyright 2016 High Fidelity, Inc.
//
//  this switch finds a fan entity and changes its angular damping so that it spins or doesn't spin as appropriate.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var SEARCH_RADIUS = 100;
    var _this;
    var utilitiesScript = Script.resolvePath('../utils.js');
    Script.include(utilitiesScript);
    Switch = function() {
        _this = this;
        this.switchSound = SoundCache.getSound("atp:/switches/lamp_switch_2.wav");
        _this.FAN_VOLUME = 0.1;
        _this.FAN_SOUND_ENTITY_NAME = "home_sfx_ceiling_fan";
    };

    Switch.prototype = {
        prefix: 'home_living_room',
        clickReleaseOnEntity: function(entityID, mouseEvent) {
            if (!mouseEvent.isLeftButton) {
                return;
            }
            this.toggle();
        },

        startNearTrigger: function() {
            this.toggle();
        },

        fanRotationOn: function() {
            var success = Entities.editEntity(_this.fan, {
                angularDamping: 0,
                angularVelocity: {
                    x: 0,
                    y: 4,
                    z: 0
                },
            });
        },

        fanRotationOff: function() {
            Entities.editEntity(_this.fan, {
                angularDamping: 0.5,
            })
        },

        fanSoundOn: function() {
            if (!_this.fanSoundEntity) {
                return;
            }
            var soundUserData = getEntityCustomData("soundKey", _this.fanSoundEntity);
            if (!soundUserData) {
                print("NO SOUND USER DATA! RETURNING.");
                return;
            }
            soundUserData.volume = _this.FAN_VOLUME;
            setEntityCustomData("soundKey", _this.fanSoundEntity, soundUserData);



        },

        fanSoundOff: function() {
            if (!_this.fanSoundEntity) {
                return;
            }
            var soundUserData = getEntityCustomData("soundKey", _this.fanSoundEntity);
            if (!soundUserData) {
                print("NO SOUND USER DATA! RETURNING.");
                return;
            }
            soundUserData.volume = 0.0;
            setEntityCustomData("soundKey", _this.fanSoundEntity, soundUserData);
        },

        findFan: function() {
            var found = [];
            var results = Entities.findEntities(this.position, SEARCH_RADIUS);
            results.forEach(function(result) {
                var properties = Entities.getEntityProperties(result);
                if (properties.name === _this.prefix + "_model_ceiling_fan_blades") {
                    found.push(result);
                }
            });
            return found[0];
        },

        findFanSoundEntity: function() {
            var entities = Entities.findEntities(_this.position, SEARCH_RADIUS);
            for (var i = 0; i < entities.length; i++) {
                var name = Entities.getEntityProperties(entities[i], "name").name;
                if (name === _this.FAN_SOUND_ENTITY_NAME) {
                    return entities[i];
                }
            }

            return null;
        },

        toggle: function() {
            this.fan = this.findFan();
            _this.fanSoundEntity = _this.findFanSoundEntity();
            this._switch = getEntityCustomData('home-switch', this.entityID, {
                state: 'off'
            });


            if (this._switch.state === 'off') {
                this.fanRotationOn();
                this.fanSoundOn();
                setEntityCustomData('home-switch', this.entityID, {
                    state: 'on'
                });

            } else {
                this.fanRotationOff();
                this.fanSoundOff();

                setEntityCustomData('home-switch', this.entityID, {
                    state: 'off'
                });
            }

            this.flipSwitch();
            Audio.playSound(this.switchSound, {
                volume: 0.5,
                position: this.position
            });

        },

        flipSwitch: function() {
            var rotation = Entities.getEntityProperties(this.entityID, "rotation").rotation;
            var axis = {
                x: 0,
                y: 1,
                z: 0
            };
            var dQ = Quat.angleAxis(180, axis);
            rotation = Quat.multiply(rotation, dQ);

            Entities.editEntity(this.entityID, {
                rotation: rotation
            });
        },

        preload: function(entityID) {
            this.entityID = entityID;
            setEntityCustomData('grabbableKey', this.entityID, {
                wantsTrigger: true
            });

            var properties = Entities.getEntityProperties(this.entityID);

            //The light switch is static, so just cache its position once
            this.position = Entities.getEntityProperties(this.entityID, "position").position;
        }
    };

    // entity scripts always need to return a newly constructed object of our type
    return new Switch();
});