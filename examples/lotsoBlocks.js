var NUM_BLOCKS = 200;
var size;
var SPAWN_RANGE = 10;
var boxes = [];
var basePosition, avatarRot;
var isAssignmentScript = false;
if(isAssignmentScript){
  basePosition = {x: 8000, y: 8000, z: 8000};
}
else {
  avatarRot = Quat.fromPitchYawRollDegrees(0, MyAvatar.bodyYaw, 0.0);
  basePosition = Vec3.sum(MyAvatar.position, Vec3.multiply(SPAWN_RANGE * 3, Quat.getFront(avatarRot)));
}
basePosition.y -= SPAWN_RANGE;

var ground = Entities.addEntity({
  type: "Model",
  modelURL: "https://hifi-public.s3.amazonaws.com/eric/models/woodFloor.fbx",
  dimensions: {
    x: 100,
    y: 2,
    z: 100
  },
  position: basePosition,
  shapeType: 'box'
});


basePosition.y += SPAWN_RANGE + 2;
for (var i = 0; i < NUM_BLOCKS; i++) {
  size = randFloat(-.2, 0.7);
  boxes.push(Entities.addEntity({
    type: 'Box',
    dimensions: {
      x: size,
      y: size,
      z: size
    },
    position: {
      x: basePosition.x + randFloat(-SPAWN_RANGE, SPAWN_RANGE),
      y: basePosition.y - randFloat(-SPAWN_RANGE, SPAWN_RANGE),
      z: basePosition.z + randFloat(-SPAWN_RANGE, SPAWN_RANGE)
    },
    color: {red: Math.random() * 255, green: Math.random() * 255, blue: Math.random() * 255},
    dynamic: true,
    gravity: {x: 0, y: 0, z: 0}
  }));
}



function cleanup() {
  Entities.deleteEntity(ground);
  boxes.forEach(function(box){
    Entities.deleteEntity(box);
  });
}

Script.scriptEnding.connect(cleanup);

function randFloat(low, high) {
  return low + Math.random() * ( high - low );
}
