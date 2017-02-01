(function() {
    var MINIMUM_LIGHT_INTENSITY = 100.0;
    var MAXIMUM_LIGHT_INTENSITY = 125.0;

    // Return a random number between `low` (inclusive) and `high` (exclusive)
    function randFloat(low, high) {
        return low + Math.random() * (high - low);
    }

    var self = this;
    this.preload = function(entityID) {
        self.intervalID = Script.setInterval(function() {
            Entities.editEntity(entityID, {
                intensity: randFloat(MINIMUM_LIGHT_INTENSITY, MAXIMUM_LIGHT_INTENSITY)
            });
        }, 100);
    };
    this.unload = function() {
        Script.clearInterval(self.intervalID);
    }
});
