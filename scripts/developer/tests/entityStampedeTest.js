var NUM_ENTITIES = 100;
var RADIUS = 2;
var DIV = NUM_ENTITIES / Math.PI / 2;
var PASS_SCRIPT_URL = Script.resolvePath('').replace('.js', '-entity.js');
var FAIL_SCRIPT_URL = Script.resolvePath('').replace('.js', '-entity-fail.js');

var origin = Vec3.sum(MyAvatar.position, Vec3.multiply(5, Quat.getForward(MyAvatar.orientation)));
origin.y += HMD.eyeHeight;

var uuids = [];

Script.scriptEnding.connect(function() {
    uuids.forEach(function(id) {
        Entities.deleteEntity(id);
    });
});

for (var i=0; i < NUM_ENTITIES; i++) {
    var failGroup = i % 2;
    uuids.push(Entities.addEntity({
        type: 'Shape',
        shape: failGroup ? 'Sphere' : 'Icosahedron',
        name: 'entityStampedeTest-' + i,
        lifetime: 120,
        position: Vec3.sum(origin, Vec3.multiplyQbyV(
            MyAvatar.orientation, { x: Math.sin(i / DIV) * RADIUS, y: Math.cos(i / DIV) * RADIUS, z: 0 }
        )),
        script: (failGroup ? FAIL_SCRIPT_URL : PASS_SCRIPT_URL) + Settings.getValue('cache_buster'),
        dimensions: Vec3.HALF,
        color: { red: 0, green: 0, blue: 0 },
    }, !Entities.serversExist()));
}
