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
    var myLocation;

    function NextGameButton() {
        _this = this;
    }

    NextGameButton.prototype = {
        preload: function(id) {
            _this.entityID = id;
            clickSound = SoundCache.getSound(CLICK_SOUND_URL);
            myLocation = Entities.getEntityProperties(_this.entityID).position;
        },
        getEntityFromGroup: function(groupName, entityName) {
            var props = Entities.getEntityProperties(_this.entityID);
            var results = Entities.findEntities(props.position, 7.5);
            var found = false;
            results.forEach(function(item) {
                var itemProps = Entities.getEntityProperties(item);
                var descriptionSplit = itemProps.description.split(":");
                if (descriptionSplit[1] === groupName && descriptionSplit[2] === entityName) {
                    found = item
                }
            });
            return found;
        },
        clickDownOnEntity: function() {
            _this.nextGame();
        },
        startNearTrigger: function() {
            _this.nextGame();
        },
        startFarTrigger: function() {},
        nextGame: function() {
            Audio.playSound(CLICK_SOUND_URL, { loop: false, position: myLocation, volume: 0.4 });
            var table = _this.getEntityFromGroup('gameTable', 'table');
            var tableString = table.substr(1, table.length - 2);
            Entities.callEntityMethod(tableString, 'nextGame');
        }
    };
    return new NextGameButton();
});
