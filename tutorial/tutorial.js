//
//  tutorial.js
//
//  Created by Ryan Huffman on 9/1/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("entityData.js");
Script.include("viveHandsv2.js");
Script.include("lighter/createButaneLighter.js");
Script.include('ownershipToken.js');

if (!Function.prototype.bind) {
  Function.prototype.bind = function(oThis) {
    if (typeof this !== 'function') {
      // closest thing possible to the ECMAScript 5
      // internal IsCallable function
      throw new TypeError('Function.prototype.bind - what is trying to be bound is not callable');
    }

    var aArgs   = Array.prototype.slice.call(arguments, 1),
        fToBind = this,
        fNOP    = function() {},
        fBound  = function() {
          return fToBind.apply(this instanceof fNOP
                 ? this
                 : oThis,
                 aArgs.concat(Array.prototype.slice.call(arguments)));
        };

    if (this.prototype) {
      // Function.prototype doesn't have a prototype property
      fNOP.prototype = this.prototype; 
    }
    fBound.prototype = new fNOP();

    return fBound;
  };
}

var DEBUG = false;
function debug() {
    if (DEBUG) {
        print.apply(this, arguments);
    }
}

var INFO = true;
function info() {
    if (INFO) {
        print.apply(this, arguments);
    }
}

var NEAR_BOX_SPAWN_NAME = "tutorial/nearGrab/box_spawn";
var FAR_BOX_SPAWN_NAME = "tutorial/farGrab/box_spawn";
var NEAR_BASKET_COLLIDER_NAME = "tutorial/nearGrab/basket_collider";
var FAR_BASKET_COLLIDER_NAME = "tutorial/farGrab/basket_collider";
var GUN_BASKET_COLLIDER_NAME = "tutorial/equip/basket_collider";
var GUN_SPAWN_NAME = "tutorial/gun_spawn";
var TELEPORT_PAD_NAME = "tutorial/teleport/pad"

var successSound = SoundCache.getSound("atp:/tutorial_sounds/good_one.L.wav");

function beginsWithFilter(value, key) {
    return value.indexOf(properties[key]) == 0;
}

findEntity = function(properties, searchRadius, filterFn) {
    var entities = findEntities(properties, searchRadius, filterFn);
    return entities.length > 0 ? entities[0] : null;
}

// Return all entities with properties `properties` within radius `searchRadius`
findEntities = function(properties, searchRadius, filterFn) {
    if (!filterFn) {
        filterFn = function(properties, key, value) {
            return value == properties[key];
        }
    }
    searchRadius = searchRadius ? searchRadius : 100000;
    var entities = Entities.findEntities({ x: 0, y: 0, z: 0 }, searchRadius);
    var matchedEntities = [];
    var keys = Object.keys(properties);
    for (var i = 0; i < entities.length; ++i) {
        var match = true;
        var candidateProperties = Entities.getEntityProperties(entities[i], keys);
        for (var key in properties) {
            if (!filterFn(properties, key, candidateProperties[key])) {
                // This isn't a match, move to next entity
                match = false;
                break;
            }
        }
        if (match) {
            matchedEntities.push(entities[i]);
        }
    }

    return matchedEntities;
}

function setControllerVisible(name, visible) {
    return;
    Messages.sendLocalMessage('Controller-Display', JSON.stringify({
        name: name,
        visible: visible,
    }));
}

function setControllerPartsVisible(parts) {
    Messages.sendLocalMessage('Controller-Display-Parts', JSON.stringify(parts));
}

function setControllerPartLayer(part, layer) {
    data = {};
    data[part] = layer;
    Messages.sendLocalMessage('Controller-Set-Part-Layer', JSON.stringify(data));
}

function triggerHapticPulse() {
    function scheduleHaptics(delay, strength, duration) {
        Script.setTimeout(function() {
            Controller.triggerHapticPulse(strength, duration, 0);
            Controller.triggerHapticPulse(strength, duration, 1);
        }, delay);
    }
    scheduleHaptics(0, 0.8, 100);
    scheduleHaptics(300, 0.5, 100);
    scheduleHaptics(600, 0.3, 100);
    scheduleHaptics(900, 0.2, 100);
    scheduleHaptics(1200, 0.1, 100);
}

function spawn(entityData, transform, modifyFn) {
    debug("Creating: ", entityData);
    if (!transform) {
        transform = {
            position: { x: 0, y: 0, z: 0 },
            rotation: { x: 0, y: 0, z: 0, w: 1 }
        }
    }
    var ids = [];
    for (var i = 0; i < entityData.length; ++i) {
        var data = entityData[i];
        debug("Creating: ", data.name);
        data.position = Vec3.sum(transform.position, data.position);
        data.rotation = Quat.multiply(data.rotation, transform.rotation);
        if (modifyFn) {
            data = modifyFn(data);
        }
        var id = Entities.addEntity(data);
        ids.push(id);
        debug(id, "data:", JSON.stringify(data));
    }
    return ids;
}

function parseJSON(jsonString) {
    var data;
    try {
        data = JSON.parse(jsonString);
    } catch(e) {
        data = {};
    }
    return data;
}

function spawnWithTag(entityData, transform, tag) {
    function modifyFn(data) {
        var userData = parseJSON(data.userData);
        userData.tag = tag;
        data.userData = JSON.stringify(userData);
        debug("In modify", tag, userData, data.userData);
        return data;
    }
    return spawn(entityData, transform, modifyFn);
}

function deleteEntitiesWithTag(tag) {
    debug("searching for...:", tag);
    var entityIDs = findEntitiesWithTag(tag);
    for (var i = 0; i < entityIDs.length; ++i) {
        Entities.deleteEntity(entityIDs[i]);
    }
}
function editEntitiesWithTag(tag, propertiesOrFn) {
    var entityIDs = findEntitiesWithTag(tag);
    for (var i = 0; i < entityIDs.length; ++i) {
        if (isFunction(propertiesOrFn)) {
            Entities.editEntity(entityIDs[i], propertiesOrFn(entityIDs[i]));
        } else {
            Entities.editEntity(entityIDs[i], propertiesOrFn);
        }
    }
}

function findEntitiesWithTag(tag) {
    return findEntities({ userData: "" }, 10000, function(properties, key, value) {
        data = parseJSON(value);
        return data.tag == tag;
    }); 
}

// From http://stackoverflow.com/questions/5999998/how-can-i-check-if-a-javascript-variable-is-function-type
function isFunction(functionToCheck) {
    var getType = {};
    return functionToCheck && getType.toString.call(functionToCheck) === '[object Function]';
}

        var defaultTransform = {
            position: {
                x: 0.2459,
                y: 0.9011,
                z: 0.7266
            },
            rotation: {
                x: 0,
                y: 0,
                z: 0,
                w: 1
            }
        };
function playSuccessSound() {
    Audio.playSound(successSound, {
        position: MyAvatar.position,
        volume: 0.7,
        loop: false
    });
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// STEP: DISABLE CONTROLLERS                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
var stepDisableControllers = function(name) {
    this.tag = name;
    this.shouldLog = false;
}
stepDisableControllers.prototype = {
    start: function(onFinish) {
        controllerDisplayManager = new ControllerDisplayManager();
        editEntitiesWithTag('door', { visible: true, collisionless: false });
        Menu.setIsOptionChecked("Overlays", false);
        Controller.disableMapping('handControllerPointer-click');
        Messages.sendLocalMessage('Hifi-Advanced-Movement-Disabler', 'disable');
        Messages.sendLocalMessage('Hifi-Teleport-Disabler', 'both');
        Messages.sendLocalMessage('Hifi-Grab-Disable', JSON.stringify({
            nearGrabEnabled: true,
            holdEnabled: false,
            farGrabEnabled: false,
        }));
        setControllerPartLayer('touchpad', 'blank');
        setControllerPartLayer('tips', 'blank');

        hideEntitiesWithTag('finish');

        onFinish();
    },
    cleanup: function() {
    }
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// STEP: ENABLE CONTROLLERS                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
function reenableEverything() {
    editEntitiesWithTag('door', { visible: false, collisionless: true });
    Menu.setIsOptionChecked("Overlays", true);
    Controller.enableMapping('handControllerPointer-click');
    Messages.sendLocalMessage('Hifi-Advanced-Movement-Disabler', 'enable');
    Messages.sendLocalMessage('Hifi-Teleport-Disabler', 'none');
    Messages.sendLocalMessage('Hifi-Grab-Disable', JSON.stringify({
        nearGrabEnabled: true,
        holdEnabled: true,
        farGrabEnabled: true,
    }));
    setControllerPartLayer('touchpad', 'blank');
    setControllerPartLayer('tips', 'blank');
    MyAvatar.shouldRenderLocally = true;
    if (controllerDisplayManager) {
        controllerDisplayManager.destroy();
        controllerDisplayManager = null;
    }
}

var stepEnableControllers = function(name) {
    this.tag = name;
    this.shouldLog = false;
}
stepEnableControllers.prototype = {
    start: function(onFinish) {
        reenableEverything();
        onFinish();
    },
    cleanup: function() {
    }
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// STEP: Welcome                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
var stepWelcome = function(name) {
    this.tag = name;
}
stepWelcome.prototype = {
    start: function(onFinish) {
        this.timerID = Script.setTimeout(onFinish, 8000);
        showEntitiesWithTag(this.tag);
    },
    cleanup: function() {
        if (this.timerID) {
            Script.clearTimeout(this.timerID);
            this.timerID = null;
        }
        hideEntitiesWithTag(this.tag);
    }
};

function StayInFrontOverlay(type, properties, distance, positionOffset) {
    this.currentOrientation = MyAvatar.orientation;
    this.currentPosition = MyAvatar.position;
    this.distance = distance;
    this.positionOffset = positionOffset;

    var forward = Vec3.multiply(this.distance, Quat.getFront(this.currentOrientation));

    properties.rotation = this.currentOrientation;
    properties.position = Vec3.sum(Vec3.sum(this.currentPosition, forward), this.positionOffset);
    this.overlayID = Overlays.addOverlay(type, properties);


    this.distance = distance;

    this.boundUpdate = this.update.bind(this);
    Script.update.connect(this.boundUpdate);
}
StayInFrontOverlay.prototype = {
    update: function(dt) {
        var targetOrientation = MyAvatar.orientation;
        var targetPosition = MyAvatar.position;
        this.currentOrientation = Quat.slerp(this.currentOrientation, targetOrientation, 0.05);
        this.currentPosition = Vec3.mix(this.currentPosition, targetPosition, 0.05);

        var forward = Vec3.multiply(this.distance, Quat.getFront(this.currentOrientation));
        Overlays.editOverlay(this.overlayID, {
            position: Vec3.sum(Vec3.sum(this.currentPosition, forward), this.positionOffset),
            rotation: this.currentOrientation,
        });
    },
    destroy: function() {
        Overlays.deleteOverlay(this.overlayID);
        Script.update.disconnect(this.boundUpdate);
    }
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// STEP: Orient and raise hands above head                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
var stepOrient = function(name) {
    this.tag = name;
    this.tempTag = name + "-temporary";
}
stepOrient.prototype = {
    start: function(onFinish) {
        this.active = true;

        var tag = this.tag;

        var defaultTransform = {
            position: {
                x: 0.2459,
                y: 0.9011,
                z: 0.7266
            },
            rotation: {
                x: 0,
                y: 0,
                z: 0,
                w: 1
            }
        };

        // Spawn content set
        debug("raise hands...", this.tag);
        editEntitiesWithTag(this.tag, { visible: true });


        this.checkIntervalID = null;
        function checkForHandsAboveHead() {
            debug("Orient: Checking for hands above head...");
            if (MyAvatar.getLeftPalmPosition().y > (MyAvatar.getHeadPosition().y + 0.1)) {
                Script.clearInterval(this.checkIntervalID);
                this.checkIntervalID = null;
                location = "/tutorial";
                Script.setTimeout(playSuccessSound, 150);
                this.active = false;
                onFinish();
            }
        }
        this.checkIntervalID = Script.setInterval(checkForHandsAboveHead.bind(this), 500);
    },
    cleanup: function() {
        if (this.active) {
            //location = "/tutorial";
            this.active = false;
        }
        if (this.overlay) {
            this.overlay.destroy();
            this.overlay = null;
        }
        if (this.checkIntervalID) {
            Script.clearInterval(this.checkIntervalID);
            this.checkIntervalID = null;
        }
        editEntitiesWithTag(this.tag, { visible: false, collisionless: 1 });
        deleteEntitiesWithTag(this.tempTag);
    }
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// STEP: Raise hands above head                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
var stepRaiseAboveHead = function(name) {
    this.tag = name;
    this.tempTag = name + "-temporary";
}
stepRaiseAboveHead.prototype = {
    start: function(onFinish) {
        var tag = this.tag;

        var STATE_START = 0;
        var STATE_HANDS_DOWN = 1;
        var STATE_HANDS_UP = 2;
        this.state = STATE_START;

        var defaultTransform = {
            position: {
                x: 0.2459,
                y: 0.9011,
                z: 0.7266
            },
            rotation: {
                x: 0,
                y: 0,
                z: 0,
                w: 1
            }
        };

        debug("raise hands...", this.tag);
        editEntitiesWithTag(this.tag, { visible: true });

        // Wait 2 seconds before starting to check for hands
        this.checkIntervalID = null;
        function checkForHandsAboveHead() {
            debug("Raise above head: Checking hands...");
            if (this.state == STATE_START) {
                if (MyAvatar.getLeftPalmPosition().y < (MyAvatar.getHeadPosition().y - 0.1)) {
                    this.state = STATE_HANDS_DOWN;
                }
            } else if (this.state == STATE_HANDS_DOWN) {
                if (MyAvatar.getLeftPalmPosition().y > (MyAvatar.getHeadPosition().y + 0.1)) {
                    this.state = STATE_HANDS_UP;
                    Script.clearInterval(this.checkIntervalID);
                    this.checkIntervalID = null;
                    playSuccessSound();
                    onFinish();
                }
            }
        }
        this.checkIntervalID = Script.setInterval(checkForHandsAboveHead.bind(this), 500);
    },
    cleanup: function() {
        if (this.checkIntervalID) {
            Script.clearInterval(this.checkIntervalID);
            this.checkIntervalID = null
        }
        if (this.waitTimeoutID) {
            Script.clearTimeout(this.waitTimeoutID);
            this.waitTimeoutID = null;
        }
        editEntitiesWithTag(this.tag, { visible: false, collisionless: 1 });
        deleteEntitiesWithTag(this.tempTag);
    }
};


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// STEP: Near Grab                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
var stepNearGrab = function(name) {
    this.tag = name;
    this.tempTag = name + "-temporary";
    this.birdIDs = [];

    Messages.subscribe("Entity-Exploded");
    Messages.messageReceived.connect(this.onMessage.bind(this));
}
stepNearGrab.prototype = {
    start: function(onFinish) {
        this.finished = false;
        this.onFinish = onFinish;

        setControllerVisible("trigger", true);
        setControllerPartLayer('tips', 'trigger');
        var tag = this.tag;

        // Spawn content set
        showEntitiesWithTag(this.tag, { visible: true });
        showEntitiesWithTag('bothGrab', { visible: true });

        var boxSpawnID = findEntity({ name: NEAR_BOX_SPAWN_NAME }, 10000);
        if (!boxSpawnID) {
            info("Error creating block, cannot find spawn");
            return null;
        }
        var boxSpawnPosition = Entities.getEntityProperties(boxSpawnID, 'position').position;
        function createBlock() {
            //Step1BlockData.position = boxSpawnPosition;
            birdFirework1.position = boxSpawnPosition;
            return spawnWithTag([birdFirework1], null, this.tempTag)[0];
        }

        // Enabled grab
        // Create table ?
        // Create blocks and basket
        this.birdIDs = [];
        this.birdIDs.push(createBlock.bind(this)());
        this.birdIDs.push(createBlock.bind(this)());
        this.birdIDs.push(createBlock.bind(this)());
        this.positionWatcher = new PositionWatcher(this.birdIDs, boxSpawnPosition, -0.4, 4);

        // If block gets too far away or hasn't been touched for X seconds, create a new block and destroy the old block
    },
    onMessage: function(channel, message, seneder) {
        if (this.finished) {
            return;
        }
        if (channel == "Entity-Exploded") {
            debug("TUTORIAL: Got entity-exploded message");

            var data = parseJSON(message);
            if (this.birdIDs.indexOf(data.entityID) >= 0) {
                playSuccessSound();
                this.finished = true;
                this.onFinish();
            }
        }
    },
    cleanup: function() {
        debug("cleaning up near grab");
        this.finished = true;
        setControllerVisible("trigger", false);
        setControllerPartLayer('tips', 'blank');
        hideEntitiesWithTag(this.tag, { visible: false});
        deleteEntitiesWithTag(this.tempTag);
        if (this.positionWatcher) {
            this.positionWatcher.destroy();
            this.positionWatcher = null;
        }
    }
};



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// STEP: Far Grab                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
var stepFarGrab = function(name) {
    this.tag = name;
    this.tempTag = name + "-temporary";
    this.finished = true;
    this.birdIDs = [];

    Messages.subscribe("Entity-Exploded");
    Messages.messageReceived.connect(this.onMessage.bind(this));
}
stepFarGrab.prototype = {
    start: function(onFinish) {
        this.finished = false;
        this.onFinish = onFinish;

        showEntitiesWithTag('bothGrab', { visible: true });

        setControllerVisible("trigger", true);
        setControllerPartLayer('tips', 'trigger');
        Messages.sendLocalMessage('Hifi-Grab-Disable', JSON.stringify({
            farGrabEnabled: true,
        }));
        var tag = this.tag;

        // Spawn content set
        showEntitiesWithTag(this.tag);

        var boxSpawnID = findEntity({ name: FAR_BOX_SPAWN_NAME }, 10000);
        if (!boxSpawnID) {
            debug("Error creating block, cannot find spawn");
            return null;
        }
        var boxSpawnPosition = Entities.getEntityProperties(boxSpawnID, 'position').position;
        function createBlock() {
            birdFirework1.position = boxSpawnPosition;
            return spawnWithTag([birdFirework1], null, this.tempTag)[0];
        }

        this.birdIDs = [];
        this.birdIDs.push(createBlock.bind(this)());
        this.birdIDs.push(createBlock.bind(this)());
        this.birdIDs.push(createBlock.bind(this)());
        this.positionWatcher = new PositionWatcher(this.birdIDs, boxSpawnPosition, -0.4, 4);
    },
    onMessage: function(channel, message, seneder) {
        if (this.finished) {
            return;
        }
        if (channel == "Entity-Exploded") {
            debug("TUTORIAL: Got entity-exploded message");
            var data = parseJSON(message);
            if (this.birdIDs.indexOf(data.entityID) >= 0) {
                playSuccessSound();
                this.finished = true;
                this.onFinish();
            }
        }
    },
    cleanup: function() {
        this.finished = true;
        setControllerVisible("trigger", false);
        setControllerPartLayer('tips', 'blank');
        hideEntitiesWithTag(this.tag, { visible: false});
        hideEntitiesWithTag('bothGrab', { visible: false});
        deleteEntitiesWithTag(this.tempTag);
        if (this.positionWatcher) {
            this.positionWatcher.destroy();
            this.positionWatcher = null;
        }
    }
};

function PositionWatcher(entityIDs, originalPosition, minY, maxDistance) {
    this.watcherIntervalID = Script.setInterval(function() {
        for (var i = 0; i < entityIDs.length; ++i) {
            var entityID = entityIDs[i];
            var props = Entities.getEntityProperties(entityID, ['position']);
            if (props.position.y < minY || Vec3.distance(originalPosition, props.position) > maxDistance) {
                Entities.editEntity(entityID, { 
                    position: originalPosition,
                    velocity: { x: 0, y: -0.01, z: 0 },
                    angularVelocity: { x: 0, y: 0, z: 0 }
                });
            }
        }
    }, 1000);
}

PositionWatcher.prototype = {
    destroy: function() {
        Script.clearInterval(this.watcherIntervalID);
    }
};



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// STEP: Equip                                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
var stepEquip = function(name) {
    this.tag = name;
    this.tagPart1 = name + "-part1";
    this.tagPart2 = name + "-part2";
    this.tempTag = name + "-temporary";
    this.PART1 = 0;
    this.PART2 = 1;
    this.PART3 = 2;
    this.COMPLETE = 3;

    Messages.subscribe('Tutorial-Spinner');
    Messages.messageReceived.connect(this.onMessage.bind(this));
}
stepEquip.prototype = {
    start: function(onFinish) {
        setControllerVisible("trigger", true);
        setControllerPartLayer('tips', 'trigger');
        Messages.sendLocalMessage('Hifi-Grab-Disable', JSON.stringify({
            holdEnabled: true,
        }));

        var tag = this.tag;

        // Spawn content set
        showEntitiesWithTag(this.tag);
        showEntitiesWithTag(this.tagPart1);

        this.currentPart = this.PART1;

        function createGun() {
            var boxSpawnID = findEntity({ name: GUN_SPAWN_NAME }, 10000);
            if (!boxSpawnID) {
                info("Error creating block, cannot find spawn");
                return null;
            }

            var transform = {};

            transform.position = Entities.getEntityProperties(boxSpawnID, 'position').position;
            transform.rotation = Entities.getEntityProperties(boxSpawnID, 'rotation').rotation;
            this.spawnTransform = transform;
            return doCreateButaneLighter(transform).id;
        }


        this.gunID = createGun.bind(this)();
        this.startWatchingGun();
        debug("Created", this.gunID);
        this.onFinish = onFinish;
    },
    startWatchingGun: function() {
        if (!this.watcherIntervalID) {
            this.watcherIntervalID = Script.setInterval(function() {
                var props = Entities.getEntityProperties(this.gunID, ['position']);
                if (props.position.y < -0.4 
                        || Vec3.distance(this.spawnTransform.position, props.position) > 4) {
                    Entities.editEntity(this.gunID, this.spawnTransform);
                }
            }.bind(this), 1000);
        }
    },
    stopWatchingGun: function() {
        if (this.watcherIntervalID) {
            Script.clearInterval(this.watcherIntervalID);
            this.watcherIntervalID = null;
        }
    },
    onMessage: function(channel, message, sender) {
        if (this.currentPart == this.COMPLETE) {
            return;
        }

        debug("Got message", channel, message, sender, MyAvatar.sessionUUID);

        if (channel == "Tutorial-Spinner") {
            if (this.currentPart == this.PART1 && message == "wasLit") {
                this.currentPart = this.PART2;
                Script.setTimeout(function() {
                    this.currentPart = this.PART3;
                    hideEntitiesWithTag(this.tagPart1);
                    showEntitiesWithTag(this.tagPart2);
                    setControllerPartLayer('tips', 'grip');
                    Messages.subscribe('Hifi-Object-Manipulation');
                }.bind(this), 9000);
            }
        } else if (channel == "Hifi-Object-Manipulation") {
            if (this.currentPart == this.PART3) {
                var data = parseJSON(message);
                if (data.action == 'release' && data.grabbedEntity == this.gunID) {
                    info("got release");
                    this.stopWatchingGun();
                    this.currentPart = this.COMPLETE;
                    playSuccessSound();
                    Script.setTimeout(this.onFinish.bind(this), 1500);
                }
            }
        }
    },
    cleanup: function() {
        if (this.watcherIntervalID) {
            Script.clearInterval(this.watcherIntervalID);
            this.watcherIntervalID = null;
        }

        setControllerVisible("trigger", false);
        setControllerPartLayer('tips', 'blank');
        this.stopWatchingGun();
        this.currentPart = this.COMPLETE;

        if (this.checkCollidesTimer) {
            Script.clearInterval(this.checkCollidesTimer);
        }
        hideEntitiesWithTag(this.tagPart1);
        hideEntitiesWithTag(this.tagPart2);
        hideEntitiesWithTag(this.tag);
        deleteEntitiesWithTag(this.tempTag);
    }
};




///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// STEP: Turn Around                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
var stepTurnAround = function(name) {
    this.tag = name;
    this.tempTag = name + "-temporary";


    //var name = "mapping-name";
    //var mapping = Controller.newMapping(name);
    //mapping.from([Controller.Actions.StepYaw]).to(function() {
    //    print("STEPYAW");
    //});
    //Script.scriptEnding.connect(function() {
    //    Controller.disableMapping(name);
    //});
}
stepTurnAround.prototype = {
    start: function(onFinish) {
        setControllerVisible("left", true);
        setControllerVisible("right", true);

        setControllerPartLayer('touchpad', 'arrows');
        setControllerPartLayer('tips', 'arrows');

        showEntitiesWithTag(this.tag);
        var hasTurnedAround = false;
        this.interval = Script.setInterval(function() {
            var dir = Quat.getFront(MyAvatar.orientation);
            var angle = Math.atan2(dir.z, dir.x);
            var angleDegrees = ((angle / Math.PI) * 180);
            if (!hasTurnedAround) {
                if (Math.abs(angleDegrees) > 140) {
                    hasTurnedAround = true;
                    info("Half way turned around");
                }
            } else {
                if (Math.abs(angleDegrees) < 30) {
                    Script.clearInterval(this.interval);
                    this.interval = null;
                    info("Turned around");
                    playSuccessSound();
                    onFinish();
                }
            }
        }.bind(this), 100);
    },
    cleanup: function() {
        setControllerVisible("left", false);
        setControllerVisible("right", false);

        setControllerPartLayer('touchpad', 'blank');
        setControllerPartLayer('tips', 'blank');

        if (this.interval) {
            Script.clearInterval(this.interval);
        }
        hideEntitiesWithTag(this.tag);
        deleteEntitiesWithTag(this.tempTag);
    }
};




///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// STEP: Teleport                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
var stepTeleport = function(name) {
    this.tag = name;
    this.tempTag = name + "-temporary";
}
stepTeleport.prototype = {
    start: function(onFinish) {
        //setControllerVisible("teleport", true);

        setControllerPartLayer('touchpad', 'teleport');
        setControllerPartLayer('tips', 'teleport');

        Messages.sendLocalMessage('Hifi-Teleport-Disabler', 'none');

        // Wait until touching teleport pad...
        var padID = findEntity({ name: TELEPORT_PAD_NAME }, 100);
        var padProps = Entities.getEntityProperties(padID, ["position", "dimensions"]);
        var xMin = padProps.position.x - padProps.dimensions.x / 2;
        var xMax = padProps.position.x + padProps.dimensions.x / 2;
        var zMin = padProps.position.z - padProps.dimensions.z / 2;
        var zMax = padProps.position.z + padProps.dimensions.z / 2;
        function checkCollides() {
            debug("Checking if on pad...");

            var pos = MyAvatar.position;

            debug('x', pos.x, xMin, xMax);
            debug('z', pos.z, zMin, zMax);

            if (pos.x > xMin && pos.x < xMax && pos.z > zMin && pos.z < zMax) {
                debug("On teleport pad");
                Script.clearInterval(this.checkCollidesTimer);
                this.checkCollidesTimer = null;
                playSuccessSound();
                onFinish();
            }
        }
        this.checkCollidesTimer = Script.setInterval(checkCollides.bind(this), 500);

        showEntitiesWithTag(this.tag);
    },
    cleanup: function() {
        setControllerPartLayer('touchpad', 'blank');
        setControllerPartLayer('tips', 'blank');

        if (this.checkCollidesTimer) {
            Script.clearInterval(this.checkCollidesTimer);
        }
        hideEntitiesWithTag(this.tag);
        deleteEntitiesWithTag(this.tempTag);
    }
};





///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// STEP: Finish                                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
var stepFinish = function(name) {
    this.tag = name;
    this.tempTag = name + "-temporary";
}
stepFinish.prototype = {
    start: function(onFinish) {
        editEntitiesWithTag('door', { visible: false, collisonless: true });
        showEntitiesWithTag(this.tag);
        Settings.setValue("tutorialComplete", true);
        onFinish();
    },
    cleanup: function() {
        //Menu.setIsOptionChecked("Overlays", true);
        //hideEntitiesWithTag(this.tag);
        //deleteEntitiesWithTag(this.tempTag);
    }
};

var stepCleanupFinish = function() {
    this.shouldLog = false;
}
stepCleanupFinish.prototype = {
    start: function(onFinish) {
        hideEntitiesWithTag('finish');
        onFinish();
    },
    cleanup: function() {
    }
};



function showEntitiesWithTag(tag) {
    editEntitiesWithTag(tag, function(entityID) {
        var userData = Entities.getEntityProperties(entityID, "userData").userData;
        var data = parseJSON(userData);
        var collisionless = data.visible === false ? true : false;
        if (data.collidable !== undefined) {
            collisionless = data.collidable === true ? false : true;
        }
        if (data.soundKey) {
            debug("Setting sound key to true");
            data.soundKey.playing = true;
        }
        var newProperties = {
            visible: data.visible == false ? false : true,
            collisionless: collisionless,
            userData: JSON.stringify(data),
        };
        Entities.editEntity(entityID, newProperties);
    });
}
function hideEntitiesWithTag(tag) {
    editEntitiesWithTag(tag, function(entityID) {
        var userData = Entities.getEntityProperties(entityID, "userData").userData;
        var data = parseJSON(userData);
        if (data.soundKey) {
            data.soundKey.playing = false;
        }
        var newProperties = {
            visible: false,
            collisionless: 1,
            ignoreForCollisions: 1,
            userData: JSON.stringify(data),
        };
        Entities.editEntity(entityID, newProperties);
    });
}


TutorialManager = function() {
    var STEPS;

    var currentStepNum = -1;
    var currentStep = null;
    var startedTutorialAt = 0;
    var startedLastStepAt = 0;

    var self = this;

    this.startTutorial = function() {
        currentStepNum = -1;
        currentStep = null;
        startedTutorialAt = Date.now();
        STEPS = [
            //new stepCleanupFinish("finish");
            new stepDisableControllers("step0"),
            new stepOrient("orient"),
            //new stepWelcome("welcome"),
            new stepRaiseAboveHead("raiseHands"),
            new stepNearGrab("nearGrab"),
            new stepFarGrab("farGrab"),
            new stepEquip("equip"),
            new stepTurnAround("turnAround"),
            new stepTeleport("teleport"),
            new stepFinish("finish"),
            new stepEnableControllers("enableControllers"),
        ];
        for (var i = 0; i < STEPS.length; ++i) {
            STEPS[i].cleanup();
        }
        MyAvatar.shouldRenderLocally = false;
        this.startNextStep();
    }

    this.onFinish = function() {
        if (currentStep && currentStep.shouldLog !== false) {
            var timeToFinishStep = (Date.now() - startedLastStepAt) / 1000;
            var tutorialTimeElapsed = (Date.now() - startedTutorialAt) / 1000;
            UserActivityLogger.tutorialProgress(
                    currentStep.tag, currentStepNum, timeToFinishStep, tutorialTimeElapsed);
        }

        self.startNextStep();
    }

    this.startNextStep = function() {
        if (currentStep) {
            currentStep.cleanup();
        }

        ++currentStepNum;

        if (currentStepNum >= STEPS.length) {
            // Done
            info("DONE WITH TUTORIAL");
            currentStepNum = -1;
            currentStep = null;
            return false;
        } else {
            info("Starting step", currentStepNum);
            currentStep = STEPS[currentStepNum];
            startedLastStepAt = Date.now();
            currentStep.start(this.onFinish);
            return true;
        }
    }.bind(this);
    this.restartStep = function() {
        if (currentStep) {
            currentStep.cleanup();
            currentStep.start(this.onFinish);
        }
    }

    this.stopTutorial = function() {
        if (currentStep) {
            currentStep.cleanup();
        }
        reenableEverything();
        currentStepNum = -1;
        currentStep = null;
    }
}


var tutorialManager = new TutorialManager();
tutorialManager.startTutorial();
