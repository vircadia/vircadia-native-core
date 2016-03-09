
(function() {
    var _this;
    Script.include("../libraries/utils.js");
    VRVJSoundEntity = function() {
        _this = this;
     
    };

    VRVJSoundEntity.prototype = {
        playSound: function() {
            // _this.soundInjector = Audio.playSound(_this.clip, {position: _this.position, volume: 1.0});
        },

        preload: function(entityID) {
            _this.entityID = entityID;
            _this.position = Entities.getEntityProperties(_this.entityID, "position").position;
            _this.userData  = getEntityUserData(_this.entityID);
            _this.clip = SoundCache.getSound(_this.userData.soundURL);

        },

        unload: function() {
            if (_this.soundInjector) {
                _this.soundInjector.stop();
                delete _this.soundInjector;
            }
        }

   
    };

    // entity scripts always need to return a newly constructed object of our type
    return new VRVJSoundEntity();
});