// Originally written for the Home content set. Pulled into the tutorial by Ryan Huffman
(function() {

    var MINIMUM_LIGHT_INTENSITY = 50.0;
    var MAXIMUM_LIGHT_INTENSITY = 200.0;
    var LIGHT_FALLOFF_RADIUS = 0.1;
    var LIGHT_INTENSITY_RANDOMNESS = 0.1;

    function randFloat(low, high) {
        return low + Math.random() * (high - low);
    }

    var _this;

    function FlickeringFlame() {
        _this = this;
    }

    var totalTime = 0;
    var spacer = 2;
    FlickeringFlame.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;
            Script.update.connect(this.update);
        },
        update: function(deltaTime) {

            totalTime += deltaTime;
            if (totalTime > spacer) {
                var howManyAvatars = AvatarList.getAvatarIdentifiers().length;
                var intensity = (MINIMUM_LIGHT_INTENSITY + (MAXIMUM_LIGHT_INTENSITY + (Math.sin(totalTime) * MAXIMUM_LIGHT_INTENSITY)));
                intensity += randFloat(-LIGHT_INTENSITY_RANDOMNESS, LIGHT_INTENSITY_RANDOMNESS);

                Entities.editEntity(_this.entityID, {
                    intensity: intensity
                });

                spacer = Math.random(0, 100) * (2 / howManyAvatars);
                totalTime = 0;
            } else {
                //just keep counting
            }
        },
        unload: function() {
            Script.update.disconnect(this.update)
        }
    }

    return new FlickeringFlame


});
