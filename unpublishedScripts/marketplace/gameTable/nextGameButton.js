//
//  Created by Thijs Wenker on 3/31/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Revision of James B. Pollack's work on GamesTable in 2016
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var _this;
    var CLICK_SOUND_URL = Script.resolvePath('assets/sfx/woodenTapClick.wav');
    var clickSound;

    function NextGameButton() {
        _this = this;
    }

    NextGameButton.prototype = {
        preload: function(id) {
            _this.entityID = id;
            clickSound = SoundCache.getSound(CLICK_SOUND_URL);
        },
        clickDownOnEntity: function() {
            _this.nextGame();
        },
        startNearTrigger: function() {
            _this.nextGame();
        },
        nextGame: function() {
            var buttonProperties = Entities.getEntityProperties(_this.entityID, ['position', 'parentID']);
            Entities.callEntityMethod(buttonProperties.parentID, 'nextGame');
            Audio.playSound(clickSound, {
                loop: false,
                position: buttonProperties.position,
                volume: 0.4
            });
        }
    };
    return new NextGameButton();
});
