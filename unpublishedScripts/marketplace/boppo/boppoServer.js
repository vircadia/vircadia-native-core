//
//  boppoServer.js
//
//  Created by Thijs Wenker on 3/15/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var CLOWN_LAUGHS = [
        'clown_laugh_1.wav',
        'clown_laugh_2.wav',
        'clown_laugh_3.wav',
        'clown_laugh_4.wav'
    ];
    var TICK_TOCK_SOUND = 'ticktock%20-%20tock.wav';
    var BOXING_RING_BELL_START = 'boxingRingBell.wav';
    var BOXING_RING_BELL_END = 'boxingRingBell-end.wav';
    var BOPPO_MUSIC = 'boppoMusic.wav';
    var CHANNEL_PREFIX = 'io.highfidelity.boppo_server_';
    var MESSAGE_HIT = 'hit';
    var MESSAGE_ENTER_ZONE = 'enter-zone';
    var MESSAGE_UNLOAD_FIX = 'unload-fix';

    var DEFAULT_SOUND_VOLUME = 0.6;

    // don't set the search radius too high, it might remove boppo's from other nearby instances
    var BOPPO_SEARCH_RADIUS = 4.0;

    var MILLISECONDS_PER_SECOND = 1000;
    // Make sure the entities are loaded at startup (TODO: more solid fix)
    var LOAD_TIMEOUT = 5000; 
    var SECONDS_PER_MINUTE = 60;
    var DEFAULT_PLAYTIME = 30; // seconds
    var BASE_TEN = 10;
    var TICK_TOCK_FROM = 3; // seconds
    var COOLDOWN_TIME_MS = MILLISECONDS_PER_SECOND * 3;

    var createBoppoServer = function() {
        var _this,
            _isInitialized = false,
            _clownLaughs = [],
            _musicInjector,
            _music,
            _laughingInjector,
            _tickTockSound,
            _boxingBellRingStart,
            _boxingBellRingEnd,
            _entityID,
            _boppoClownID,
            _channel,
            _boppoEntities,
            _isGameRunning,
            _updateInterval,
            _timeLeft,
            _hits,
            _coolDown;

        var getOwnBoppoUserData = function() {
            try {
                return JSON.parse(Entities.getEntityProperties(_entityID, ['userData']).userData).Boppo;
            } catch (e) {
                // e
            }
            return {};
        };

        var updateBoppoEntities = function() {
            Entities.getChildrenIDs(_entityID).forEach(function(entityID) {
                try {
                    var userData = JSON.parse(Entities.getEntityProperties(entityID, ['userData']).userData);
                    if (userData.Boppo.type !== undefined) {
                        _boppoEntities[userData.Boppo.type] = entityID;
                    }
                } catch (e) {
                    // e
                }
            });
        };

        var clearUntrackedBoppos = function() {
            var position = Entities.getEntityProperties(_entityID, ['position']).position;
            Entities.findEntities(position, BOPPO_SEARCH_RADIUS).forEach(function(entityID) {
                try {
                    if (JSON.parse(Entities.getEntityProperties(entityID, ['userData']).userData).Boppo.type === 'boppo') {
                        Entities.deleteEntity(entityID);
                    }
                } catch (e) {
                    // e
                }
            });
        };

        var updateTimerDisplay = function() {
            if (_boppoEntities['timer']) {
                var secondsString = _timeLeft % SECONDS_PER_MINUTE;
                if (secondsString < BASE_TEN) {
                    secondsString = '0' + secondsString;
                }
                var minutesString = Math.floor(_timeLeft / SECONDS_PER_MINUTE);
                Entities.editEntity(_boppoEntities['timer'], {
                    text: minutesString + ':' + secondsString
                });
            }
        };

        var updateScoreDisplay = function() {
            if (_boppoEntities['score']) {
                Entities.editEntity(_boppoEntities['score'], {
                    text: 'SCORE: ' + _hits
                });
            }
        };

        var playSoundAtBoxingRing = function(sound, properties) {
            var _properties = properties ? properties : {};
            if (_properties['volume'] === undefined) {
                _properties['volume'] = DEFAULT_SOUND_VOLUME;
            }
            _properties['position'] = Entities.getEntityProperties(_entityID, ['position']).position;
            // play beep
            return Audio.playSound(sound, _properties);
        };

        var onUpdate = function() {
            _timeLeft--;

            if (_timeLeft > 0 && _timeLeft <= TICK_TOCK_FROM) {
                // play beep
                playSoundAtBoxingRing(_tickTockSound);
            }
            if (_timeLeft === 0) {
                if (_musicInjector !== undefined && _musicInjector.isPlaying()) {
                    _musicInjector.stop();
                    _musicInjector = undefined;
                }
                playSoundAtBoxingRing(_boxingBellRingEnd);
                _isGameRunning = false;
                Script.clearInterval(_updateInterval);
                _updateInterval = null;
                _coolDown = true;
                Script.setTimeout(function() {
                    _coolDown = false;
                    _this.resetBoppo();
                }, COOLDOWN_TIME_MS);
            }
            updateTimerDisplay();
        };

        var onMessage = function(channel, message, sender) {
            if (channel === _channel) {
                if (message === MESSAGE_HIT) {
                    _this.hit();
                } else if (message === MESSAGE_ENTER_ZONE && !_isGameRunning) {
                    _this.resetBoppo();
                } else if (message === MESSAGE_UNLOAD_FIX && _isInitialized) {
                    _this.unload();
                }
            }
        };

        var BoppoServer = function () {
            _this = this;
            _hits = 0;
            _boppoClownID = null;
            _coolDown = false;
            CLOWN_LAUGHS.forEach(function(clownLaugh) {
                _clownLaughs.push(SoundCache.getSound(Script.getExternalPath(Script.ExternalPaths.HF_Content, + clownLaugh)));
            });
            _tickTockSound = SoundCache.getSound(Script.getExternalPath(Script.ExternalPaths.HF_Content, TICK_TOCK_SOUND));
            _boxingBellRingStart = SoundCache.getSound(Script.getExternalPath(Script.ExternalPaths.HF_Content, BOXING_RING_BELL_START));
            _boxingBellRingEnd = SoundCache.getSound(Script.getExternalPath(Script.ExternalPaths.HF_Content, BOXING_RING_BELL_END));
            _music = SoundCache.getSound(Script.getExternalPath(Script.ExternalPaths.HF_Content, BOPPO_MUSIC));
            _boppoEntities = {};
        };

        BoppoServer.prototype = {
            preload: function(entityID) {
                _entityID = entityID;
                _channel = CHANNEL_PREFIX + entityID;

                Messages.sendLocalMessage(_channel, MESSAGE_UNLOAD_FIX);
                Script.setTimeout(function() {
                    clearUntrackedBoppos();
                    updateBoppoEntities();
                    Messages.subscribe(_channel);
                    Messages.messageReceived.connect(onMessage);
                    _this.resetBoppo();
                    _isInitialized = true;
                }, LOAD_TIMEOUT);
            },
            resetBoppo: function() {
                if (_boppoClownID !== null) {
                    print('deleting boppo: ' + _boppoClownID);
                    Entities.deleteEntity(_boppoClownID);
                }
                var boppoBaseProperties = Entities.getEntityProperties(_entityID, ['position', 'rotation']);
                _boppoClownID = Entities.addEntity({
                    angularDamping: 0.0,
                    collisionSoundURL: Script.getExternalPath(Script.ExternalPaths.HF_Content, 'caitlyn/production/elBoppo/51460__andre-rocha-nascimento__basket-ball-01-bounce.wav'),
                    collisionsWillMove: true,
                    compoundShapeURL: Script.getExternalPath(Script.ExternalPaths.HF_Content, 'caitlyn/production/elBoppo/bopo_phys.obj'),
                    damping: 1.0,
                    density: 10000,
                    dimensions: {
                        x: 1.2668079137802124,
                        y: 2.0568051338195801,
                        z: 0.88563752174377441
                    },
                    dynamic: 1.0,
                    friction: 1.0,
                    gravity: {
                        x: 0,
                        y: -25,
                        z: 0
                    },
                    modelURL: Script.getExternalPath(Script.ExternalPaths.HF_Content, 'caitlyn/production/elBoppo/elBoppo3_VR.fbx'),
                    name: 'El Boppo the Punching Bag Clown',
                    registrationPoint: {
                        x: 0.5,
                        y: 0,
                        z: 0.3
                    },
                    restitution: 0.99,
                    rotation: boppoBaseProperties.rotation,
                    position: Vec3.sum(boppoBaseProperties.position,
                        Vec3.multiplyQbyV(boppoBaseProperties.rotation, {
                            x: 0.08666179329156876,
                            y: -1.5698202848434448,
                            z: 0.1847127377986908
                        })),
                    script: Script.resolvePath('boppoClownEntity.js'),
                    shapeType: 'compound',
                    type: 'Model',
                    userData: JSON.stringify({
                        lookAt: {
                            targetID: _boppoEntities['lookAtThis'],
                            disablePitch: true,
                            disableYaw: false,
                            disableRoll: true,
                            clearDisabledAxis: true,
                            rotationOffset: { x: 0.0, y: 180.0, z: 0.0}
                        },
                        Boppo: {
                            type: 'boppo',
                            gameParentID: _entityID
                        },
                        grabbableKey: {
                            grabbable: false
                        }
                    })
                });
                updateBoppoEntities();
                _boppoEntities['boppo'] = _boppoClownID;
            },
            laugh: function() {
                if (_laughingInjector !== undefined && _laughingInjector.isPlaying()) {
                    return;
                }
                var randomLaughIndex = Math.floor(Math.random() * _clownLaughs.length);
                _laughingInjector = Audio.playSound(_clownLaughs[randomLaughIndex], {
                    position: Entities.getEntityProperties(_boppoClownID, ['position']).position
                });
            },
            hit: function() {
                if (_coolDown) {
                    return;
                }
                if (!_isGameRunning) {
                    var boxingRingBoppoData = getOwnBoppoUserData();
                    _updateInterval = Script.setInterval(onUpdate, MILLISECONDS_PER_SECOND);
                    _timeLeft = boxingRingBoppoData.playTimeSeconds ? parseInt(boxingRingBoppoData.playTimeSeconds) :
                        DEFAULT_PLAYTIME;
                    _isGameRunning = true;
                    _hits = 0;
                    playSoundAtBoxingRing(_boxingBellRingStart);
                    _musicInjector = playSoundAtBoxingRing(_music, {loop: true, volume: 0.6});
                }
                _hits++;
                updateTimerDisplay();
                updateScoreDisplay();
                _this.laugh();
            },
            unload: function() {
                print('unload called');
                if (_updateInterval) {
                    Script.clearInterval(_updateInterval);
                }
                Messages.messageReceived.disconnect(onMessage);
                Messages.unsubscribe(_channel);
                Entities.deleteEntity(_boppoClownID);
                print('endOfUnload');
            }
        };

        return new BoppoServer();
    };

    return createBoppoServer();
});
