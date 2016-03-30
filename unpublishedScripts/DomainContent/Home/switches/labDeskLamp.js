(function() {
    var SEARCH_RADIUS = 10;
    var _this;
    
    var utilitiesScript = Script.resolvePath('../utils.js');

    Script.include(utilitiesScript);
    
    Switch = function() {
        _this = this;
        this.switchSound = SoundCache.getSound("atp:/switches/lamp_switch_2.wav");
    };

    Switch.prototype = {
        prefix: 'hifi-home-lab-desk-lamp',
        clickReleaseOnEntity: function(entityID, mouseEvent) {
            if (!mouseEvent.isLeftButton) {
                return;
            }
            this.toggleLights();
        },

        startNearTrigger: function() {
            this.toggleLights();
        },

        modelEmitOn: function(discModel) {
            Entities.editEntity(glowDisc, {
                textures: {
                    "Tex.Lamp-Bulldog": "http://hifi-content.s3.amazonaws.com/alan/dev/Lamp-Bulldog-Base.fbx/Lamp-Bulldog-Base.fbm/dog_statue.jpg",
                    "Texture.001": "http://hifi-content.s3.amazonaws.com/alan/dev/Lamp-Bulldog-Base.fbx/Lamp-Bulldog-Base.fbm/Emissive-Map.png"
                }
            })

        },

        modelEmitOff: function(discModel) {
            Entities.editEntity(glowDisc, {
                textures: {
                    "Tex.Lamp-Bulldog": "http://hifi-content.s3.amazonaws.com/alan/dev/Lamp-Bulldog-Base.fbx/Lamp-Bulldog-Base.fbm/dog_statue.jpg",
                    "Texture.001": ""
                }
            })
        },

        masterLightOn: function(masterLight) {
            Entities.editEntity(masterLight, {
                visible: true
            });
        },

        masterLightOff: function() {
            Entities.editEntity(masterLight, {
                visible: false
            });
        },


        findMasterLights: function() {
            var found = [];
            var results = Entities.findEntities(this.position, SEARCH_RADIUS);
            results.forEach(function(result) {
                var properties = Entities.getEntityProperties(result);
                if (properties.name === _this.prefix + "spotlight") {
                    found.push(result);
                }
            });
            return found;
        },

        findEmitModels: function() {
            var found = [];
            var results = Entities.findEntities(this.position, SEARCH_RADIUS);
            results.forEach(function(result) {
                var properties = Entities.getEntityProperties(result);
                if (properties.name === _this.prefix + "model") {
                    found.push(result);
                }
            });
            return found;
        },

        toggleLights: function() {

            this._switch = getEntityCustomData('home-switch', this.entityID, {
                state: 'off'
            });

            var masterLights = this.findMasterLights();
            var emitModels = this.findEmitModels();

            if (this._switch.state === 'off') {

                masterLights.forEach(function(masterLight) {
                    _this.masterLightOn(masterLight);
                });
                emitModels.forEach(function(emitModel) {
                    _this.modelEmitOn(emitModel);
                });
                setEntityCustomData('home-switch', this.entityID, {
                    state: 'on'
                });

            } else {

                masterLights.forEach(function(masterLight) {
                    _this.masterLightOff(masterLight);
                });
                emitModels.forEach(function(emitModel) {
                    _this.modelEmitOff(emitModel);
                });
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