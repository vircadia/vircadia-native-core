(function() {

    var AXIS_SCALE = 1;
    var COLOR_MAX = 255;
    var INTENSITY_MAX = 10;
    var CUTOFF_MAX = 360;
    var EXPONENT_MAX = 1;

    function Box() {
        return this;
    }

    Box.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;
            var entityProperties = Entities.getEntityProperties(this.entityID, "userData");
            print('USER DATA:::' + entityProperties.userData)
            var parsedUserData = JSON.parse(entityProperties.userData);
            this.userData = parsedUserData.lightModifierKey;
            this.bPrime = Vec3.subtract(this.userData.endOfAxis, this.userData.axisBasePosition);
            this.bPrimeMagnitude = Vec3.length(this.bPrime);

        },
        startNearGrab: function() {
            this.setInitialProperties();
        },
        startDistantGrab: function() {
            // Entities.editEntity(this.entityID, {
            //     parentID: MyAvatar.sessionUUID,
            //     parentJointIndex: MyAvatar.getJointIndex("LeftHand")
            // });
            this.setInitialProperties();
        },
        setInitialProperties: function() {
            this.initialProperties = Entities.getEntityProperties(this.entityID);
        },
        clampPosition: function() {

            var currentProperties = Entities.getEntityProperties(this.entityID);

            var aPrime = Vec3.subtract(this.userData.axisBasePosition, currentProperties.position);

            var dotProduct = Vec3.dot(aPrime, this.bPrime);

            var scalar = dotProduct / this.bPrimeMagnitude;

            print('SCALAR:::' + scalar);

            var projection = Vec3.sum(this.userData.axisBasePosition, Vec3.multiply(scalar, Vec3.normalize(this.bPrime)));

            this.currentProjection = projection;

        },
        continueDistantGrab: function() {
            //     this.clampPosition();
            print('distant grab')
            var currentPosition = Entities.getEntityProperties(this.entityID, "position").position;

            var distance = Vec3.distance(this.axisBasePosition, this.currentProjection);

            if (this.userData.sliderType === 'color_red' || this.userData.sliderType === 'color_green' || this.userData.sliderType === 'color_blue') {
                this.sliderValue = this.scaleValueBasedOnDistanceFromStart(distance, COLOR_MAX);
            }
            if (this.userData.sliderType === 'intensity') {
                this.sliderValue = this.scaleValueBasedOnDistanceFromStart(distance, INTENSITY_MAX);
            }
            if (this.userData.sliderType === 'cutoff') {
                this.sliderValue = this.scaleValueBasedOnDistanceFromStart(distance, CUTOFF_MAX);
            }
            if (this.userData.sliderType === 'exponent') {
                this.sliderValue = this.scaleValueBasedOnDistanceFromStart(distance, EXPONENT_MAX);
            };


        },
        releaseGrab: function() {
            Entities.editEntity(this.entityID, {
                velocity: {
                    x: 0,
                    y: 0,
                    z: 0
                },
                parentID: null
            })

            this.sendValueToSlider();
        },
        scaleValueBasedOnDistanceFromStart: function(value, max2) {
            var min1 = 0;
            var max1 = AXIS_SCALE;
            var min2 = 0;
            var max2 = max2;
            return min2 + (max2 - min2) * ((value - min1) / (max1 - min1));
        },
        sendValueToSlider: function() {
            var _t = this;
            var message = {
                lightID: _t.userData.lightID,
                sliderType: _t.userData.sliderType,
                sliderValue: _t.sliderValue
            }
            Messages.sendMessage('Hifi-Slider-Value-Reciever', JSON.stringify(message));
        }
    };

    return new Box();
});