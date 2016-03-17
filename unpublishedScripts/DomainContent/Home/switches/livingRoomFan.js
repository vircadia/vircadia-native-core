(function() {
    var SEARCH_RADIUS = 100;
    var _this;
    var utilitiesScript = Script.resolvePath('../../../../libraries/utils.js');
    Script.include(utilitiesScript);
    Switch = function() {
        _this = this;
        this.switchSound = SoundCache.getSound("https://hifi-public.s3.amazonaws.com/sounds/Switches%20and%20sliders/lamp_switch_2.wav");
    };

    Switch.prototype = {
        prefix: 'hifi-home-living-room-',
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
            print("EBL TURN FAN ON" + JSON.stringify(_this.fan));
          var success=  Entities.editEntity(_this.fan, {
                angularDamping: 0,
                angularVelocity: {
                    x: 0,
                    y: 4,
                    z: 0
                },
            });
        },

        fanRotationOff: function() {
            print("EBL TURN FAN OFF")
            Entities.editEntity(_this.fan, {
                angularDamping: 0.5,
                // angularVelocity:{
                //     x:0,
                //     y:0,
                //     z:0
                // }
            })
        },

        fanSoundOn: function() {

        },

        fanSoundOff: function() {

        },

        ventSoundOn: function() {

        },

        ventSoundOff: function() {

        },

        findFan: function() {
            var found = [];
            var results = Entities.findEntities(this.position, SEARCH_RADIUS);
            results.forEach(function(result) {
                var properties = Entities.getEntityProperties(result);
                if (properties.name === _this.prefix + "ceiling-fan") {
                    print("EBL FOUND FAN");
                    found.push(result);
                }
            });
            return found[0];
        },

        toggle: function() {
            this.fan = this.findFan();
            this._switch = getEntityCustomData('home-switch', this.entityID, {
                state: 'off'
            });

            print("SWITCH STATE " + JSON.stringify(_this._switch));

            if (this._switch.state === 'off') {
                this.fanRotationOn();
                this.fanSoundOn();
                this.ventSoundOn();
                setEntityCustomData('home-switch', this.entityID, {
                    state: 'on'
                });

            } else {
                this.fanRotationOff();
                this.fanSoundOff();
                this.ventSoundOff();

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