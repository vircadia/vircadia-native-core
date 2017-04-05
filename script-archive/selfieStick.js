// selfieStick.js
//
// Created by Faye Li on March 23, 2016
//
// Usage instruction: Spacebar toggles camera control - WASD first person free movement or no movement but allowing others to grab the selfie stick 
// and control your camera.
// For best result, turn off avatar collisions(Developer > Avatar > Uncheck Enable Avatar Collisions)
// 

(function() { // BEGIN LOCAL_SCOPE
    var MODEL_URL = "https://hifi-content.s3.amazonaws.com/faye/twitch-stream/selfie_stick.json";
    var AVATAR_URL =  "https://hifi-content.s3.amazonaws.com/jimi/avatar/camera/fst/camera.fst";
    var originalAvatar = null;
    var importedEntityIDs = [];
    var selfieStickEntityID = null;
    var lensEntityID = null;
    var freeMovementMode = true;

    changeAvatar();
    importModel();
    processImportedEntities();
    parentEntityToAvatar();
    setupSpaceBarControl();

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

    function parentEntityToAvatar() {
        var props = {
            "parentID" : MyAvatar.sessionUUID
        };
        Entities.editEntity(selfieStickEntityID, props);
    }

    function unparentEntityFromAvatar() {
        var props = {
            "parentID" : 0
        };
        Entities.editEntity(selfieStickEntityID, props);
    }

    function parentAvatarToEntity() {
        MyAvatar.setParentID(selfieStickEntityID);
    }

    function unparentAvatarFromEntity() {
        MyAvatar.setParentID(0);
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
                // Camera.mode = "entity";
                // Camera.cameraEntity = lensEntityID;
                unparentEntityFromAvatar();
                parentAvatarToEntity();
            } else {
                freeMovementMode = true;
                // Camera.mode = "first person";
                unparentAvatarFromEntity();
                parentEntityToAvatar();
            }
        });
        Controller.enableMapping(mappingName);
    }

    // Removes all entities we imported and reset settings we've changed
    function cleanup() {
        importedEntityIDs.forEach(function(id) {
            Entities.deleteEntity(id);
        });
        Camera.mode = "first person";
        Controller.disableMapping("Handheld-Cam-Space-Bar");
        MyAvatar.skeletonModelURL = originalAvatar;
    }
    Script.scriptEnding.connect(cleanup);
}()); // END LOCAL_SCOPE