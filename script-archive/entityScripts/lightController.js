(function() {
    this.entityID = null;
    this.lightID = null;
    this.sound = null;
    this.soundURLs = ["https://hifi-public.s3.amazonaws.com/sounds/Switches%20and%20sliders/lamp_switch_1.wav",
                      "https://hifi-public.s3.amazonaws.com/sounds/Switches%20and%20sliders/lamp_switch_2.wav",
                      "https://hifi-public.s3.amazonaws.com/sounds/Switches%20and%20sliders/lamp_switch_3.wav"]

    var DEFAULT_USER_DATA = {
        creatingLight: false,
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
        },
        soundIndex: Math.floor(Math.random() * this.soundURLs.length)
    };

    function copyObject(object) {
        return JSON.parse(JSON.stringify(object));
    }
    function didEntityExist(entityID) {
        return entityID;
    }
    function doesEntityExistNow(entityID) {
        return entityID;
    }
    function getTrueID(entityID) {
        var properties = Entities.getEntityProperties(entityID);
        return { id: properties.id };
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
            var soundIndex = getUserData(this.entityID).soundIndex;
            this.sound = SoundCache.getSound(this.soundURLs[soundIndex]);
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
    
    // Checks whether the userData is well-formed and updates it if not
    this.checkUserData = function() {
        var userData = getUserData(this.entityID);
        if (!userData) {
            userData = DEFAULT_USER_DATA;
        } else if (!userData.lightDefaultProperties) {
            userData.lightDefaultProperties = DEFAULT_USER_DATA.lightDefaultProperties;
        } else if (!userData.soundIndex) {
            userData.soundIndex = DEFAULT_USER_DATA.soundIndex;
        }
        updateUserData(this.entityID, userData);
    }
    
    // Create a Light entity
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
    
    // Tries to find a valid light, creates one otherwise
    this.updateLightID = function() {
        // Find valid light
        if (doesEntityExistNow(this.lightID)) {
            return;
        }
        
        var userData = getUserData(this.entityID);
        if (doesEntityExistNow(userData.lightID)) {
            this.lightID = userData.lightID;
            return;
        }

        if (!userData.creatingLight) {
            // No valid light, create one
            userData.creatingLight = true;
            updateUserData(this.entityID, userData);
            this.lightID = this.createLight(userData);
            this.maybeUpdateLightIDInUserData();
            print("Created new light entity");
        }
    }
    
    this.maybeUpdateLightIDInUserData = function() {
        if (getTrueID(this.lightID).isKnownID) {
            this.lightID = getTrueID(this.lightID);
            this.updateLightIDInUserData();
        } else {
            var that = this;
            Script.setTimeout(function() { that.maybeUpdateLightIDInUserData() }, 500);
        }
    }
    
    // Update user data with new lightID
    this.updateLightIDInUserData = function() {
        var userData = getUserData(this.entityID);
        userData.lightID = this.lightID;
        userData.creatingLight = false;
        updateUserData(this.entityID, userData);
    }
    
    // Moves light entity if the lamp entity moved
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

    // Stores light entity relative position in the lamp metadata
    this.updateRelativeLightPosition = function() {
        if (!doesEntityExistNow(this.lightID)) {
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
    
    // This function should be called before any callback is executed
    this.preOperation = function(entityID) {
        this.entityID = entityID;
        
        this.checkUserData();
        this.maybeDownloadSound();
    }

    // Toggles the associated light entity
    this.toggleLight = function() {
        if (this.lightID) {
            var lightProperties = Entities.getEntityProperties(this.lightID);
            Entities.editEntity(this.lightID, { visible: !lightProperties.visible });
            this.playSound();
        } else {
            print("Warning: No light to turn on/off");
        }
    }

    this.preload = function(entityID) {
        this.preOperation(entityID);
    }
    
    this.clickReleaseOnEntity = function(entityID, mouseEvent) {
        this.preOperation(entityID);
        
        if (mouseEvent.isLeftButton) {
            this.updateLightID();
            this.maybeMoveLight();
            this.toggleLight();
        } else if (mouseEvent.isRightButton) {
            this.updateRelativeLightPosition();
        }
    };
})
