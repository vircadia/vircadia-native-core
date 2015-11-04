var partsURLS = [
    "https://s3.amazonaws.com/hifi-public/eric/models/blade.fbx",
    "https://s3.amazonaws.com/hifi-public/eric/models/body.fbx",
    "https://s3.amazonaws.com/hifi-public/eric/models/tail.fbx",
]

var parts = [];

var explodePosition;
var helicopter;
var entities = Entities.findEntities(MyAvatar.position, 2000);
for (i = 0; i < entities.length; i++) {
    var name = Entities.getEntityProperties(entities[i], 'name').name;
    if (name === "Helicopter") {
        helicopter = entities[i];
        explodeHelicopter(Entities.getEntityProperties(helicopter, 'position').position);
    }
}


function explodeHelicopter(explodePosition) {
    Entities.deleteEntity(helicopter);
    for (var i = 0; i < partsURLS.length; i++) {
        var part = Entities.addEntity({
            type: "Model",
            modelURL: partsURLS[i],
            position: explodePosition,
            shapeType: "box",
            damping: 0
        });
        parts.push(part);
    }

    Script.setTimeout(function() {
        parts.forEach(function(part) {
            var naturalDimensions = Entities.getEntityProperties(part, "naturalDimensions").naturalDimensions;
            Entities.editEntity(part, {
                dimensions: naturalDimensions,
                gravity: {
                    x: 0,
                    y: -9.6,
                    z: 0
                },
                velocity: {
                    x: Math.random(),
                    y: -10,
                    z: Math.random()
                },
                collisionsWillMove: true
            });
        });
    }, 1000);

}

function cleanup() {
    parts.forEach(function(part) {
        Entities.deleteEntity(part);
    });
}

Script.scriptEnding.connect(cleanup);