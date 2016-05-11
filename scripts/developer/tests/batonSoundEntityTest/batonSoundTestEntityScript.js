
(function() {
    Script.include("../../libraries/virtualBaton.js");

    var baton;

    var _this;
    BatonSoundEntity = function() {
        _this = this;
        _this.drumSound = SoundCache.getSound("https://s3.amazonaws.com/hifi-public/sounds/Drums/deepdrum1.wav");
        _this.injectorOptions = {position: MyAvatar.position, loop: false, volume: 1};
        _this.soundIntervalConnected = false;
        _this.batonDebugModel = Entities.addEntity({
            type: "Box",
            color: {red: 200, green: 10, blue: 200},
            position: Vec3.sum(MyAvatar.position, {x: 0, y: 1, z: 0}),
            dimensions: {x: 0.5, y: 1, z: 0},
            parentID: MyAvatar.sessionUUID,
            visible: false
        });
    };

    function startUpdate() {
        // We are claiming the baton! So start our clip
        if (!_this.soundInjector) {
            // This client hasn't created their injector yet so create one
           _this.soundInjector = Audio.playSound(_this.drumSound, _this.injectorOptions);
        } else {
            // We already have our injector so just restart it
            _this.soundInjector.restart();
        }
        print("EBL START UPDATE");
        Entities.editEntity(_this.batonDebugModel, {visible: true});
        _this.playSoundInterval = Script.setInterval(function() {
            _this.soundInjector.restart();
        }, _this.drumSound.duration * 1000); // Duration is in seconds so convert to ms
        _this.soundIntervalConnected = true;
    }

    function stopUpdateAndReclaim() {
        print("EBL STOP UPDATE AND RECLAIM")
        // when the baton is release
        if (_this.soundIntervalConnected === true) {
            Script.clearInterval(_this.playSoundInterval);
            _this.soundIntervalConnected = false;
            print("EBL CLEAR INTERVAL")
        }
        Entities.editEntity(_this.batonDebugModel, {visible: false});
        // hook up callbacks to the baton
        baton.claim(startUpdate, stopUpdateAndReclaim);
    }

    BatonSoundEntity.prototype = {


        preload: function(entityID) {
            _this.entityID = entityID;
            print("EBL PRELOAD ENTITY SCRIPT!!!");
            baton = virtualBaton({
                // One winner for each entity
                batonName: "io.highfidelity.soundEntityBatonTest:" + _this.entityID,
                // debugFlow: true
            });
            stopUpdateAndReclaim();
        },

        unload: function() {
            print("EBL UNLOAD");
            // baton.release();
            baton.unload();
            Entities.deleteEntity(_this.batonDebugModel);
            if (_this.soundIntervalConnected === true) {
                Script.clearInterval(_this.playSoundInterval);
                _this.soundIntervalConnected = false;
                _this.soundInjector.stop();
                delete _this.soundInjector;
            }
        }

   
    };

    // entity scripts always need to return a newly constructed object of our type
    return new BatonSoundEntity();
});