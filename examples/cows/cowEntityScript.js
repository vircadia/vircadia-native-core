(function() {
    Script.include("../libraries/utils.js");

    var _this = this;
    _this.COLLISION_COOLDOWN_TIME = 5000;


    this.preload = function(entityID) {
        print("EBL Preload!!");
        _this.entityID = entityID;
        _this.mooSound = SoundCache.getSound("https://s3-us-west-1.amazonaws.com/hifi-content/eric/Sounds/moo.wav")
        _this.mooSoundOptions = {volume: 0.7, loop: false};
        _this.timeSinceLastCollision = 0;
        _this.shouldUntipCow = true;
    }

    this.collisionWithEntity = function(myID, otherID, collisionInfo) {
        print("EBL COLLISION WITH ENTITY!");
        if(_this.shouldUntipCow) {
          Script.setTimeout(function() {
             _this.untipCow();  
            _this.shouldUntipCow = true;
          }, _this.COLLISION_COOLDOWN_TIME);
        } 

        _this.shouldUntipCow = false;
        
    }

    this.untipCow = function() {
        print("EBL UNTIP COW");
        // keep yaw but reset pitch and roll
        var cowProps = Entities.getEntityProperties(_this.entityID, ["rotation", "position"]);
        var eulerRotation = Quat.safeEulerAngles(cowProps.rotation);
        eulerRotation.x = 0;
        eulerRotation.z = 0;
        var newRotation = Quat.fromVec3Degrees(eulerRotation);
        Entities.editEntity(_this.entityID, {
            rotation: newRotation,
            velocity: {x: 0, y: 0, z: 0},
            angularVelocity: {x: 0, y: 0, z:0}
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