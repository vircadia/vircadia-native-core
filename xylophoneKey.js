(function() {
    var BASE_PATH = "http://mpassets.highfidelity.com/3fd92e5e-93cf-4bc1-b2f1-c6bae5629814-v1/";
    Script.include(BASE_PATH + "pUtils.js");
    var TIMEOUT = 150;
    var _this;

    function XylophoneKey() {
        _this = this;
        return;
    }

    XylophoneKey.prototype = {
        sound: null,
        isWaiting: false,
        homePos: null,
        injector: null,

        preload: function(entityID) {
            _this.entityID = entityID;

            var soundURL = BASE_PATH + JSON.parse(Entities.getEntityProperties(_this.entityID, ["userData"]).userData).soundFile;
            //_this.homePos = Entities.getEntityProperties(entityID, ["position"]).position;
            //Entities.editEntity(_this.entityID, {position: _this.homePos}); //This is the workaround for collisionWithEntity not being triggered after entity is reloaded.
            _this.sound = SoundCache.getSound(soundURL);

                    MyAvatar.collisionWithEntity.connect(function(collision){
                        print ("The avatar collided with an entity.");
                        if (!_this.isWaiting && collision.type == 0) {_this.hit();}
                    });
        },

        collisionWithEntity: function(thisEntity, otherEntity, collision) {
            if (!_this.isWaiting && collision.type == 0) {_this.hit();}
        },

        clickDownOnEntity: function() {
            _this.hit();
        },

        hit: function() {
            _this.isWaiting = true;
            _this.homePos = Entities.getEntityProperties(_this.entityID, ["position"]).position;
            _this.injector = Audio.playSound(_this.sound, {position: _this.homePos, volume: 1});
            Controller.triggerHapticPulse(1, 15, 2); //This should be made to only pulse the hand thats holding the mallet.
            editEntityTextures(_this.entityID, "file5", BASE_PATH + "xylotex_bar_gray.png");
            var newPos = Vec3.sum(_this.homePos, {x:0,y:-0.025,z:0});
            Entities.editEntity(_this.entityID, {position: newPos});
            _this.timeout();
        },

        timeout: function() {
            Script.setTimeout(function() {
                editEntityTextures(_this.entityID, "file5", BASE_PATH + "xylotex_bar_black.png");
                Entities.editEntity(_this.entityID, {position: _this.homePos});
                _this.isWaiting = false;
            }, TIMEOUT)
        },

    };

    return new XylophoneKey();

});