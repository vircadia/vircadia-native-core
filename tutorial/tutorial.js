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
Script.include("lighter/createButaneLighter.js");
Script.include("tutorialEntityIDs.js");

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

var DEBUG = true;
function debug() {
    if (DEBUG) {
        var args = Array.prototype.slice.call(arguments);
        args.unshift("tutorial.js | ");
        print.apply(this, args);
    }
}

var INFO = true;
function info() {
    if (INFO) {
        var args = Array.prototype.slice.call(arguments);
        args.unshift("tutorial.js | ");
        print.apply(this, args);
    }
}

var NEAR_BOX_SPAWN_NAME = "tutorial/nearGrab/box_spawn";
var FAR_BOX_SPAWN_NAME = "tutorial/farGrab/box_spawn";
var GUN_SPAWN_NAME = "tutorial/gun_spawn";
var TELEPORT_PAD_NAME = "tutorial/teleport/pad"

var successSound = SoundCache.getSound("atp:/tutorial_sounds/good_one.L.wav");
var firecrackerSound = SoundCache.getSound("atp:/tutorial_sounds/Pops_Firecracker.wav");


var CHANNEL_AWAY_ENABLE = "Hifi-Away-Enable";
function setAwayEnabled(value) {
    var message = value ? 'enable' : 'disable';
    Messages.sendLocalMessage(CHANNEL_AWAY_ENABLE, message);
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

function findEntitiesWithTag(tag) {
    return findEntities({ userData: "" }, 10000, function(properties, key, value) {
        data = parseJSON(value);
        return data.tag === tag;
    });
}

/**
 * A controller in made up of parts, and each part can have multiple "layers,"
 * which are really just different texures. For example, the "trigger" part
 * has "normal" and "highlight" layers.
 */
function setControllerPartLayer(part, layer) {
    data = {};
    data[part] = layer;
    Messages.sendLocalMessage('Controller-Set-Part-Layer', JSON.stringify(data));
}

/**
 * @param {object[]} entityPropertiesList A list of properties to 
 */
function spawn(entityPropertiesList, transform, modifyFn) {
    debug("Creating: ", entityPropertiesList);
    if (!transform) {
        transform = {
            position: { x: 0, y: 0, z: 0 },
            rotation: { x: 0, y: 0, z: 0, w: 1 }
        }
    }
    var ids = [];
    for (var i = 0; i < entityPropertiesList.length; ++i) {
        var data = entityPropertiesList[i];
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

/**
 * @function parseJSON
 * @param {string} jsonString The string to parse.
 * @return {object} Return an empty if the string was not valid JSON, otherwise
 *   the parsed object is returned.
 */
function parseJSON(jsonString) {
    var data;
    try {
        data = JSON.parse(jsonString);
    } catch(e) {
        data = {};
    }
    return data;
}

/**
 * Spawn entities with `tag` in the userData.
 * @function spawnWithTag
 */
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

/**
 * Delete all entities with the tag `tag` in their userData.
 * @function deleteEntitiesWithTag
 */
function deleteEntitiesWithTag(tag) {
    debug("searching for...:", tag);
    var entityIDs = findEntitiesWithTag(tag);
    for (var i = 0; i < entityIDs.length; ++i) {
        Entities.deleteEntity(entityIDs[i]);
    }
}

function editEntitiesWithTag(tag, propertiesOrFn) {
    var entities = TUTORIAL_TAG_TO_ENTITY_IDS_MAP[tag];

    debug("Editing tag: ", tag);
    if (entities) {
        for (entityID in entities) {
            debug("Editing: ", entityID, ", ", propertiesOrFn, ", Is in local tree: ", isEntityInLocalTree(entityID));
            if (isFunction(propertiesOrFn)) {
                Entities.editEntity(entityID, propertiesOrFn(entityIDs[i]));
            } else {
                Entities.editEntity(entityID, propertiesOrFn);
            }
        }
    }
}

// From http://stackoverflow.com/questions/5999998/how-can-i-check-if-a-javascript-variable-is-function-type
function isFunction(functionToCheck) {
    var getType = {};
    return functionToCheck && getType.toString.call(functionToCheck) === '[object Function]';
}

/**
 * Return `true` if `entityID` can be found in the local entity tree, otherwise `false`.
 */
function isEntityInLocalTree(entityID) {
    return Entities.getEntityProperties(entityID, 'visible').visible !== undefined;
}

/**
 *
 */
function showEntitiesWithTags(tags) {
    for (var i = 0; i < tags.length; ++i) {
        showEntitiesWithTag(tags[i]);
    }
}

function showEntitiesWithTag(tag) {
    var entities = TUTORIAL_TAG_TO_ENTITY_IDS_MAP[tag];
    if (entities) {
        for (entityID in entities) {
            var data = entities[entityID];

            var collisionless = data.visible === false ? true : false;
            if (data.collidable !== undefined) {
                collisionless = data.collidable === true ? false : true;
            }
            if (data.soundKey) {
                data.soundKey.playing = true;
            }
            var newProperties = {
                visible: data.visible == false ? false : true,
                collisionless: collisionless,
                userData: JSON.stringify(data),
            };
            debug("Showing: ", entityID, ", Is in local tree: ", isEntityInLocalTree(entityID));
            Entities.editEntity(entityID, newProperties);
        }
    } else {
        debug("ERROR | No entities for tag: ", tag);
    }

    // Dynamic method, suppressed for now
    return;
    editEntitiesWithTag(tag, function(entityID) {
        var userData = Entities.getEntityProperties(entityID, "userData").userData;
        var data = parseJSON(userData);
        var collisionless = data.visible === false ? true : false;
        if (data.collidable !== undefined) {
            collisionless = data.collidable === true ? false : true;
        }
        if (data.soundKey) {
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

function hideEntitiesWithTags(tags) {
    for (var i = 0; i < tags.length; ++i) {
        hideEntitiesWithTag(tags[i]);
    }
}

function hideEntitiesWithTag(tag) {
    var entities = TUTORIAL_TAG_TO_ENTITY_IDS_MAP[tag];
    if (entities) {
        for (entityID in entities) {
            var data = entities[entityID];

            if (data.soundKey) {
                data.soundKey.playing = false;
            }
            var newProperties = {
                visible: false,
                collisionless: 1,
                ignoreForCollisions: 1,
                userData: JSON.stringify(data),
            };

            debug("Hiding: ", entityID, ", Is in local tree: ", isEntityInLocalTree(entityID));
            Entities.editEntity(entityID, newProperties);
        }
    }

    // Dynamic method, suppressed for now
    return;
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

/** 
 * Return the entity properties for an entity with a given name if it is in our
 * cached list of entities. Otherwise, return undefined.
 */
function getEntityWithName(name) {
    debug("Getting entity with name:", name);
    var entityID = TUTORIAL_NAME_TO_ENTITY_PROPERTIES_MAP[name];
    debug("Entity id: ", entityID, ", Is in local tree: ", isEntityInLocalTree(entityID));
    return entityID;
}

function playSuccessSound() {
    Audio.playSound(successSound, {
        position: MyAvatar.position,
        volume: 0.7,
        loop: false
    });
}

function playFirecrackerSound(position) {
    Audio.playSound(firecrackerSound, {
        position: position,
        volume: 0.5,
        loop: false
    });
}

/**
 * This disables everything, including:
 *
 *   - The door to leave the tutorial
 *   - Overlays
 *   - Hand controlelrs
 *   - Teleportation
 *   - Advanced movement
 *   - Equip and far grab
 *   - Away mode
 */
function disableEverything() {
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
    setControllerPartLayer('trigger', 'blank');
    setControllerPartLayer('joystick', 'blank');
    setControllerPartLayer('grip', 'blank');
    setControllerPartLayer('button_a', 'blank');
    setControllerPartLayer('button_b', 'blank');
    setControllerPartLayer('tips', 'blank');

    hideEntitiesWithTag('finish');

    setAwayEnabled(false);
}

/**
 * This reenables everything that disableEverything() disables. This can be
 * used when leaving the tutorial to ensure that nothing is left disabled.
 */
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
    setControllerPartLayer('trigger', 'blank');
    setControllerPartLayer('joystick', 'blank');
    setControllerPartLayer('grip', 'blank');
    setControllerPartLayer('button_a', 'blank');
    setControllerPartLayer('button_b', 'blank');
    setControllerPartLayer('tips', 'blank');
    MyAvatar.shouldRenderLocally = true;
    setAwayEnabled(true);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// STEP: DISABLE CONTROLLERS                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
var stepStart = function() {
};
stepStart.prototype = {
    start: function(onFinish) {
        disableEverything();

        HMD.requestShowHandControllers();

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

var stepEnableControllers = function() {
    this.shouldLog = false;
};
stepEnableControllers.prototype = {
    start: function(onFinish) {
        reenableEverything();
        HMD.requestHideHandControllers();
        onFinish();
    },
    cleanup: function() {
    }
};



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// STEP: Orient and raise hands above head                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
var stepOrient = function(tutorialManager) {
    this.tags = ["orient", "orient-" + tutorialManager.controllerName];
}
stepOrient.prototype = {
    start: function(onFinish) {
        this.active = true;

        var tag = this.tag;

        // Spawn content set
        //editEntitiesWithTag(this.tag, { visible: true });
        showEntitiesWithTags(this.tags);

        this.checkIntervalID = null;
        function checkForHandsAboveHead() {
            debug("Orient | Checking for hands above head");
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
        debug("Orient | Cleanup");
        if (this.active) {
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
        //editEntitiesWithTag(this.tag, { visible: false, collisionless: 1 });
        hideEntitiesWithTags(this.tags);
    }
};



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// STEP: Near Grab                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
var stepNearGrab = function() {
    this.tag = "nearGrab";
    this.tempTag = "nearGrab-temporary";
    this.birdIDs = [];

    Messages.subscribe("Entity-Exploded");
    Messages.messageReceived.connect(this.onMessage.bind(this));
}
stepNearGrab.prototype = {
    start: function(onFinish) {
        this.finished = false;
        this.onFinish = onFinish;

        setControllerPartLayer('tips', 'trigger');
        setControllerPartLayer('trigger', 'highlight');

        // Spawn content set
        showEntitiesWithTag(this.tag, { visible: true });
        showEntitiesWithTag('bothGrab', { visible: true });

        var boxSpawnPosition = getEntityWithName(NEAR_BOX_SPAWN_NAME).position;
        function createBlock(fireworkNumber) {
            fireworkBaseProps.position = boxSpawnPosition;
            fireworkBaseProps.modelURL = fireworkURLs[fireworkNumber % fireworkURLs.length];
            debug("Creating firework with url: ", fireworkBaseProps.modelURL);
            return spawnWithTag([fireworkBaseProps], null, this.tempTag)[0];
        }

        this.birdIDs = [];
        this.birdIDs.push(createBlock.bind(this)(0));
        this.birdIDs.push(createBlock.bind(this)(1));
        this.birdIDs.push(createBlock.bind(this)(2));
        this.positionWatcher = new PositionWatcher(this.birdIDs, boxSpawnPosition, -0.4, 4);
    },
    onMessage: function(channel, message, seneder) {
        if (this.finished) {
            return;
        }
        if (channel == "Entity-Exploded") {
            debug("NearGrab | Got entity-exploded message: ", message);

            var data = parseJSON(message);
            if (this.birdIDs.indexOf(data.entityID) >= 0) {
                debug("NearGrab | It's one of the firecrackers");
                playFirecrackerSound(data.position);
                playSuccessSound();
                this.finished = true;
                this.onFinish();
            }
        }
    },
    cleanup: function() {
        debug("NearGrab | Cleanup");
        this.finished = true;
        setControllerPartLayer('tips', 'blank');
        setControllerPartLayer('trigger', 'normal');
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
var stepFarGrab = function() {
    this.tag = "farGrab";
    this.tempTag = "farGrab-temporary";
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

        setControllerPartLayer('tips', 'trigger');
        setControllerPartLayer('trigger', 'highlight');
        Messages.sendLocalMessage('Hifi-Grab-Disable', JSON.stringify({
            farGrabEnabled: true,
        }));
        var tag = this.tag;

        // Spawn content set
        showEntitiesWithTag(this.tag);

        var boxSpawnPosition = getEntityWithName(FAR_BOX_SPAWN_NAME).position;
        function createBlock(fireworkNumber) {
            fireworkBaseProps.position = boxSpawnPosition;
            fireworkBaseProps.modelURL = fireworkURLs[fireworkNumber % fireworkURLs.length];
            debug("Creating firework with url: ", fireworkBaseProps.modelURL);
            return spawnWithTag([fireworkBaseProps], null, this.tempTag)[0];
        }

        this.birdIDs = [];
        this.birdIDs.push(createBlock.bind(this)(3));
        this.birdIDs.push(createBlock.bind(this)(4));
        this.birdIDs.push(createBlock.bind(this)(5));
        this.positionWatcher = new PositionWatcher(this.birdIDs, boxSpawnPosition, -0.4, 4);
    },
    onMessage: function(channel, message, seneder) {
        if (this.finished) {
            return;
        }
        if (channel == "Entity-Exploded") {
            debug("FarGrab | Got entity-exploded message: ", message);
            var data = parseJSON(message);
            if (this.birdIDs.indexOf(data.entityID) >= 0) {
                debug("FarGrab | It's one of the firecrackers");
                playFirecrackerSound(data.position);
                playSuccessSound();
                this.finished = true;
                this.onFinish();
            }
        }
    },
    cleanup: function() {
        debug("FarGrab | Cleanup");
        this.finished = true;
        setControllerPartLayer('tips', 'blank');
        setControllerPartLayer('trigger', 'normal');
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
    debug("Creating position watcher");
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
        debug("Destroying position watcher");
        Script.clearInterval(this.watcherIntervalID);
    }
};



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// STEP: Equip                                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
var stepEquip = function(tutorialManager) {
    const controllerName = tutorialManager.controllerName;

    this.tags = ["equip", "equip-" + controllerName];
    this.tagsPart1 = ["equip-part1", "equip-part1-" + controllerName];
    this.tagsPart2 = ["equip-part2", "equip-part2-" + controllerName];
    this.tempTag = "equip-temporary";

    this.PART1 = 0;
    this.PART2 = 1;
    this.PART3 = 2;
    this.COMPLETE = 3;

    Messages.subscribe('Tutorial-Spinner');
    Messages.messageReceived.connect(this.onMessage.bind(this));
}
stepEquip.prototype = {
    start: function(onFinish) {
        setControllerPartLayer('tips', 'trigger');
        setControllerPartLayer('trigger', 'highlight');

        Messages.sendLocalMessage('Hifi-Grab-Disable', JSON.stringify({
            holdEnabled: true,
        }));

        var tag = this.tag;

        // Spawn content set
        showEntitiesWithTags(this.tags);
        showEntitiesWithTags(this.tagsPart1);

        this.currentPart = this.PART1;

        function createLighter() {
            var transform = {};

            var boxSpawnProps = getEntityWithName(GUN_SPAWN_NAME);
            transform.position = boxSpawnProps.position;
            transform.rotation = boxSpawnProps.rotation;
            transform.velocity = { x: 0, y: -0.01, z: 0 };
            transform.angularVelocity = { x: 0, y: 0, z: 0 };
            this.spawnTransform = transform;
            return doCreateButaneLighter(transform).id;
        }


        this.lighterID = createLighter.bind(this)();
        this.startWatchingLighter();
        debug("Created lighter", this.lighterID);
        this.onFinish = onFinish;
    },
    startWatchingLighter: function() {
        if (!this.watcherIntervalID) {
            debug("Starting to watch lighter position");
            this.watcherIntervalID = Script.setInterval(function() {
                debug("Checking lighter position");
                var props = Entities.getEntityProperties(this.lighterID, ['position']);
                if (props.position.y < -0.4
                        || Vec3.distance(this.spawnTransform.position, props.position) > 4) {
                    debug("Moving lighter back to table");
                    Entities.editEntity(this.lighterID, this.spawnTransform);
                }
            }.bind(this), 1000);
        }
    },
    stopWatchingGun: function() {
        if (this.watcherIntervalID) {
            debug("Stopping watch of lighter position");
            Script.clearInterval(this.watcherIntervalID);
            this.watcherIntervalID = null;
        }
    },
    onMessage: function(channel, message, sender) {
        if (this.currentPart == this.COMPLETE) {
            return;
        }

        debug("Equip | Got message", channel, message, sender, MyAvatar.sessionUUID);

        if (channel == "Tutorial-Spinner") {
            if (this.currentPart == this.PART1 && message == "wasLit") {
                this.currentPart = this.PART2;
                debug("Equip | Starting part 2");
                Script.setTimeout(function() {
                    debug("Equip | Starting part 3");
                    this.currentPart = this.PART3;
                    hideEntitiesWithTags(this.tagsPart1);
                    showEntitiesWithTags(this.tagsPart2);
                    setControllerPartLayer('trigger', 'normal');
                    setControllerPartLayer('grip', 'highlight');
                    setControllerPartLayer('tips', 'grip');
                    Messages.subscribe('Hifi-Object-Manipulation');
                    debug("Equip | Finished starting part 3");
                }.bind(this), 9000);
            }
        } else if (channel == "Hifi-Object-Manipulation") {
            if (this.currentPart == this.PART3) {
                var data = parseJSON(message);
                if (data.action == 'release' && data.grabbedEntity == this.lighterID) {
                    debug("Equip | Got release, finishing step");
                    this.stopWatchingGun();
                    this.currentPart = this.COMPLETE;
                    playSuccessSound();
                    Script.setTimeout(this.onFinish.bind(this), 1500);
                }
            }
        }
    },
    cleanup: function() {
        debug("Equip | Got yaw action");
        if (this.watcherIntervalID) {
            Script.clearInterval(this.watcherIntervalID);
            this.watcherIntervalID = null;
        }

        setControllerPartLayer('tips', 'blank');
        setControllerPartLayer('grip', 'normal');
        setControllerPartLayer('trigger', 'normal');
        this.stopWatchingGun();
        this.currentPart = this.COMPLETE;

        if (this.checkCollidesTimer) {
            Script.clearInterval(this.checkCollidesTimer);
            this.checkColllidesTimer = null;
        }

        hideEntitiesWithTags(this.tagsPart1);
        hideEntitiesWithTags(this.tagsPart2);
        hideEntitiesWithTags(this.tags);
        deleteEntitiesWithTag(this.tempTag);
    }
};




///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// STEP: Turn Around                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
var stepTurnAround = function(tutorialManager) {
    this.tags = ["turnAround", "turnAround-" + tutorialManager.controllerName];
    this.tempTag = "turnAround-temporary";

    this.onActionBound = this.onAction.bind(this);
    this.numTimesSnapTurnPressed = 0;
    this.numTimesSmoothTurnPressed = 0;
}
stepTurnAround.prototype = {
    start: function(onFinish) {
        setControllerPartLayer('joystick', 'highlight');
        setControllerPartLayer('touchpad', 'arrows');
        setControllerPartLayer('tips', 'arrows');

        showEntitiesWithTags(this.tags);

        this.numTimesSnapTurnPressed = 0;
        this.numTimesSmoothTurnPressed = 0;
        this.smoothTurnDown = false;
        Controller.actionEvent.connect(this.onActionBound);

        this.interval = Script.setInterval(function() {
            debug("TurnAround | Checking if finished",
                  this.numTimesSnapTurnPressed, this.numTimesSmoothTurnPressed);
            var FORWARD_THRESHOLD = 90;
            var REQ_NUM_TIMES_SNAP_TURN_PRESSED = 3;
            var REQ_NUM_TIMES_SMOOTH_TURN_PRESSED = 2;

            var dir = Quat.getFront(MyAvatar.orientation);
            var angle = Math.atan2(dir.z, dir.x);
            var angleDegrees = ((angle / Math.PI) * 180);

            var hasTurnedEnough = this.numTimesSnapTurnPressed >= REQ_NUM_TIMES_SNAP_TURN_PRESSED
                || this.numTimesSmoothTurnPressed >= REQ_NUM_TIMES_SMOOTH_TURN_PRESSED;
            var facingForward = Math.abs(angleDegrees) < FORWARD_THRESHOLD
            if (hasTurnedEnough && facingForward) {
                Script.clearInterval(this.interval);
                this.interval = null;
                playSuccessSound();
                onFinish();
            }
        }.bind(this), 100);
    },
    onAction: function(action, value) {
        var STEP_YAW_ACTION = 6;
        var SMOOTH_YAW_ACTION = 4;

        if (action == STEP_YAW_ACTION && value != 0) {
            debug("TurnAround | Got step yaw action");
            ++this.numTimesSnapTurnPressed;
        } else if (action == SMOOTH_YAW_ACTION) {
            debug("TurnAround | Got smooth yaw action");
            if (this.smoothTurnDown && value === 0) {
                this.smoothTurnDown = false;
                ++this.numTimesSmoothTurnPressed;
            } else if (!this.smoothTurnDown && value !== 0) {
                this.smoothTurnDown = true;
            }
        }
    },
    cleanup: function() {
        debug("TurnAround | Cleanup");
        try {
            Controller.actionEvent.disconnect(this.onActionBound);
        } catch (e) {
        }

        setControllerPartLayer('joystick', 'normal');
        setControllerPartLayer('touchpad', 'blank');
        setControllerPartLayer('tips', 'blank');

        if (this.interval) {
            Script.clearInterval(this.interval);
        }
        hideEntitiesWithTags(this.tags);
        deleteEntitiesWithTag(this.tempTag);
    }
};




///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// STEP: Teleport                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
var stepTeleport = function(tutorialManager) {
    this.tags = ["teleport", "teleport-" + tutorialManager.controllerName];
    this.tempTag = "teleport-temporary";
}
stepTeleport.prototype = {
    start: function(onFinish) {
        setControllerPartLayer('button_a', 'highlight');
        setControllerPartLayer('touchpad', 'teleport');
        setControllerPartLayer('tips', 'teleport');

        Messages.sendLocalMessage('Hifi-Teleport-Disabler', 'none');

        // Wait until touching teleport pad...
        var padProps = getEntityWithName(TELEPORT_PAD_NAME);
        var xMin = padProps.position.x - padProps.dimensions.x / 2;
        var xMax = padProps.position.x + padProps.dimensions.x / 2;
        var zMin = padProps.position.z - padProps.dimensions.z / 2;
        var zMax = padProps.position.z + padProps.dimensions.z / 2;
        function checkCollides() {
            debug("Teleport | Checking if on pad...");

            var pos = MyAvatar.position;

            debug('Teleport | x', pos.x, xMin, xMax);
            debug('Teleport | z', pos.z, zMin, zMax);

            if (pos.x > xMin && pos.x < xMax && pos.z > zMin && pos.z < zMax) {
                debug("Teleport | On teleport pad");
                Script.clearInterval(this.checkCollidesTimer);
                this.checkCollidesTimer = null;
                playSuccessSound();
                onFinish();
            }
        }
        this.checkCollidesTimer = Script.setInterval(checkCollides.bind(this), 500);

        showEntitiesWithTags(this.tags);
    },
    cleanup: function() {
        debug("Teleport | Cleanup");
        setControllerPartLayer('button_a', 'normal');
        setControllerPartLayer('touchpad', 'blank');
        setControllerPartLayer('tips', 'blank');

        if (this.checkCollidesTimer) {
            Script.clearInterval(this.checkCollidesTimer);
        }
        hideEntitiesWithTags(this.tags);
        deleteEntitiesWithTag(this.tempTag);
    }
};





///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// STEP: Finish                                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
var stepFinish = function() {
    this.tag = "finish";
    this.tempTag = "finish-temporary";
}
stepFinish.prototype = {
    start: function(onFinish) {
        editEntitiesWithTag('door', { visible: false, collisonless: true });
        showEntitiesWithTag(this.tag);
        Settings.setValue("tutorialComplete", true);
        onFinish();
    },
    cleanup: function() {
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





TutorialManager = function() {
    var STEPS;

    var currentStepNum = -1;
    var currentStep = null;
    var startedTutorialAt = 0;
    var startedLastStepAt = 0;
    var didFinishTutorial = false;

    var wentToEntryStepNum;
    var VERSION = 2;
    var tutorialID;

    var self = this;

    // The real controller name is the actual detected controller name, or 'unknown'
    // if one is not found.
    if (HMD.isSubdeviceContainingNameAvailable("OculusTouch")) {
        this.controllerName = "touch";
        this.realControllerName = "touch";
    } else if (HMD.isHandControllerAvailable("OpenVR")) {
        this.controllerName = "vive";
        this.realControllerName = "vive";
    } else {
        info("ERROR, no known hand controller found, defaulting to Vive");
        this.controllerName = "vive";
        this.realControllerName = "unknown";
    }

    this.startTutorial = function() {
        currentStepNum = -1;
        currentStep = null;
        startedTutorialAt = Date.now();

        // Old versions of interface do not have the Script.generateUUID function.
        // If Script.generateUUID is not available, default to an empty string.
        tutorialID = Script.generateUUID ? Script.generateUUID() : "";
        STEPS = [
            new stepStart(this),
            new stepOrient(this),
            new stepNearGrab(this),
            new stepFarGrab(this),
            new stepEquip(this),
            new stepTurnAround(this),
            new stepTeleport(this),
            new stepFinish(this),
            new stepEnableControllers(this),
        ];
        wentToEntryStepNum = STEPS.length;
        for (var i = 0; i < STEPS.length; ++i) {
            STEPS[i].cleanup();
        }
        MyAvatar.shouldRenderLocally = false;
        this.startNextStep();
    }

    this.onFinish = function() {
        debug("onFinish", currentStepNum);
        if (currentStep && currentStep.shouldLog !== false) {
            self.trackStep(currentStep.tag, currentStepNum);
        }

        self.startNextStep();
    }

    this.startNextStep = function() {
        if (currentStep) {
            currentStep.cleanup();
        }

        ++currentStepNum;

        // This always needs to be set because we use this value when
        // tracking that the user has gone through the entry portal. When the
        // tutorial finishes, there is a last "pseudo" step that the user
        // finishes when stepping into the portal.
        startedLastStepAt = Date.now();

        if (currentStepNum >= STEPS.length) {
            // Done
            info("DONE WITH TUTORIAL");
            currentStepNum = -1;
            currentStep = null;
            didFinishTutorial = true;
            return false;
        } else {
            info("Starting step", currentStepNum);
            currentStep = STEPS[currentStepNum];
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
            HMD.requestHideHandControllers();
        }
        reenableEverything();
        currentStepNum = -1;
        currentStep = null;
    }

    this.trackStep = function(name, stepNum) {
        var timeToFinishStep = (Date.now() - startedLastStepAt) / 1000;
        var tutorialTimeElapsed = (Date.now() - startedTutorialAt) / 1000;
        UserActivityLogger.tutorialProgress(
                name, stepNum, timeToFinishStep, tutorialTimeElapsed,
                tutorialID, VERSION, this.realControllerName);
    }

    // This is a message sent from the "entry" portal in the courtyard,
    // after the tutorial has finished.
    this.enteredEntryPortal = function() {
        info("Got enteredEntryPortal");
        if (didFinishTutorial) {
            info("Tracking wentToEntry");
            this.trackStep("wentToEntry", wentToEntryStepNum);
        }
    }
}

// To run the tutorial:
//
//var tutorialManager = new TutorialManager();
//tutorialManager.startTutorial();
//
//
//var keyReleaseHandler = function(event) {
//    if (event.isShifted && event.isAlt) {
//        print('here', event.text);
//        if (event.text == "F12") {
//            if (!tutorialManager.startNextStep()) {
//                tutorialManager.startTutorial();
//            }
//        } else if (event.text == "F11") {
//            tutorialManager.restartStep();
//        } else if (event.text == "F10") {
//            MyAvatar.shouldRenderLocally = !MyAvatar.shouldRenderLocally;
//        } else if (event.text == "r") {
//            tutorialManager.stopTutorial();
//            tutorialManager.startTutorial();
//        }
//    }
//};
//Controller.keyReleaseEvent.connect(keyReleaseHandler);
