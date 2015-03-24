(function() {
    this.entityID = null;
    this.properties = null;
    this.lightID = null;
    this.sound = null;

    function copyObject(object) {
        return JSON.parse(JSON.stringify(object));
    }
    function didEntityExist(entityID) {
        return entityID && entityID.isKnownID;
    }
    function doesEntityExistNow(entityID) {
        return entityID && getTrueID(entityID).isKnownID;
    }
    function getTrueID(entityID) {
        var properties = Entities.getEntityProperties(entityID);
        return { id: properties.id, creatorTokenID: properties.creatorTokenID, isKnownID: properties.isKnownID };
    }
    function getUserData(entityID) {
        var properties = Entities.getEntityProperties(entityID);
        if (properties.userData) {
            return JSON.parse(properties.userData);
        } else {
            print("Warning: light controller has no user data.");
            return null;
        }
    }
    function updateUserData(entityID, userData) {
        Entities.editEntity(entityID, { userData: JSON.stringify(userData) });
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
            Audio.playSound(this.sound, {
                position: Entities.getEntityProperties(this.entityID).position,
                volume: 0.2
            });
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
    
    this.createLight = function(userData) {
        var lightProperties = copyObject(userData.lightDefaultProperties);
        if (lightProperties) {
            var entityProperties = Entities.getEntityProperties(this.entityID);
            
            lightProperties.visible = false;
            lightProperties.position = Vec3.sum(entityProperties.position,
                                                Vec3.multiplyQbyV(entityProperties.rotation,
                                                                  lightProperties.position));
            return Entities.addEntity(lightProperties);
        } else {
            print("Warning: light controller has no default light.");
            return null;
        }
    }
    
    this.updateLightID = function() {
        var userData = getUserData(this.entityID);
        if (!userData) {
            userData = {
                lightID: null,
                lightDefaultProperties: {
                    type: "Light",
                    position: { x: 0, y: 0, z: 0 },
                    dimensions: { x: 5, y: 5, z: 5 },
                    isSpotlight: false,
                    color: { red: 255, green: 48, blue: 0 },
                    diffuseColor: { red: 255, green: 255, blue: 255 },
                    ambientColor: { red: 255, green: 255, blue: 255 },
                    specularColor: { red: 0, green: 0, blue: 0 },
                    constantAttenuation: 1,
                    linearAttenuation: 0,
                    quadraticAttenuation: 0,
                    intensity: 10,
                    exponent: 0,
                    cutoff: 180, // in degrees
                }
            };
            updateUserData(this.entityID, userData);
        }
        
        // Find valid light
        if (doesEntityExistNow(this.lightID)) {
            if (!didEntityExist(this.lightID)) {
                // Light now has an ID, so update it in userData
                this.lightID = getTrueID(this.lightID);
                userData.lightID = this.lightID;
                updateUserData(this.entityID, userData);
            }
            return;
        }
        
        if (doesEntityExistNow(userData.lightID)) {
            this.lightID = getTrueID(userData.lightID);
            return;
        }

        // No valid light, create one
        this.lightID = this.createLight(userData);
        print("Created new light entity");
        
        // Update user data with new ID
        userData.lightID = this.lightID;
        updateUserData(this.entityID, userData);
    }
    
    this.maybeMoveLight = function() {
        var entityProperties = Entities.getEntityProperties(this.entityID);
        var lightProperties = Entities.getEntityProperties(this.lightID);
        var lightDefaultProperties = getUserData(this.entityID).lightDefaultProperties;
        
        var position = Vec3.sum(entityProperties.position,
                                Vec3.multiplyQbyV(entityProperties.rotation,
                                                  lightDefaultProperties.position));
                                                              
        if (!Vec3.equal(position, lightProperties.position)) {
            print("Lamp entity moved, moving light entity as well");
            Entities.editEntity(this.lightID, { position: position });
        }
    }

    this.updateRelativeLightPosition = function() {
        if (!doesEntityExistNow(this.entityID) || !doesEntityExistNow(this.lightID)) {
            print("Warning: ID invalid, couldn't save relative position.");
            return;
        }

        var userData = getUserData(this.entityID);
        var entityProperties = Entities.getEntityProperties(this.entityID);
        var lightProperties = Entities.getEntityProperties(this.lightID);
        var newProperties = {};

        // Copy only meaningful properties (trying to save space in userData here)
        for (var key in userData.lightDefaultProperties) {
            if (userData.lightDefaultProperties.hasOwnProperty(key)) {
                newProperties[key] = lightProperties[key];
            }
        }

        // Compute new relative position
        newProperties.position = Quat.multiply(Quat.inverse(entityProperties.rotation),
                                                 Vec3.subtract(lightProperties.position,
                                                               entityProperties.position));
        // inverse "visible" because right after we loaded the properties, the light entity is toggled.
        newProperties.visible = !lightProperties.visible;

        userData.lightDefaultProperties = copyObject(newProperties);
        updateUserData(this.entityID, userData);
        print("Relative properties of light entity saved.");
    }

    this.preload = function(entityID) {
        this.entityID = entityID;
        this.maybeDownloadSound();
    };
    
    this.clickReleaseOnEntity = function(entityID, mouseEvent) {
        this.entityID = entityID;
        this.maybeDownloadSound();
        
        if (mouseEvent.isLeftButton) {
            this.updateLightID();
            this.maybeMoveLight();
            this.toggleLight();
            this.playSound();
        } else if (mouseEvent.isRightButton) {
            this.updateRelativeLightPosition();
        }
    };
})