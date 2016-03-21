
(function() {
    Script.include("../../libraries/virtualBaton.js");

    var baton;
    var iOwn = false;
    var updateLoopConnected = false;

    var _this;
    BatonSoundEntity = function() {
        _this = this;
        _this.drumSound = SoundCache.getSound("https://s3.amazonaws.com/hifi-public/sounds/Drums/deepdrum1.wav");
        _this.injectorOptions = {position: MyAvatar.position, loop: false, volume: 0.7};
    };

    function startUpdate() {
        iOwn = true;
        updateLoopConnected = true;
        print("START UPDATE");
        Script.update.connect(_this.update);
    }

    function stopUpdateAndReclaim() {
        // when the baton is release
        print("CLAIM BATON")
        if (updateLoopConnected === true) {
            updateLoopConnected = false;
            Script.update.disconnect(_this.update);
        }
        iOwn = false;


        // hook up callbacks to the baton
        baton.claim(startUpdate, stopUpdateAndReclaim);
    }

    BatonSoundEntity.prototype = {

        update: function() {
            if (iOwn === false) {
                return;
            }

            print("EBL I AM UPDATING");
        },

        preload: function(entityID) {
            _this.entityID = entityID;
            print("PRELOAD ENTITY SCRIPT!!!");
            baton = virtualBaton({
                // One winner for each entity
                batonName: "io.highfidelity.soundEntityBatonTest:" + _this.entityID
            });
            Audio.playSound(_this.drumSound, _this.injectorOptions);
            stopUpdateAndReclaim();
        },

        unload: function() {
            if (updateLoopConnected === true) {
                updateLoopConnected = false;
                Script.update.disconnect(_this.update);
            }
        }

   
    };

    // entity scripts always need to return a newly constructed object of our type
    return new BatonSoundEntity();
});