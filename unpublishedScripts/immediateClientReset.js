/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, pointInExtents, vec3equal, setEntityCustomData, getEntityCustomData */

/*global MasterReset */

var masterResetScript = Script.resolvePath("masterReset.js");
var hiddenEntityScriptURL = Script.resolvePath("hiddenEntityReset.js");

Script.include(masterResetScript);



function createHiddenMasterSwitch() {

    var resetKey = "resetMe";
    var masterSwitch = Entities.addEntity({
        type: "Box",
        name: "Master Switch",
        script: hiddenEntityScriptURL,
        dimensions: {
            x: 0.2,
            y: 0.2,
            z: 0.2
        },
        color: {
            red: 42,
            green: 36,
            blue: 30
        },
        position: {
            x: 554,
            y: 495.5,
            z: 503.2
        }
    });
}


var entities = Entities.findEntities(MyAvatar.position, 100);

entities.forEach(function (entity) {
    //params: customKey, id, defaultValue
    var name = Entities.getEntityProperties(entity, "name").name
    if (name === "Master Switch") {
        Entities.deleteEntity(entity);
    }
});
createHiddenMasterSwitch();
MasterReset();