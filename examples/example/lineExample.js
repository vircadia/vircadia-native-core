Script.include("../libraries/line.js");

var basePosition = MyAvatar.position;
var line = new InfiniteLine(basePosition);

for (var i = 0; i < (16 * Math.PI); i += 0.05) {
    var x = 0
    var y = 0.25 * Math.sin(i);
    var z = i / 10;

    line.enqueuePoint(Vec3.sum(basePosition, { x: x, y: y, z: z }));
}
