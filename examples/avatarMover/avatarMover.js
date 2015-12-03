(function() {
    this.defaultRange = 5;
    this.acceleration = {
        x: 0,
        y: 0,
        z: 0
    };
    this.onColor = {
        red: 77,
        green: 11,
        blue: 111
    };
    this.offColor = {
        red: 200,
        green: 0,
        blue: 0
    };
    var self = this;
    //Default forward direction of mover object
    this.forward = {
        x: 0,
        y: 0,
        z: -1
    };
    this.isMoving = false;
    this.velocity = {
        x: 0,
        y: 0,
        z: 0
    };
    this.defaultThrust = 500;
    this.maxRotMixVal = 0.01;
    this.minRotMixVal = this.maxRotMixVal * 0.5;
    this.minThrustPercentage = 0.2;
    this.userData = {};


    this.getUserData = function() {
        if (this.properties.userData) {
            this.userData = JSON.parse(this.properties.userData);
        }
    }

    this.updateUserData = function() {
        Entities.editEntity(this.entityId, {
            userData: JSON.stringify(this.userData)
        });
    }


    this.toggleMover = function() {
        if (!this.userData.active) {
            this.activate();
        } else if (this.userData.active) {
            this.deactivate();
        }

    }

    this.clickReleaseOnEntity = function(entityId, mouseEvent) {
        this.entityId = entityId
        if (mouseEvent.isLeftButton) {
            this.toggleMover();
        }
    }

    this.activate = function() {
        //activate a light at the movers position
        this.properties = Entities.getEntityProperties(this.entityId);
        this.getUserData();
        this.userData.active = true;
        this.initUserData();
        var lightPos = this.properties.position;
        lightPos.y += .1;
        this.light = Entities.addEntity({
            type: "Light",
            position: lightPos,
            isSpotlight: false,
            dimensions: {
                x: 2,
                y: 2,
                z: 2
            },
            color: this.onColor,
            intensity: 10
                // rotation: {x : 0, y: Math.PI/2, z: 0}
        });

        this.field = Overlays.addOverlay("sphere", {
            position: this.properties.position,
            size: this.userData.range,
            solid: false,
            color: {
                red: 250,
                green: 10,
                blue: 10
            },
        })

        //change color
        Entities.editEntity(this.entityId, {
            color: this.onColor,
        });
    }

    this.initUserData = function() {
        this.userData.range = this.userData.range || this.defaultRange;
        this.userData.thrust = this.userData.thrust || this.defaultThrust;
        this.updateUserData();
    }

    this.updateOverlays = function() {
        if (this.field) {
            Overlays.editOverlay(this.field, {
                size: this.userData.range
            });
        }
    }


    this.deactivate = function() {
        this.userData.active = false;
        this.updateUserData();
        Entities.editEntity(this.entityId, {
            color: this.offColor
        });
        this.cleanUp();
    }

    this.scriptEnding = function() {
        this.cleanUp();
    }

    this.update = function(deltaTime) {
        self.properties = Entities.getEntityProperties(self.entityId);
        self.getUserData();
        self.updateOverlays();
        if (!self.userData.active) {
            return;
        }
        self.distance = Vec3.distance(MyAvatar.position, self.properties.position);
        if (self.distance < self.userData.range) {
            self.rotationMixVal = map(self.distance, 0, self.userData.range, self.maxRotMixVal, self.minRotMixVal);

            //We want to extract yaw from rotated object so avatars do not pith or roll, as they will be stuck that way.
            self.sanitizedRotation = Quat.fromPitchYawRollDegrees(0, Quat.safeEulerAngles(self.properties.rotation).y, 0);
            self.newOrientation = Quat.mix(MyAvatar.orientation, self.sanitizedRotation, self.rotationMixVal);
            MyAvatar.orientation = self.newOrientation;

            self.rotatedDir = {
                x: self.forward.x,
                y: self.forward.y,
                z: self.forward.z
            };
            self.rotatedDir = Vec3.multiplyQbyV(self.properties.rotation, self.rotatedDir);

            self.thrust = map(self.distance, 0, self.userData.range, self.userData.thrust, self.userData.thrust * self.minThrustPercentage);
            self.direction = Vec3.normalize(self.rotatedDir);
            self.velocity = Vec3.multiply(self.direction, self.thrust);
            MyAvatar.addThrust(Vec3.multiply(self.velocity, deltaTime));
        }

    }


    this.preload = function(entityId) {
        this.entityId = entityId;
    }

    this.unload = function() {
        Script.update.disconnect(this.update);
        this.cleanUp();
    }


    this.cleanUp = function() {
        Entities.deleteEntity(this.light);
        Overlays.deleteOverlay(this.field);
    }

    function map(value, min1, max1, min2, max2) {
        return min2 + (max2 - min2) * ((value - min1) / (max1 - min1));
    }

    Script.scriptEnding.connect(this.scriptEnding);
    Script.update.connect(this.update);

});