Script.include("https://s3.amazonaws.com/DreamingContent/scripts/Austin.js");

var ENTITY_SPAWN_LIMIT = 500;
var ENTITY_LIFETIME = 600; 
var RADIUS = 1.0;   // Spawn within this radius (square)
var TEST_ENTITY_NAME = "EntitySpawnTest";
    
var entities = [];
var textureIndex = 0;
var texture = Script.resolvePath('cube_texture.png');
var shader = Script.resolvePath('textureStress.fs');
var qml = Script.resolvePath('textureStress.qml');
qmlWindow = new OverlayWindow({
    title: 'Test Qml', 
    source: qml, 
    height: 240, 
    width: 320, 
    toolWindow: false,
    visible: true
});

function deleteItems(count) {
    if (!count) {
        var ids = Entities.findEntities(MyAvatar.position, 50);
        ids.forEach(function(id) {
            var properties = Entities.getEntityProperties(id, ["name"]);
            if (properties.name === TEST_ENTITY_NAME) {
                Entities.deleteEntity(id);
            }
        }, this);
        entities = [];
        return;
    } else {
        // FIXME... implement
    }
}

function createItems(count) {
    for (var i = 0; i < count; ++i) {
        var newEntity = Entities.addEntity({
            type: "Box",
            name: TEST_ENTITY_NAME,
            position: AUSTIN.avatarRelativePosition(AUSTIN.randomPositionXZ({ x: 0, y: 0, z: -2 }, RADIUS)),
            color: { r: 255, g: 255, b: 255 },
            dimensions: { x: 0.5, y: 0.5, z: 0.5 }, //AUSTIN.randomDimensions(),
            lifetime: ENTITY_LIFETIME,
            userData: JSON.stringify({ 
                ProceduralEntity: {
                    version: 2,
                    shaderUrl: shader,
                    channels: [ texture + "?" + textureIndex++ ]  
                }
            })
        });
        entities.push(newEntity);
    }    
}

qmlWindow.fromQml.connect(function(message){
    print(message);
    if (message[0] === "create") {
        var count = message[1] || 1;
        createItems(message[1] || 1);
    } else if (message[0] === "delete") {
        deleteItems(message[1]);
    }
});
