(function() {
    var SEARCH_RADIUS = 10;

    var EMISSIVE_TEXTURE_URL = "http://hifi-content.s3.amazonaws.com/highfidelitysign_white_emissive.png";

    var DIFFUSE_TEXTURE_URL = "http://hifi-content.s3.amazonaws.com/highfidelity_diffusebaked.png";

    var _this;
    var utilitiesScript = Script.resolvePath('../../../../libraries/utils.js');
    Script.include(utilitiesScript);
    Switch = function() {
        _this = this;
        this.switchSound = SoundCache.getSound("https://hifi-public.s3.amazonaws.com/sounds/Switches%20and%20sliders/lamp_switch_2.wav");
    };

    Switch.prototype = {
        prefix: 'hifi-home-living-room-desk-lamp-',
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

            })
        },

        modelEmitOff: function(discModel) {
            Entities.editEntity(glowDisc, {

            })
        },

        masterLightOn: function(masterLight) {
            Entities.editEntity(masterLight, {
                visible: true
            });
        },

        masterLightOff: function(masterLight) {
            print("EBL TURN LIGHT OFF");
            Entities.editEntity(masterLight, {
                visible: false
            });
        },


        findMasterLights: function() {
            var found = [];
            var results = Entities.findEntities(_this.position, SEARCH_RADIUS);
            results.forEach(function(result) {
                var properties = Entities.getEntityProperties(result);
                if (properties.name === _this.prefix + "spotlight") {
                    print("EBL FOUND THE SPOTLIGHT!");
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
            print("EBL TOGGLE LIGHTS");

            this._switch = getEntityCustomData('home-switch', this.entityID, {
                state: 'off'
            });

            var masterLights = this.findMasterLights();
            var emitModels = this.findEmitModels();

            if (this._switch.state === 'off') {
                print("EBL TURN LIGHTS ON");
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
                print("EBL TURN LIGHTS OFF");
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

            Audio.playSound(this.switchSound, {
                volume: 0.5,
                position: this.position
            });

        },


        preload: function(entityID) {
            print("EBL PRELOAD LIGHT SWITCH SCRIPT");
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