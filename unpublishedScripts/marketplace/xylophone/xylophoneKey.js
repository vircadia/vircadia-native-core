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
    var TIMEOUT = 50; // at 30 ms, the key's color sometimes fails to switch when hit
    var TEXTURE_GRAY = Script.resolvePath("xylotex_bar_gray.png");
    var TEXTURE_BLACK = Script.resolvePath("xylotex_bar_black.png");
    var IS_DEBUG = false;
    var _this;
 
    function XylophoneKey() {
        _this = this;
    }

    XylophoneKey.prototype = {
        sound: null,
        isWaiting: false,
        homePosition: null,
        injector: null,

        preload: function(entityID) {
            _this.entityID = entityID;
            var soundURL = Script.resolvePath(JSON.parse(Entities.getEntityProperties(_this.entityID, ["userData"]).userData).soundFile);
            _this.sound = SoundCache.getSound(soundURL);
            Entities.editEntity(_this.entityID, {dimensions: {x: 0.15182036161422729, y: 0.049085158854722977, z: 0.39702033996582031}});
        },

        collisionWithEntity: function(thisEntity, otherEntity, collision) {
            if (collision.type === 0) {
                _this.hit(otherEntity);                
            }
        },

        clickDownOnEntity: function(otherEntity) {
            _this.hit(otherEntity);
        },

        hit: function(otherEntity) {
            if (!_this.isWaiting) {
                _this.isWaiting = true;
                _this.homePosition = Entities.getEntityProperties(_this.entityID, ["position"]).position;
                _this.injector = Audio.playSound(_this.sound, {position: _this.homePosition, volume: 1});
                _this.editEntityTextures(_this.entityID, "file5", TEXTURE_GRAY);
                
                var HAPTIC_STRENGTH = 1;
                var HAPTIC_DURATION = 20;                
                var userData = JSON.parse(Entities.getEntityProperties(otherEntity, 'userData').userData);
                if (userData.hasOwnProperty('hand')){
                    Controller.triggerHapticPulse(HAPTIC_STRENGTH, HAPTIC_DURATION, userData.hand);
                }

                _this.timeout();
            }
        },

        timeout: function() {
            Script.setTimeout(function() {
                _this.editEntityTextures(_this.entityID, "file5", TEXTURE_BLACK);
                _this.isWaiting = false;
            }, TIMEOUT);
        },

        getEntityTextures: function(id) {
            var results = null;
            var properties = Entities.getEntityProperties(id, "textures");
            if (properties.textures) { 
                try {
                    results = JSON.parse(properties.textures);
                } catch (err) {
                    if (IS_DEBUG) {
                        print(err);
                        print(properties.textures);
                    }
                }
            }
            return results ? results : {};
        },

        setEntityTextures: function(id, textureList) {
            var json = JSON.stringify(textureList);
            Entities.editEntity(id, {textures: json});
        },

        editEntityTextures: function(id, textureName, textureURL) {
            var textureList = _this.getEntityTextures(id);
            textureList[textureName] = textureURL;
            _this.setEntityTextures(id, textureList);
        }
    };

    return new XylophoneKey();
});
