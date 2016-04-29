(function() {
    // The attached entity will move away from you if you are too close, checking at distanceRate.
    // See tests/performance/simpleKeepAway.js
    var entityID,
        distanceRate = 1, // hertz
        distanceAllowance = 3, // meters
        distanceScale = 0.5, // meters/second
        distanceTimer;

    function moveDistance() { // every user checks their distance and tries to claim if close enough.
        var me = MyAvatar.position,
            ball = Entities.getEntityProperties(entityID, ['position']).position;
        ball.y = me.y;
        var vector = Vec3.subtract(ball, me);

        if (Vec3.length(vector) < distanceAllowance) {
            Entities.editEntity(entityID, {
                velocity: Vec3.multiply(distanceScale, Vec3.normalize(vector))
            });
        }
    }

    this.preload = function(givenEntityID) {
        var properties = Entities.getEntityProperties(givenEntityID, ['userData']),
            userData = properties.userData && JSON.parse(properties.userData);
        entityID = givenEntityID;
        if (userData) {
            distanceRate = userData.distanceRate || distanceRate;
            distanceAllowance = userData.distanceAllowance || distanceAllowance;
            distanceScale = userData.distanceScale || distanceScale;
        }

        // run all the time by everyone:
        distanceTimer = Script.setInterval(moveDistance, distanceRate);
    };
    this.unload = function() {
        Script.clearTimeout(distanceTimer);
    };
})