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

Script.include("entityData.js");

Script.include("viveHandsv2.js");

var BASKET_URL = "http://hifi-content.s3.amazonaws.com/alan/dev/Trach-Can-3.fbx";
var BASKET_COLLIDER_URL = "http://hifi-content.s3.amazonaws.com/alan/dev/Trash-Can-4.obj";
//var successSound = SoundCache.getSound(Script.resolvePath("success48.wav"));
var successSound = SoundCache.getSound("http://hifi-content.s3.amazonaws.com/DomainContent/Tutorial/Sounds/good_one.L.wav");

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
// On start tutorial...

// Load assets
var NEAR_BOX_SPAWN_NAME = "tutorial/nearGrab/box_spawn";
var FAR_BOX_SPAWN_NAME = "tutorial/farGrab/box_spawn";
var NEAR_BASKET_COLLIDER_NAME = "tutorial/nearGrab/basket_collider";
var FAR_BASKET_COLLIDER_NAME = "tutorial/farGrab/basket_collider";
var GUN_BASKET_COLLIDER_NAME = "tutorial/equip/basket_collider";
var GUN_SPAWN_NAME = "tutorial/gun_spawn";
var GUN_AMMO_NAME = "Tutorial Ping Pong Ball"
var TELEPORT_PAD_NAME = "tutorial/teleport/pad"

function spawn(entityData, transform, modifyFn) {
    print("Creating: ", entityData);
    if (!transform) {
        transform = {
            position: { x: 0, y: 0, z: 0 },
            rotation: { x: 0, y: 0, z: 0, w: 1 }
        }
    }
    var ids = [];
    for (var i = 0; i < entityData.length; ++i) {
        var data = entityData[i];
        print("Creating: ", data.name);
        data.position = Vec3.sum(transform.position, data.position);
        data.rotation = Quat.multiply(data.rotation, transform.rotation);
        if (modifyFn) {
            data = modifyFn(data);
        }
        var id = Entities.addEntity(data);
        ids.push(id);
        print(id, "data:", JSON.stringify(data));
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
        print("In modify", tag, userData, data.userData);
        return data;
    }
    return spawn(entityData, transform, modifyFn);
}

function deleteEntitiesWithTag(tag) {
        print("searching for...:", tag);
    var entityIDs = findEntitiesWithTag(tag);
    for (var i = 0; i < entityIDs.length; ++i) {
        print("Deleteing:", entityIDs[i]);
        Entities.deleteEntity(entityIDs[i]);
    }
}
function editEntitiesWithTag(tag, propertiesOrFn) {
    print("Editing:", tag);
    var entityIDs = findEntitiesWithTag(tag);
    print("Editing...", entityIDs);
    for (var i = 0; i < entityIDs.length; ++i) {
        print("Editing...", entityIDs[i]);
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
}
stepDisableControllers.prototype = {
    start: function(onFinish) {
        editEntitiesWithTag('door', { visible: true });
        Menu.setIsOptionChecked("Overlays", false);
        Messages.sendLocalMessage('Hifi-Teleport-Disabler', 'both');
        Messages.sendLocalMessage('Hifi-Grab-Disable', JSON.stringify({
            nearGrabEnabled: true,
            holdEnabled: false,
            farGrabEnabled: false,
        }));
        setControllerPartLayer('touchpad', 'blank');
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
        print("Updating...");
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

        this.overlay = new StayInFrontOverlay("model", {
            url: "http://hifi-content.s3.amazonaws.com/alan/dev/Prompt-Cards/raiseHands.fbx?11",
            ignoreRayIntersection: true,
        }, 2, { x: 0, y: 0.3, z: 0 });

        // Spawn content set
        //spawnWithTag(HandsAboveHeadData, defaultTransform, tag);
        print("raise hands...", this.tag);
        editEntitiesWithTag(this.tag, { visible: true });


        this.checkIntervalID = null;
        function checkForHandsAboveHead() {
            print("Checking for hands above head...");
            if (MyAvatar.getLeftPalmPosition().y > (MyAvatar.getHeadPosition().y + 0.1)) {
                Script.clearInterval(this.checkIntervalID);
                this.checkIntervalID = null;
                playSuccessSound();
                location = "/tutorial";
                onFinish();
            }
        }
        this.checkIntervalID = Script.setInterval(checkForHandsAboveHead.bind(this), 500);
    },
    cleanup: function() {
        if (this.overlay) {
            this.overlay.destroy();
            this.overlay = null;
        }
        if (this.checkIntervalID != null) {
            Script.clearInterval(this.checkIntervalID);
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
        //spawnWithTag(HandsAboveHeadData, defaultTransform, tag);
        print("raise hands...", this.tag);
        editEntitiesWithTag(this.tag, { visible: true });


        this.checkIntervalID = null;
        function checkForHandsAboveHead() {
            print("Checking for hands above head...");
            if (MyAvatar.getLeftPalmPosition().y > (MyAvatar.getHeadPosition().y + 0.1)) {
                Script.clearInterval(this.checkIntervalID);
                this.checkIntervalID = null;
                playSuccessSound();
                //location = "/tutorial";
                onFinish();
            }
        }
        this.checkIntervalID = Script.setInterval(checkForHandsAboveHead.bind(this), 500);
    },
    cleanup: function() {
        if (this.checkIntervalID != null) {
            Script.clearInterval(this.checkIntervalID);
        }
        editEntitiesWithTag(this.tag, { visible: false, collisionless: 1 });
        deleteEntitiesWithTag(this.tempTag);
    }
};

function setControllerVisible(name, visible) {
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


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// STEP: Near Grab                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
var stepNearGrab = function(name) {
    this.tag = name;
    this.tempTag = name + "-temporary";
}
stepNearGrab.prototype = {
    start: function(onFinish) {
        this.finished = false;
        this.onFinish = onFinish;

        setControllerVisible("trigger", true);
        var tag = this.tag;

        // Spawn content set
        //spawnWithTag(Step1EntityData, null, tag);
        showEntitiesWithTag(this.tag, { visible: true });
        showEntitiesWithTag('bothGrab', { visible: true });

        var basketColliderID = findEntity({ name: NEAR_BASKET_COLLIDER_NAME }, 10000); 
        var basketPosition = Entities.getEntityProperties(basketColliderID, 'position').position;

        var boxSpawnID = findEntity({ name: NEAR_BOX_SPAWN_NAME }, 10000);
        if (!boxSpawnID) {
            print("Error creating block, cannot find spawn");
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
        this.boxID = createBlock.bind(this)();
        print("Created", this.boxID);

        //function posChecker() {
            //Vec3.distance(
        //}

        Messages.subscribe("Entity-Exploded");
        Messages.messageReceived.connect(this.onMessage.bind(this));

        // If block gets too far away or hasn't been touched for X seconds, create a new block and destroy the old block
    },
    onMessage: function(channel, message, seneder) {
        if (this.finished) {
            return;
        }
        if (channel == "Entity-Exploded") {
            print("TUTORIAL: Got entity-exploded message");
            playSuccessSound();
            var data = parseJSON(message);
            //if (data.entityID == this.boxID) {
                this.finished = true;
                this.onFinish();
            //}
        }
    },
    cleanup: function() {
        print("cleaning up near grab");
        this.finished = true;
        setControllerVisible("trigger", false);
        hideEntitiesWithTag(this.tag, { visible: false});
        deleteEntitiesWithTag(this.tempTag);
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
}
stepFarGrab.prototype = {
    start: function(onFinish) {
        this.finished = false;
        this.onFinish = onFinish;

        showEntitiesWithTag('bothGrab', { visible: true });

        setControllerVisible("trigger", true);
        Messages.sendLocalMessage('Hifi-Grab-Disable', JSON.stringify({
            farGrabEnabled: true,
        }));
        var tag = this.tag;
        var transform = {
            position: { x: 3, y: 0, z: 0 },
            rotation: { x: 0, y: 0, z: 0, w: 1 }
        }

        // Spawn content set
        //spawnWithTag(Step1EntityData, transform, tag);
        showEntitiesWithTag(this.tag);

        var basketColliderID = findEntity({ name: FAR_BASKET_COLLIDER_NAME }, 10000); 
        var basketPosition = Entities.getEntityProperties(basketColliderID, 'position').position;

        function createBlock() {
            var boxSpawnID = findEntity({ name: FAR_BOX_SPAWN_NAME }, 10000);
            if (!boxSpawnID) {
                print("Error creating block, cannot find spawn");
                return null;
            }

            birdFirework1.position = Entities.getEntityProperties(boxSpawnID, 'position').position;
            return spawnWithTag([birdFirework1], null, this.tempTag)[0];
        }

        // Enabled grab
        // Create table ?
        // Create blocks and basket
        this.boxID = createBlock.bind(this)();
        print("Created", this.boxID);

        Messages.subscribe("Entity-Exploded");
        Messages.messageReceived.connect(this.onMessage.bind(this));

        // When block collides with basket start step 2
        //var checkCollidesTimer = null;
        // function checkCollides() {
        //     print("CHECKING...");
        //     if (Vec3.distance(basketPosition, Entities.getEntityProperties(this.boxID, 'position').position) < 0.2) {
        //         Script.clearInterval(checkCollidesTimer);
        //         playSuccessSound();
        //         Script.setTimeout(onHit.bind(this), 1000);
        //     }
        // }
        // checkCollidesTimer = Script.setInterval(checkCollides.bind(this), 500);

        // If block gets too far away or hasn't been touched for X seconds, create a new block and destroy the old block
    },
    onMessage: function(channel, message, seneder) {
        if (this.finished) {
            return;
        }
        if (channel == "Entity-Exploded") {
            print("TUTORIAL: Got entity-exploded message");
            playSuccessSound();
            var data = parseJSON(message);
            if (data.entityID == this.boxID) {
                this.finished = true;
                this.onFinish();
            }
        }
    },
    cleanup: function() {
        //Messages.messageReceived.disconnect(this.onMessage.bind(this));
        this.finished = true;
        setControllerVisible("trigger", false);
        hideEntitiesWithTag(this.tag, { visible: false});
        hideEntitiesWithTag('bothGrab', { visible: false});
        deleteEntitiesWithTag(this.tempTag);
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
    this.COMPLETE = 2;
}
stepEquip.prototype = {
    start: function(onFinish) {
        setControllerVisible("trigger", true);
        Messages.sendLocalMessage('Hifi-Grab-Disable', JSON.stringify({
            holdEnabled: true,
        }));

        var tag = this.tag;

        var defaultTransform = {
            position: {
                x: 0.0,
                y: 0.0,
                z: 0.75
            },
            rotation: {
                x: 0,
                y: 0,
                z: 0,
                w: 1
            }
        };

        // Spawn content set
        //spawnWithTag(StepGunData, defaultTransform, tag);
        showEntitiesWithTag(this.tag);
        showEntitiesWithTag(this.tagPart1);

        this.currentPart = this.PART1;

        var basketColliderID = findEntity({ name: GUN_BASKET_COLLIDER_NAME }, 10000); 
        var basketPosition = Entities.getEntityProperties(basketColliderID, 'position').position;

        function createGun() {
            var boxSpawnID = findEntity({ name: GUN_SPAWN_NAME }, 10000);
            if (!boxSpawnID) {
                print("Error creating block, cannot find spawn");
                return null;
            }

            GunData.position = Entities.getEntityProperties(boxSpawnID, 'position').position;
            GunData.rotation = Entities.getEntityProperties(boxSpawnID, 'rotation').rotation;
            Vec3.print("spawn", GunData.position);
            print("Adding: ", JSON.stringify(GunData));
            return spawnWithTag([GunData], null, this.tempTag)[0];
        }

        // Enabled grab
        // Create table ?
        // Create blocks and basket
        this.gunID = createGun.bind(this)();
        print("Created", this.gunID);
        this.onFinish = onFinish;
        Messages.subscribe('Tutorial-Spinner');
        Messages.messageReceived.connect(this.onMessage.bind(this));

//        function onHit() {
//        }
//
//        // When block collides with basket start step 2
//        function checkCollides() {
//            //print("CHECKING FOR PING PONG...");
//            var ammoIDs = findEntities({ name: GUN_AMMO_NAME }, 15);
//            for (var i = 0; i < ammoIDs.length; ++i) {
//                if (Vec3.distance(basketPosition, Entities.getEntityProperties(ammoIDs[i], 'position').position) < 0.25) {
//                    Script.clearInterval(this.checkCollidesTimer);
//                    this.checkCollidesTimer = null;
//                    playSuccessSound();
//                    Script.setTimeout(onHit.bind(this), 1000);
//                    return;
//                }
//            }
//        }
//        this.checkCollidesTimer = Script.setInterval(checkCollides.bind(this), 100);


        // If block gets too far away or hasn't been touched for X seconds, create a new block and destroy the old block
    },
    onMessage: function(channel, message, sender) {
        if (this.currentPart == this.COMPLETE) {
            return;
        }
        print("Got message", channel, message, sender, MyAvatar.sessionUUID);
        if (channel == "Tutorial-Spinner") {
            if (this.currentPart == this.PART1 && message == "wasLit") {
                hideEntitiesWithTag(this.tagPart1);
                showEntitiesWithTag(this.tagPart2);
                Messages.subscribe('Hifi-Object-Manipulation');
            }
        } else if (channel == "Hifi-Object-Manipulation") {
            if (this.currentPart == this.PART2) {
                var data = parseJSON(message);
                print("Here", data.action, data.grabbedEntity, this.gunID);
                if (data.action == 'release' && data.grabbedEntity == this.gunID) {
                    try {
                        Messages.messageReceived.disconnect(this.onMessage);
                    } catch(e) {
                    }
                    playSuccessSound();
                    print("FINISHED");
                    Script.setTimeout(this.onFinish.bind(this), 1500);
                    this.currentPart = this.COMPLETE;
                    //this.onFinish();
                }
            }
        }
    },
    cleanup: function() {
        setControllerVisible("trigger", false);
        this.currentPart = this.COMPLETE;
        try {
            Messages.messageReceived.disconnect(this.onMessage);
        } catch(e) {
            print("error disconnecting");
        }
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

        showEntitiesWithTag(this.tag);
        var hasTurnedAround = false;
        this.interval = Script.setInterval(function() {
            var dir = Quat.getFront(MyAvatar.orientation);
            var angle = Math.atan2(dir.z, dir.x);
            var angleDegrees = ((angle / Math.PI) * 180);
            print("CHECK");
            if (!hasTurnedAround) {
                if (Math.abs(angleDegrees) > 140) {
                    hasTurnedAround = true;
                    print("half way there...");
                }
            } else {
                if (Math.abs(angleDegrees) < 30) {
                    Script.clearInterval(this.interval);
                    this.interval = null;
                    print("DONE");
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

        Messages.sendLocalMessage('Hifi-Teleport-Disabler', 'none');

        // Wait until touching teleport pad...
        var padID = findEntity({ name: TELEPORT_PAD_NAME }, 100);
        print(padID);
        var padProps = Entities.getEntityProperties(padID, ["position", "dimensions"]);
        print(Object.keys(padProps));
        var xMin = padProps.position.x - padProps.dimensions.x / 2;
        var xMax = padProps.position.x + padProps.dimensions.x / 2;
        var zMin = padProps.position.z - padProps.dimensions.z / 2;
        var zMax = padProps.position.z + padProps.dimensions.z / 2;
        function checkCollides() {
            print("Checking if on pad...");
            var pos = MyAvatar.position;
            print('x', pos.x, xMin, xMax);
            print('z', pos.z, zMin, zMax);
            if (pos.x > xMin && pos.x < xMax && pos.z > zMin && pos.z < zMax) {
                print("On pad!!");
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
        //setControllerVisible("teleport", false);

        setControllerPartLayer('touchpad', 'blank');

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
        editEntitiesWithTag('door', { visible: false });
        showEntitiesWithTag(this.tag);
    },
    cleanup: function() {
        //Menu.setIsOptionChecked("Overlays", true);
        hideEntitiesWithTag(this.tag);
        deleteEntitiesWithTag(this.tempTag);
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
        var newProperties = {
            visible: data.visible == false ? false : true,
            collisionless: collisionless,
            //collisionless: data.collisionless == true ? true : false,
        };
        Entities.editEntity(entityID, newProperties);
    });
}
function hideEntitiesWithTag(tag) {
    editEntitiesWithTag(tag, function(entityID) {
        var userData = Entities.getEntityProperties(entityID, "userData").userData;
        var data = parseJSON(userData);
        var newProperties = {
            visible: false,
            collisionless: 1,
            ignoreForCollisions: 1,
        };
        Entities.editEntity(entityID, newProperties);
    });
}

var STEPS;

var currentStepNum = -1;
var currentStep = null;
function startTutorial() {
    currentStepNum = -1;
    currentStep = null;
    STEPS = [
        new stepDisableControllers("step0"),
        new stepOrient("orient"),
        new stepWelcome("welcome"),
        new stepRaiseAboveHead("raiseHands"),
        new stepNearGrab("nearGrab"),
        new stepFarGrab("farGrab"),
        new stepEquip("equip"),
        new stepTurnAround("turnAround"),
        new stepTeleport("teleport"),
        new stepFinish("finish"),
    ];
    for (var i = 0; i < STEPS.length; ++i) {
        STEPS[i].cleanup();
    }
    location = "/tutorial_begin";
    //location = "/tutorial";
    MyAvatar.shouldRenderLocally = false;
    startNextStep();
}

function startNextStep() {
    if (currentStep) {
        currentStep.cleanup();
    }

    ++currentStepNum;

    if (currentStepNum >= STEPS.length) {
        // Done
        print("DONE WITH TUTORIAL");
        currentStepNum = -1;
        currentStep = null;
        return false;
    } else {
        print("Starting step", currentStepNum);
        currentStep = STEPS[currentStepNum];
        currentStep.start(startNextStep);
        return true;
    }
}
function restartStep() {
    if (currentStep) {
        currentStep.cleanup();
        currentStep.start(startNextStep);
    }
}

function skipTutorial() {
}

function stopTutorial() {
    if (currentStep) {
        currentStep.cleanup();
    }
    currentStepNum = -1;
    currentStep = null;
}

startTutorial();

Script.scriptEnding.connect(function() {
    Controller.enableMapping('handControllerPointer-click');
});
Controller.disableMapping('handControllerPointer-click');

//mapping.from([Controller.Standard.RY]).to(noop);
        //{ "from": "Vive.LeftApplicationMenu", "to": "Standard.LeftSecondaryThumb" },
//mapping.from([Controller.Standard.RY]).when("Controller.Application.Grounded").to(noop);
//mapping.from([Controller.Standard.RY]).when(Controller.Application.Grounded).to(noop);


Script.scriptEnding.connect(stopTutorial);



Controller.keyReleaseEvent.connect(function (event) {
    print(event.text);
    if (event.text == ",") {
        if (!startNextStep()) {
            startTutorial();
        }
    } else if (event.text == "F12") {
        restartStep();
    } else if (event.text == "F10") {
        MyAvatar.shouldRenderLocally = !MyAvatar.shouldRenderLocally;
    } else if (event.text == "r") {
        stopTutorial();
        startTutorial();
    }
});

// Messages.sendLocalMessage('Controller-Display', JSON.stringify({
//     name: "menu",
//     visible: false,
// }));
