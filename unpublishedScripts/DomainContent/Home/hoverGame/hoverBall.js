(function() {
    var _this;
    HoverBall = function() {
        _this = this;
    }

    var MIN_DISTANCE_THRESHOLD = 0.075;

    var CENTER_POINT_LOCATION = {
        x: 0,
        y: 0,
        z: 0
    };

    HoverBall.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;

        },
        unload: function() {

        },
        startDistanceGrab: function() {

        },
        continueDistantGrab: function() {
            var position = Entities.getEntityProperties(_this.entityID).position;
            var distanceFromCenterPoint = Vec3.distance(position, CENTER_POINT_LOCATION);
            if (distanceFromCenterPoint < MIN_DISTANCE_THRESHOLD) {

                _this.turnOnGlow();
            } else {
                _this.turnOffGlow();
            }
        },
        releaseGrab: function() {
            _this.turnOffGlow();
        },
        turnOnGlow: function() {

        },
        turnOffGlow: function() {

        },
        findHoverContainer: function() {
            var position = Entities.getEntityProperties(_this.entityID).position;
            var results = Entities.findEntities(position, 3);
            results.forEach(function(item) {
                var props = Entities.getEntityProperties(item);
                if (props.name.indexOf('hoverGame_container') > -1) {
                    return item
                }
            })
        },

    }

    return new HoverBall();

})