//
//  xylophoneKey.js
//
//  Created by Patrick Gosch on 03/28/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    Script.include(Script.resolvePath("pUtils.js"));
    var TIMEOUT = 150;
    var TEXGRAY = Script.resolvePath("xylotex_bar_gray.png");
    var TEXBLACK = Script.resolvePath("xylotex_bar_black.png");
    var _this;

    function XylophoneKey() {
        _this = this;
    }

    XylophoneKey.prototype = {
        sound: null,
        isWaiting: false,
        homePos: null,
        injector: null,

        preload: function(entityID) {
            _this.entityID = entityID;
            var soundURL = Script.resolvePath(JSON.parse(Entities.getEntityProperties(_this.entityID, ["userData"]).userData).soundFile);
            _this.sound = SoundCache.getSound(soundURL);
            //Explicitly setting dimensions is a workaround for collisionWithEntity not being triggered after entity is reloaded. Fogbugz Case no. 1939
            Entities.editEntity(_this.entityID, {dimensions: {x: 0.15182036161422729, y: 0.049085158854722977, z: 0.39702033996582031}});
        },

        collisionWithEntity: function(thisEntity, otherEntity, collision) {
            if (collision.type === 0) {
                _this.hit();
            }
        },

        clickDownOnEntity: function() {
            _this.hit();
        },

        hit: function() {
            if (!_this.isWaiting) {
                _this.isWaiting = true;
                _this.homePos = Entities.getEntityProperties(_this.entityID, ["position"]).position;
                _this.injector = Audio.playSound(_this.sound, {position: _this.homePos, volume: 1});
                Controller.triggerHapticPulse(1, 15, 2); //This should be made to only pulse the hand thats holding the mallet.
                editEntityTextures(_this.entityID, "file5", TEXGRAY);
                var newPos = Vec3.sum(_this.homePos, {x:0,y:-0.025,z:0});
                Entities.editEntity(_this.entityID, {position: newPos});
                _this.timeout();
            }
        },

        timeout: function() {
            Script.setTimeout(function() {
                editEntityTextures(_this.entityID, "file5", TEXBLACK);
                Entities.editEntity(_this.entityID, {position: _this.homePos});
                _this.isWaiting = false;
            }, TIMEOUT);
        },
    };

    return new XylophoneKey();

});
