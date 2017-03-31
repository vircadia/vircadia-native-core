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
	var CLEARCACHE = "?"+Math.random().toString(36).substring(7);
	var GAMES_LIST_ENDPOINT = Script.resolvePath('games/gamesDirectory.svo.json') + CLEARCACHE;

    var _this;
    var INITIAL_DELAY = 1000;

    function GameTable() {
        _this = this;
    }

    GameTable.prototype = {
        matCorner: null,
        currentGameIndex: 0,
        preload: function(entityID) {
            _this.entityID = entityID;
            Script.setTimeout(function() {
                _this.setCurrentGamesList();
            }, INITIAL_DELAY);
        },
        collisionWithEntity: function(me, other, collision) {
            //stick the table to the ground
            if (collision.type !== 1) {
                return;
            }
            var myProps = Entities.getEntityProperties(_this.entityID, ["rotation", "position"]);
            var eulerRotation = Quat.safeEulerAngles(myProps.rotation);
            eulerRotation.x = 0;
            eulerRotation.z = 0;
            var newRotation = Quat.fromVec3Degrees(eulerRotation);

            //we zero out the velocity and angular velocity so the table doesn't change position or spin
            Entities.editEntity(_this.entityID, {
                rotation: newRotation,
                dynamic: true,
                velocity: {
                    x: 0,
                    y: 0,
                    z: 0
                },
                angularVelocity: {
                    x: 0,
                    y: 0,
                    z: 0
                }
            });
        },
        getCurrentGame: function() {
            var userData = _this.getCurrentUserData();
            if (!(userData.gameTableData !== undefined && userData.gameTableData.currentGame !== undefined)) {
                _this.currentGameIndex = -1;
                return;
            }
            var foundIndex = -1;
            _this.gamesList.forEach(function(game, index) {
                if (game.gameName === userData.gameTableData.currentGame) {
                    foundIndex = index;
                }
            });
            _this.currentGameIndex = foundIndex;
        },
        setInitialGameIfNone: function() {
            _this.getCurrentGame();
            if (_this.currentGameIndex === -1) {
                _this.setCurrentGame();
                _this.cleanupGameEntities();
            }
        },
        resetGame: function() {
            // always check the current game before resetting
            _this.getCurrentGame();
            _this.cleanupGameEntities();
        },
        nextGame: function() {
            // always check the current game before switching
            _this.getCurrentGame();
            _this.currentGameIndex = (_this.currentGameIndex + 1) % _this.gamesList.length;
            _this.cleanupGameEntities();
        },
        cleanupGameEntities: function() {
            var props = Entities.getEntityProperties(_this.entityID);
            var results = Entities.findEntities(props.position, 5);
            var found = [];
            results.forEach(function(item) {
                var itemProps = Entities.getEntityProperties(item);
                if (itemProps.description.indexOf('hifi:gameTable:piece:') === 0) {
                    found.push(item);
                }
                if (itemProps.description.indexOf('hifi:gameTable:anchor') > -1) {
                    found.push(item);
                }
            });
            print('deleting ' + found.length + ' matching piece');
            found.forEach(function(foundItem) {
                Entities.deleteEntity(foundItem);
            });
            _this.setCurrentGame();
            _this.spawnEntitiesForGame();
        },
        setCurrentGamesList: function() {
            _this.gamesList = getGamesList();
            _this.setInitialGameIfNone();
        },
        setCurrentGame: function() {
            print('index in set current game: ' + _this.currentGameIndex);
            // print('games list in set current game' + JSON.stringify(_this.gamesList));
            print('game at index' + _this.gamesList[_this.currentGameIndex]);
            _this.currentGame = _this.gamesList[_this.currentGameIndex].gameName;
            _this.currentGameFull = _this.gamesList[_this.currentGameIndex];
            _this.setCurrentUserData({
                currentGame: _this.currentGame
            });
        },
        setCurrentUserData: function(data) {
            var userData = _this.getCurrentUserData();
            userData['gameTableData'] = data;
            var success = Entities.editEntity(_this.entityID, {
                userData: JSON.stringify(userData)
            });
        },
        getCurrentUserData: function() {
            var props = Entities.getEntityProperties(_this.entityID);
            var hasUserData = props.hasOwnProperty('userData');
            var json = {};
            try {
                json = JSON.parse(props.userData);
            } catch (e) {
                print('user data is not json' + props.userData)
            }
            return json;
        },
        getEntityFromGroup: function(groupName, entityName) {
            print('getting entity from group: ' + groupName);
            var props = Entities.getEntityProperties(_this.entityID);
            var results = Entities.findEntities(props.position, 7.5);
            var found;
            var result = null;
            results.forEach(function(item) {
                var itemProps = Entities.getEntityProperties(item);

                var descriptionSplit = itemProps.description.split(":");
                if (descriptionSplit[1] === groupName && descriptionSplit[2] === entityName) {
                    result = item;
                }
            });
            return result
        },
        spawnEntitiesForGame: function() {
            var entitySpawner = _this.getEntityFromGroup('gameTable', 'entitySpawner');
            var mat = _this.getEntityFromGroup('gameTable', 'mat');

            Entities.callEntityMethod(entitySpawner, 'spawnEntities', [JSON.stringify(_this.currentGameFull),mat,
                _this.entityID]);
        }
    };

    function getGamesList() {
        var request = new XMLHttpRequest();
        request.open("GET", GAMES_LIST_ENDPOINT, false);
        request.send();
        return JSON.parse(request.responseText);
    }

    return new GameTable();
});
