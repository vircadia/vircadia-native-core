(function() {
    this.entityID = null;
    this.lightID = null;
    this.sound = null;

    function checkEntity(entityID) {
        return entityID && Entities.identifyEntity(entityID).isKnownID;
    }
    function getUserData(entityID) {
        var properties = Entities.getEntityProperties(entityID);
        if (properties.userData) {
            return JSON.parse(properties.userData);
        }
        print("Warning: light controller has no user data.");
        // TODO: Remove before merge
        this.DO_NOT_MERGE();
        return getUserData(entityID);
        ////////////////////////////
        return null;
    }
    // Download sound if needed
    this.maybeDownloadSound = function() {
        if (this.sound === null) {
            this.sound = SoundCache.getSound("http://public.highfidelity.io/sounds/Footsteps/FootstepW3Left-12db.wav");
        }
    }
    // Play switch sound
    this.playSound = function() {
        if (this.sound && this.sound.downloaded) {
            Audio.playSound(this.sound, { position: Entities.getEntityProperties(this.entityID).position });
        } else {
            print("Warning: Couldn't play sound.");
        }
    }
    // Toggles the associated light entity
    this.toggleLight = function() {
        if (this.lightID) {
            var lightProperties = Entities.getEntityProperties(this.lightID);
            Entities.editEntity(this.lightID, { visible: !lightProperties.visible });
        } else {
            print("Warning: No light to turn on/off");
        }
    }
    
    this.createLight = function() {
        var lightProperties = getUserData(this.entityID).lightDefaultProperties;
        if (lightProperties) {
            var properties = Entities.getEntityProperties(this.entityID);
            
            lightProperties.rotation = Quat.multiply(properties.rotation, lightProperties.rotation);
            lightProperties.position = Vec3.sum(properties.position,
                                                Vec3.multiplyQbyV(properties.rotation, lightProperties.position));
            print("Created light");
            return Entities.addEntity(lightProperties);
        } else {
            print("Warning: light controller has no default light.");
            return null;
        }
    }
    
    this.updateLight = function() {
        // Find valid light
        if (!checkEntity(this.lightID)) {
            var userData = getUserData(this.entityID);
            if (userData.lightID && checkEntity(userData.lightID)) {
                this.lightID = userData.lightID;
            } else {
                this.lightID = null;
            }
        }

        // No valid light, create one
        if (!this.lightID) {
            this.lightID = this.createLight();
        }
    }

    this.preload = function(entityID) {
        this.entityID = entityID;
        this.maybeDownloadSound();
    };
    
    this.clickReleaseOnEntity = function(entityID, mouseEvent) {
        if (mouseEvent.isLeftButton) {
            this.updateLight();
            this.toggleLight();
            this.playSound();
        } else if (mouseEvent.isRightButton) {
            print("Right button");
        }
    };
    
    this.DO_NOT_MERGE = function() {
        var userData = {
            lightID: null,
            lightDefaultProperties: {
                type: "Light",  
                position: { x: 0, y: 0.5, z: 0 },
                dimensions: { x: 2, y: 2, z: 2 },
                angularVelocity: { x: 0, y: 0, z: 0 },
                angularDamping: 0,

                isSpotlight: true,
                color: { red: 255, green: 0, blue: 0 },
                diffuseColor: { red: 255, green: 0, blue: 0 },
                ambientColor: { red: 0, green: 0, blue: 255 },
                specularColor: { red: 0, green: 255, blue: 0 },

                intensity: 10,
                constantAttenuation: 0,
                linearAttenuation: 1,
                quadraticAttenuation: 0,
                exponent: 0,
                cutoff: 180, // in degrees
            }
        };
        Entities.editEntity(this.entityID, { userData: JSON.stringify(userData) });
    }
})