
(function() {
    var _this;
    Script.include("../libraries/utils.js");
    VRVJVisualEntity = function() {
        _this = this;
        _this.SOUND_LOOP_NAME
     
    };

    VRVJVisualEntity.prototype = {
        
        releaseGrab: function() {
            // search for nearby sound loop entities and if found, add it as a parent
            _this.searchForNearbySoundLoops();
        },

        searchForNearbySoundLoops: function() {

        },

        preload: function(entityID) {
            _this.entityID = entityID;

        },
    };

    // entity scripts always need to return a newly constructed object of our type
    return new VRVJVisualEntity();
});