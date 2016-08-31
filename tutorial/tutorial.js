Script.include("entityData.js");
//
// var FAR_GRAB_INPUTS = [
//     Controller.Standard.RT
//     Controller.Standard.RTClick
//     Controller.Standard.LT
//     Controller.Standard.LTClick
// ];
//
// var TELEPORT_INPUTS = [
//     Controller.Standard.LeftPrimaryThumb
//     Controller.Standard.RightPrimaryThumb
// ];
//
// function noop(value) { }
// var FAR_GRAB_MAPPING_NAME = "com.highfidelity.farGrab.disable";
// var farGrabMapping = Controller.newMapping(FAR_GRAB_MAPPING_NAME);
// for (var i = 0; i < FAR_GRAB_INPUTS.length; ++i) {
//     mapping.from([FAR_GRAB_INPUTS[i]]).to(noop);
// }
//
// var TELEPORT_MAPPING_NAME = "com.highfidelity.teleport.disable";
// var teleportMapping = Controller.newMapping(TELEPORT_MAPPING_NAME);
// for (var i = 0; i < FAR_GRAB_INPUTS.length; ++i) {
//     mapping.from([TELEPORT_INPUTS[i]]).to(noop);
// }
//
// mapping.from([Controller.Standard.RT]).to(noop);
// mapping.from([Controller.Standard.RTClick]).to(noop);
//
// mapping.from([Controller.Standard.LT]).to(noop);
// mapping.from([Controller.Standard.LTClick]).to(noop);
//
// mapping.from([Controller.Standard.RB]).to(noop);
// mapping.from([Controller.Standard.LB]).to(noop);
// mapping.from([Controller.Standard.LeftGrip]).to(noop);
// mapping.from([Controller.Standard.RightGrip]).to(noop);
//
// mapping.from([Controller.Standard.LeftPrimaryThumb]).to(noop);
// mapping.from([Controller.Standard.RightPrimaryThumb]).to(noop);
//
// Script.scriptEnding.connect(function() {
//     Controller.disableMapping(MAPPING_NAME);
// });
//
// Controller.enableMapping(MAPPING_NAME);

var BASKET_URL = "http://hifi-content.s3.amazonaws.com/alan/dev/Trach-Can-3.fbx";
var BASKET_COLLIDER_URL = "http://hifi-content.s3.amazonaws.com/alan/dev/Trash-Can-4.obj";
var successSound = SoundCache.getSound(Script.resolvePath("success48.wav"));

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
var BOX_SPAWN_NAME = "tutorial/box_spawn";
var BASKET_COLLIDER_NAME = "tutorial/basket_collider";
var GUN_SPAWN_NAME = "tutorial/gun_spawn";
var GUN_AMMO_NAME = "Tutorial Ping Pong Ball"

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
        return data;
    }
    return spawn(entityData, transform, modifyFn);
}

function findEntitiesWithTag(tag) {
    return findEntities({ userData: "" }, 10000, function(properties, key, value) {
        data = parseJSON(value);
        return data.tag == tag;
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
        Menu.setIsOptionChecked("Overlays", false);
        Messages.sendLocalMessage('Hifi-Teleport-Disabler', 'both');
        Messages.sendLocalMessage('Hifi-Grab-Disable', JSON.stringify({
            nearGrabEnabled: true,
            holdEnabled: false,
            farGrabEnabled: false,
        }));
        onFinish();
    },
    cleanup: function() {
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
        spawnWithTag(HandsAboveHeadData, defaultTransform, tag);

        var checkIntervalID = null;
        function checkForHandsAboveHead() {
            print("Checking...");
            if (MyAvatar.getLeftPalmPosition().y > (MyAvatar.getHeadPosition().y + 0.1)) {
                Script.clearInterval(checkIntervalID);
                this.soundInjector = Audio.playSound(successSound, {
                    position: defaultTransform.position,
                    volume: 0.7,
                    loop: false
                });
                onFinish();
            }
        }
        checkIntervalID = Script.setInterval(checkForHandsAboveHead, 500);
    },
    cleanup: function() {
        var entityIDs = findEntitiesWithTag(this.tag);
        print("entities: ", entityIDs.length);
        for (var i = 0; i < entityIDs.length; ++i) {
            print("Deleting: ", entityIDs[i]);
            Entities.deleteEntity(entityIDs[i]);
        }
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
}
stepNearGrab.prototype = {
    start: function(onFinish) {
        var tag = this.tag;

        // Spawn content set
        spawnWithTag(Step1EntityData, null, tag);

        var basketColliderID = findEntity({ name: BASKET_COLLIDER_NAME }, 10000); 
        var basketPosition = Entities.getEntityProperties(basketColliderID, 'position').position;

        function createBlock() {
            var boxSpawnID = findEntity({ name: BOX_SPAWN_NAME }, 10000);
            if (!boxSpawnID) {
                print("Error creating block, cannot find spawn");
                return null;
            }

            Step1BlockData.position = Entities.getEntityProperties(boxSpawnID, 'position').position;
            return spawnWithTag([Step1BlockData], null, tag)[0];
        }

        // Enabled grab
        // Create table ?
        // Create blocks and basket
        var boxID = createBlock();
        print("Created", boxID);

        function onHit() {
            onFinish();
        }

        // When block collides with basket start step 2
        var checkCollidesTimer = null;
        function checkCollides() {
            print("CHECKING...");
            if (Vec3.distance(basketPosition, Entities.getEntityProperties(boxID, 'position').position) < 0.1) {
                Script.clearInterval(checkCollidesTimer);
                this.soundInjector = Audio.playSound(successSound, {
                    position: basketPosition,
                    volume: 0.7,
                    loop: false
                });
                Script.setTimeout(onHit, 1000);
            }
        }
        checkCollidesTimer = Script.setInterval(checkCollides, 500);

        // If block gets too far away or hasn't been touched for X seconds, create a new block and destroy the old block
    },
    cleanup: function() {
        var entityIDs = findEntitiesWithTag(this.tag);
        print("entities: ", entityIDs.length);
        for (var i = 0; i < entityIDs.length; ++i) {
            print("Deleting: ", entityIDs[i]);
            Entities.deleteEntity(entityIDs[i]);
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
}
stepFarGrab.prototype = {
    start: function(onFinish) {
        Messages.sendLocalMessage('Hifi-Grab-Disable', JSON.stringify({
            farGrabEnabled: true,
        }));
        var tag = this.tag;
        var transform = {
            position: { x: 3, y: 0, z: 0 },
            rotation: { x: 0, y: 0, z: 0, w: 1 }
        }

        // Spawn content set
        spawnWithTag(Step1EntityData, transform, tag);

        var basketColliderID = findEntity({ name: BASKET_COLLIDER_NAME }, 10000); 
        var basketPosition = Entities.getEntityProperties(basketColliderID, 'position').position;

        function createBlock() {
            var boxSpawnID = findEntity({ name: BOX_SPAWN_NAME }, 10000);
            if (!boxSpawnID) {
                print("Error creating block, cannot find spawn");
                return null;
            }

            Step1BlockData.position = Entities.getEntityProperties(boxSpawnID, 'position').position;
            return spawnWithTag([Step1BlockData], null, tag)[0];
        }

        // Enabled grab
        // Create table ?
        // Create blocks and basket
        var boxID = createBlock();
        print("Created", boxID);

        function onHit() {
            onFinish();
        }

        // When block collides with basket start step 2
        var checkCollidesTimer = null;
        function checkCollides() {
            print("CHECKING...");
            if (Vec3.distance(basketPosition, Entities.getEntityProperties(boxID, 'position').position) < 0.1) {
                Script.clearInterval(checkCollidesTimer);
                this.soundInjector = Audio.playSound(successSound, {
                    position: basketPosition,
                    volume: 0.7,
                    loop: false
                });
                Script.setTimeout(onHit, 1000);
            }
        }
        checkCollidesTimer = Script.setInterval(checkCollides, 500);

        // If block gets too far away or hasn't been touched for X seconds, create a new block and destroy the old block
    },
    cleanup: function() {
        var entityIDs = findEntitiesWithTag(this.tag);
        print("entities: ", entityIDs.length);
        for (var i = 0; i < entityIDs.length; ++i) {
            print("Deleting: ", entityIDs[i]);
            Entities.deleteEntity(entityIDs[i]);
        }
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
}
stepEquip.prototype = {
    start: function(onFinish) {
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
        spawnWithTag(StepGunData, defaultTransform, tag);

        var basketColliderID = findEntity({ name: BASKET_COLLIDER_NAME }, 10000); 
        var basketPosition = Entities.getEntityProperties(basketColliderID, 'position').position;

        function createGun() {
            var boxSpawnID = findEntity({ name: GUN_SPAWN_NAME }, 10000);
            if (!boxSpawnID) {
                print("Error creating block, cannot find spawn");
                return null;
            }

            GunData.position = Entities.getEntityProperties(boxSpawnID, 'position').position;
            Vec3.print("spawn", GunData.position);
            print("Adding: ", JSON.stringify(GunData));
            return spawnWithTag([GunData], null, tag)[0];
        }

        // Enabled grab
        // Create table ?
        // Create blocks and basket
        var gunID = createGun();
        print("Created", gunID);

        function onHit() {
            onFinish();
        }

        // When block collides with basket start step 2
        var checkCollidesTimer = null;
        function checkCollides() {
            print("CHECKING...");
            var ammoIDs = findEntities({ name: GUN_AMMO_NAME }, 15);
            for (var i = 0; i < ammoIDs.length; ++i) {
                if (Vec3.distance(basketPosition, Entities.getEntityProperties(ammoIDs[i], 'position').position) < 0.2) {
                    Script.clearInterval(checkCollidesTimer);
                    this.soundInjector = Audio.playSound(successSound, {
                        position: basketPosition,
                        volume: 0.7,
                        loop: false
                    });
                    Script.setTimeout(onHit, 1000);
                }
            }
        }
        checkCollidesTimer = Script.setInterval(checkCollides, 500);

        // If block gets too far away or hasn't been touched for X seconds, create a new block and destroy the old block
    },
    cleanup: function() {
        var entityIDs = findEntitiesWithTag(this.tag);
        print("entities: ", entityIDs.length);
        for (var i = 0; i < entityIDs.length; ++i) {
            print("Deleting: ", entityIDs[i]);
            Entities.deleteEntity(entityIDs[i]);
        }
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
}
stepTeleport.prototype = {
    start: function(onFinish) {
        Messages.sendLocalMessage('Hifi-Teleport-Disabler', 'none');
        Menu.setIsOptionChecked("Overlays", false);
    },
    cleanup: function() {
        var entityIDs = findEntitiesWithTag(this.tag);
        print("entities: ", entityIDs.length);
        for (var i = 0; i < entityIDs.length; ++i) {
            print("Deleting: ", entityIDs[i]);
            Entities.deleteEntity(entityIDs[i]);
        }
    }
};





var STEPS;

var currentStepNum = -1;
var currentStep = null;
function startTutorial() {
    currentStepNum = -1;
    currentStep = null;
    STEPS = [
        new stepDisableControllers("step0"),
        //new stepRaiseAboveHead("step1"),
        new stepNearGrab("step2"),
        new stepFarGrab("step3"),
        new stepEquip("step4"),
        new stepTeleport("teleport"),
    ]
    startNextStep();
}

function startNextStep() {
    if (currentStep) {
        //currentStep.cleanup();
    }

    ++currentStepNum;

    if (currentStepNum >= STEPS.length) {
        // Done
        print("DONE WITH TUTORIAL");
    } else {
        print("Starting step", currentStepNum);
        currentStep = STEPS[currentStepNum];
        currentStep.start(startNextStep);
        startNextStep();
    }
}

function skipTutorial() {
}

function stopTutorial() {
    if (currentStep) {
        currentStep.cleanup();
    }
}

location = "/tutorial";
startTutorial();

Script.scriptEnding.connect(stopTutorial);

