var avatarDetector = null;

function createAvatarDetector() {

    var detectorProperties = {
        name: 'Hifi-Avatar-Detector',
        type: 'Box',
        position: MyAvatar.position,
        dimensions: {
            x: 1,
            y: 2,
            z: 1
        },
        collisionsWillMove: false,
        ignoreForCollisions: true,
        visible: false,
        color: {
            red: 255,
            green: 0,
            blue: 0
        }
    }
   avatarDetector= Entities.addEntity(detectorProperties);
};
var updateAvatarDetector = function() {
    //  print('updating detector position' + JSON.stringify(MyAvatar.position))

    Entities.editEntity(avatarDetector, {
        position: MyAvatar.position
    })
};

var cleanup = function() {
    Script.update.disconnect(updateAvatarDetector);
    Entities.deleteEntity(avatarDetector);
}

createAvatarDetector();
Script.scriptEnding.connect(cleanup);
Script.update.connect(updateAvatarDetector);