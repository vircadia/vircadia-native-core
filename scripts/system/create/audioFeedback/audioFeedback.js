//
//  audioFeedback.js
//
//  Created by Alezia Kurdis on September 30, 2020.
//  Copyright 2020 Vircadia contributors.
//
//  This script add audio feedback (confirmation and rejection) for user interactions that require one.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

audioFeedback = (function() {
    var that = {};

    var confirmationSound = SoundCache.getSound(Script.resolvePath("./sounds/confirmation.mp3"));
    var rejectionSound = SoundCache.getSound(Script.resolvePath("./sounds/rejection.mp3"));
    var actionSound = SoundCache.getSound(Script.resolvePath("./sounds/action.mp3"));
    
    that.confirmation = function() { //Play a confirmation sound
        var injector = Audio.playSound(confirmationSound, {
            "volume": 0.3,
            "localOnly": true
        });
    }

    that.rejection = function() { //Play a rejection sound
        var injector = Audio.playSound(rejectionSound, {
            "volume": 0.3,
            "localOnly": true
        });
    }

    that.action = function() { //Play an action sound
        var injector = Audio.playSound(actionSound, {
            "volume": 0.3,
            "localOnly": true
        });
    }

    return that;
})();
