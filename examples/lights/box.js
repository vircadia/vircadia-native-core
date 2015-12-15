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
            var parsedUserData = JSON.parse(entityProperties.userData);
            var userData = parsedUserData.lightModifierKey;
        },
        startNearGrab: function() {
            this.setInitialProperties();
        },
        startDistantGrab: function() {
            this.setInitialProperties();
        },
        setInitialProperties: function() {
            this.initialProperties = Entities.getEntityProperties(this.entityID);
        },
        getClampedPosition: function() {
            var dPosition;
           // dPosition = Vec3.subtract(MyAvatar.position, this.previousPosition);
            //convert to localFrame
          //  dPosition = Vec3.multiplyQbyV(Quat.inverse(MyAvatar.orientation), dPosition);

            return dPosition;
        },
        getClampedRotation: function() {
            var rotation = initialProperties.rotation;
            return rotation;
        },
        continueDistantGrab: function() {
            var currentPosition = this.getClampedPosition();
            var distance = Vec3.distance(this.initialProperties.position, currentPosition);

            if (userData.sliderType === 'color_red' || userData.sliderType === 'color_green' || userData.sliderType === 'color_blue') {
                this.sliderValue = scaleValueBasedOnDistanceFromStart(distance, COLOR_MAX);
            }
            if (userData.sliderType === 'intensity') {
                this.sliderValue = scaleValueBasedOnDistanceFromStart(distance, INTENSITY_MAX);
            }
            if (userData.sliderType === 'cutoff') {
                this.sliderValue = scaleValueBasedOnDistanceFromStart(distance, CUTOFF_MAX);
            }
            if (userData.sliderType === 'exponent') {
                this.sliderValue = scaleValueBasedOnDistanceFromStart(distance, EXPONENT_MAX);
            };

            Entities.editEntity(this.entityID, {
                position: currentPosition,
                // rotation: this.getClampedRotation()
            });
        },
        releaseGrab: function() {
            Entities.editEntity(this.entityID, {
                velocity: {
                    x: 0,
                    y: 0,
                    z: 0
                }
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
            var message = {
                lightID: userData.lightID,
                sliderType: userData.sliderType,
                sliderValue: this.sliderValue
            }
            Messages.sendMessage('Hifi-Slider-Value-Reciever', JSON.stringify(message));
        }
    };

    return new Box();
});