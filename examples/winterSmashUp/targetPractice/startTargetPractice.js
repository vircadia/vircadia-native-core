//
//  startTargetPractice.js
//  examples/winterSmashUp/targetPractice
//
//  Created by Thijs Wenker on 12/21/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  The Winter Smash Up Target Practice Game using a bow.
//  This script starts the game, when the entity that contains the script gets shot.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var _this = this;
    var waitForScriptLoad = false;

    const MAX_GAME_TIME = 60; //seconds
    const SCRIPT_URL = 'http://s3.amazonaws.com/hifi-public/scripts/winterSmashUp/targetPractice/targetPracticeGame.js';
    const GAME_CHANNEL = 'winterSmashUpGame';

    var isScriptRunning = function(script) {
        script = script.toLowerCase().trim();
        var runningScripts = ScriptDiscoveryService.getRunning();
        for (i in runningScripts) {
            if (runningScripts[i].url.toLowerCase().trim() == script) {
                return true;
            }
        }
        return false;
    };

    var sendStartSignal = function() {
        Messages.sendMessage(GAME_CHANNEL, JSON.stringify({
            action: 'start',
            gameEntityID: _this.entityID,
            playerSessionUUID: MyAvatar.sessionUUID
        }));
    }

    var startGame = function() {
        //TODO: check here if someone is already playing this game instance by verifying the userData for playerSessionID
        // and startTime with a maximum timeout of X seconds (30?)


        if (!isScriptRunning(SCRIPT_URL)) {
            // Loads the script for the player if this isn't yet loaded
            Script.load(SCRIPT_URL);
            waitForScriptLoad = true;
            return;
        }
        sendStartSignal();
    };

    Messages.messageReceived.connect(function (channel, message, senderID) {
        if (channel == GAME_CHANNEL) {
            var data = JSON.parse(message);
            switch (data.action) {
                case 'scriptLoaded':
                    if (waitForPing) {
                        sendStartSignal();
                        waitForPing = false;
                    }
                    break;
            }
        }
    });

    _this.preload = function(entityID) {
        _this.entityID = entityID;
    };

    _this.collisionWithEntity = function(entityA, entityB, collisionInfo) {
        if (entityA == _this.entityID) {
            try {
                var data = JSON.parse(Entities.getEntityProperties(entityB).userData);
                if (data.creatorSessionUUID === MyAvatar.sessionUUID) {
                    print('attempting to startGame by collisionWithEntity.');
                    startGame();
                }
            } catch(e) {
            }
        }
    };

    _this.onShot = function(forceDirection) {
        print('attempting to startGame by onShot.');
        startGame();
    };

    Messages.subscribe(GAME_CHANNEL);
});
