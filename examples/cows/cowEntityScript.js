(function() {
    Script.include("../libraries/utils.js");

    var _this = this;



    this.preload = function(entityID) {
        print("EBL Preload!!");
        _this.entityID = entityID;
        _this.mooSound = SoundCache.getSound("https://s3-us-west-1.amazonaws.com/hifi-content/eric/Sounds/moo.wav")
        _this.mooSoundOptions = {volume: 0.7, loop: false};
    }

    this.collisionWithEntity = function(myID, otherID, collisionInfo) {
        print("EBL COLLISION WITH ENTITY!");
        this.untipCow();
    }

    this.untipCow = function() {
        // keep yaw but reset pitch and roll
        var cowProps = Entities.getEntityProperties(_this.entityID, ["rotation", "position"]);
        var eulerRotation = Quat.safeEulerAngles(cowProps.rotation);
        eulerRotation.x = 0;
        eulerRotation.z = 0;
        var newRotation = Quat.fromVec3Degrees(eulerRotation);
        var newRotation = Entities.editEntity(_this.entityID, {
            rotation: newRotation
        });


        _this.mooSoundOptions.position = cowProps.position;
        if (!_this.soundInjector) {
            _this.soundInjector = Audio.playSound(_this.mooSound, _this.mooSoundOptions);
        } else {
            _this.soundInjector.setOptions(_this.mooSoundOptions);
            _this.soundInjector.restart();
        }
    }


});