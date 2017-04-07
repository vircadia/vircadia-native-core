// selfieStick.js
//
// Created by Faye Li on March 23, 2016
//
// Usage instruction: Spacebar toggles camera control - WASD first person free movement or no movement but allowing others to grab the selfie stick 
// and control your camera.
// For best result, turn off avatar collisions(Developer > Avatar > Uncheck Enable Avatar Collisions)
// 

// selfieStick.js
//
// Created by Faye Li on March 23, 2016
//
// Usage instruction: Spacebar toggles camera control - WASD first person free movement or no movement but allowing others to grab the selfie stick 
// and control your camera.
// 

(function() { // BEGIN LOCAL_SCOPE
    var MODEL_URL = "https://hifi-content.s3.amazonaws.com/faye/twitch-stream/selfie_stick.json";
    var AVATAR_URL =  "https://hifi-content.s3.amazonaws.com/jimi/avatar/camera/fst/camera.fst";
    var originalAvatar = null;
    var importedEntityIDs = [];
    var selfieStickEntityID = null;
    var lensEntityID = null;
    var freeMovementMode = true;

    turnOffAvatarCollisions();
    changeAvatar();
    importModel();
    processImportedEntities();
    setupSpaceBarControl();
    Script.update.connect(update);

    function turnOffAvatarCollisions() {
        Menu.setIsOptionChecked("Enable avatar collisions", 0);
    }

    function turnOnAvatarCollisions() {
        Menu.setIsOptionChecked("Enable avatar collisions", 1);
    }

    function changeAvatar() {
        originalAvatar = MyAvatar.skeletonModelURL;
        MyAvatar.skeletonModelURL = AVATAR_URL;
    }

    function importModel() {
        var success = Clipboard.importEntities(MODEL_URL);
        var spawnLocation = MyAvatar.position;
        if (success) {
            importedEntityIDs = Clipboard.pasteEntities(spawnLocation);    
        }
    }

    function processImportedEntities() {
        importedEntityIDs.forEach(function(id){
            var props = Entities.getEntityProperties(id);
            if (props.name === "Selfie Stick") {
                selfieStickEntityID = id;
            } else if (props.name === "Lens") {
                lensEntityID = id;
            }
        });
    }

    function setupSpaceBarControl() {
        var mappingName = "Handheld-Cam-Space-Bar";
        var myMapping = Controller.newMapping(mappingName);
        myMapping.from(Controller.Hardware.Keyboard.Space).to(function(value){
            if ( value === 0 ) {
                return;
            }
            if (freeMovementMode) {
                freeMovementMode = false;
                Camera.mode = "entity";
                Camera.cameraEntity = lensEntityID;
            } else {
                freeMovementMode = true;
                Camera.mode = "first person";
            }
        });
        Controller.enableMapping(mappingName);
    }

    function update(deltaTime) {
        if (freeMovementMode) {
            var upFactor = 0.1;
            var upUnitVec = Vec3.normalize(Quat.getUp(MyAvatar.orientation));
            var upOffset = Vec3.multiply(upUnitVec, upFactor);
            var forwardFactor = -0.1;
            var forwardUnitVec = Vec3.normalize(Quat.getFront(MyAvatar.orientation));
            var forwardOffset = Vec3.multiply(forwardUnitVec, forwardFactor);
            var newPos = Vec3.sum(Vec3.sum(MyAvatar.position, upOffset), forwardOffset);
            var newRot = MyAvatar.orientation;
            Entities.editEntity(selfieStickEntityID, {position: newPos, rotation: newRot});
        } else {
            var props = Entities.getEntityProperties(selfieStickEntityID);
            var upFactor = 0.1;
            var upUnitVec = Vec3.normalize(Quat.getUp(props.rotation));
            var upOffset = Vec3.multiply(upUnitVec, -upFactor);
            var forwardFactor = -0.1;
            var forwardUnitVec = Vec3.normalize(Quat.getFront(props.rotation));
            var forwardOffset = Vec3.multiply(forwardUnitVec, -forwardFactor);
            var newPos = Vec3.sum(Vec3.sum(props.position, upOffset), forwardOffset);
            MyAvatar.position = newPos;
            MyAvatar.orientation = props.rotation;
        }
    }

    // Removes all entities we imported and reset settings we've changed
    function cleanup() {
        importedEntityIDs.forEach(function(id) {
            Entities.deleteEntity(id);
        });
        Camera.mode = "first person";
        Controller.disableMapping("Handheld-Cam-Space-Bar");
        MyAvatar.skeletonModelURL = originalAvatar;
        turnOnAvatarCollisions();
    }
    Script.scriptEnding.connect(cleanup);
}()); // END LOCAL_SCOPE