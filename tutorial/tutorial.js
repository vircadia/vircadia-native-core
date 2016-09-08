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
    this.soundInjector = Audio.playSound(successSound, {
        position: defaultTransform.position,
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
        setControllerPartsVisible({
            touchpad: true,
            touchpad_teleport: false,
            touchpad_arrows: false
        });
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
        Script.clearTimeout(this.timerID);
        hideEntitiesWithTag(this.tag);
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
            print("Checking...");
            if (MyAvatar.getLeftPalmPosition().y > (MyAvatar.getHeadPosition().y + 0.1)) {
                Script.clearInterval(this.checkIntervalID);
                this.checkIntervalID = null;
                this.soundInjector = Audio.playSound(successSound, {
                    position: defaultTransform.position,
                    volume: 0.7,
                    loop: false
                });
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
        setControllerVisible("trigger", true);
        var tag = this.tag;

        // Spawn content set
        //spawnWithTag(Step1EntityData, null, tag);
        showEntitiesWithTag(this.tag, { visible: true });

        var basketColliderID = findEntity({ name: NEAR_BASKET_COLLIDER_NAME }, 10000); 
        var basketPosition = Entities.getEntityProperties(basketColliderID, 'position').position;

        var boxSpawnID = findEntity({ name: NEAR_BOX_SPAWN_NAME }, 10000);
        if (!boxSpawnID) {
            print("Error creating block, cannot find spawn");
            return null;
        }
        var boxSpawnPosition = Entities.getEntityProperties(boxSpawnID, 'position').position;
        function createBlock() {
            Step1BlockData.position = boxSpawnPosition;
            return spawnWithTag([Step1BlockData], null, this.tempTag)[0];
        }

        // Enabled grab
        // Create table ?
        // Create blocks and basket
        this.boxID = createBlock.bind(this)();
        print("Created", this.boxID);

        //function posChecker() {
            //Vec3.distance(
        //}

        function onHit() {
            onFinish();
        }

        // When block collides with basket start step 2
        function checkCollides() {
            var dist = Vec3.distance(basketPosition, Entities.getEntityProperties(this.boxID, 'position').position);
            print(this.tag, "CHECKING...", dist);
            if (dist < 0.1) {
                Script.clearInterval(this.checkCollidesTimer);
                this.checkCollidesTimer = null;
                this.soundInjector = Audio.playSound(successSound, {
                    position: basketPosition,
                    volume: 0.7,
                    loop: false
                });
                Script.setTimeout(onHit.bind(this), 1000);
            }
        }
        this.checkCollidesTimer = Script.setInterval(checkCollides.bind(this), 500);

        // If block gets too far away or hasn't been touched for X seconds, create a new block and destroy the old block
    },
    cleanup: function() {
        setControllerVisible("trigger", false);
        if (this.checkCollidesTimer) {
            Script.clearInterval(this.checkCollidesTimer);
        }
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

            Step1BlockData.position = Entities.getEntityProperties(boxSpawnID, 'position').position;
            return spawnWithTag([Step1BlockData], null, this.tempTag)[0];
        }

        // Enabled grab
        // Create table ?
        // Create blocks and basket
        this.boxID = createBlock.bind(this)();
        print("Created", this.boxID);

        function onHit() {
            onFinish();
        }

        // When block collides with basket start step 2
        var checkCollidesTimer = null;
        function checkCollides() {
            print("CHECKING...");
            if (Vec3.distance(basketPosition, Entities.getEntityProperties(this.boxID, 'position').position) < 0.2) {
                Script.clearInterval(checkCollidesTimer);
                this.soundInjector = Audio.playSound(successSound, {
                    position: basketPosition,
                    volume: 0.7,
                    loop: false
                });
                Script.setTimeout(onHit.bind(this), 1000);
            }
        }
        checkCollidesTimer = Script.setInterval(checkCollides.bind(this), 500);

        // If block gets too far away or hasn't been touched for X seconds, create a new block and destroy the old block
    },
    cleanup: function() {
        setControllerVisible("trigger", false);
        hideEntitiesWithTag(this.tag, { visible: false});
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

        this.hasFinished = false;

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

        function onHit() {
            hideEntitiesWithTag(this.tagPart1);
            showEntitiesWithTag(this.tagPart2);
            print("HIT, wiating for unequip...");
            Messages.subscribe('Hifi-Object-Manipulation');
            Messages.messageReceived.connect(this.onMessage.bind(this));
        }

        // When block collides with basket start step 2
        function checkCollides() {
            print("CHECKING FOR PING PONG...");
            var ammoIDs = findEntities({ name: GUN_AMMO_NAME }, 15);
            for (var i = 0; i < ammoIDs.length; ++i) {
                if (Vec3.distance(basketPosition, Entities.getEntityProperties(ammoIDs[i], 'position').position) < 0.25) {
                    Script.clearInterval(this.checkCollidesTimer);
                    this.checkCollidesTimer = null;
                    this.soundInjector = Audio.playSound(successSound, {
                        position: basketPosition,
                        volume: 0.7,
                        loop: false
                    });
                    Script.setTimeout(onHit.bind(this), 1000);
                    return;
                }
            }
        }
        this.checkCollidesTimer = Script.setInterval(checkCollides.bind(this), 100);

        // If block gets too far away or hasn't been touched for X seconds, create a new block and destroy the old block
    },
    onMessage: function(channel, message, sender) {
        if (this.hasFinished) {
            return;
        }
        print("Got message", channel, message, sender, MyAvatar.sessionUUID);
        //if (sender === MyAvatar.sessionUUID) {
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
                this.hasFinished = true;
                //this.onFinish();
            }
        //}
    },
    cleanup: function() {
        setControllerVisible("trigger", false);
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
}
stepTurnAround.prototype = {
    start: function(onFinish) {
        setControllerVisible("left", true);
        setControllerVisible("right", true);

        setControllerPartsVisible({
            touchpad: false,
            touchpad_teleport: false,
            touchpad_arrows: true
        });

        showEntitiesWithTag(this.tag);
        var hasTurnedAround = false;
        this.interval = Script.setInterval(function() {
            var dir = Quat.getFront(MyAvatar.orientation);
            var angle = Math.atan2(dir.z, dir.x);
            var angleDegrees = ((angle / Math.PI) * 180);
            print("CHECK");
            if (!hasTurnedAround) {
                if (Math.abs(angleDegrees) > 100) {
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

        setControllerPartsVisible({
            touchpad: true,
            touchpad_teleport: false,
            touchpad_arrows: false
        });

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

        setControllerPartsVisible({
            touchpad: false,
            touchpad_teleport: true,
            touchpad_arrows: false
        });

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

        setControllerPartsVisible({
            touchpad: true,
            touchpad_teleport: false,
            touchpad_arrows: false
        });

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
        var newProperties = {
            visible: data.visible == false ? false : true,
            collisionless: data.visible == false ? true : false ,
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
        new stepWelcome("welcome"),
        new stepRaiseAboveHead("raiseHands"),
        new stepNearGrab("nearGrab"),
        new stepFarGrab("farGrab"),
        new stepEquip("equip"),
        new stepTurnAround("turnAround"),
        new stepTeleport("teleport"),
        new stepFinish("finish"),
    ]
    location = "/tutorial";
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
    } else if (event.text == "r") {
        stopTutorial();
        startTutorial();
    }
});

// Messages.sendLocalMessage('Controller-Display', JSON.stringify({
//     name: "menu",
//     visible: false,
// }));
