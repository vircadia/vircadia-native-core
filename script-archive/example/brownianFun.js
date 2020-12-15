var 
var VIRCADIA_PUBLIC_CDN = networkingConstants.PUBLIC_BUCKET_CDN_URL;
var SOUND_PATH = VIRCADIA_PUBLIC_CDN + "sounds/Collisions-hitsandslaps/";
var soundURLS = ["67LCollision01.wav", "67LCollision02.wav", "airhockey_hit1.wav"];
var FLOOR_SIZE = 10;
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(FLOOR_SIZE * 1.5, Quat.getFront(Camera.getOrientation())));
var WALL_WIDTH = .1;
var FLOOR_HEIGHT_OFFSET = -2;
var WALL_HEIGHT = FLOOR_SIZE / 4;
var BALL_DROP_HEIGHT = center.y + WALL_HEIGHT;
var NUM_BALLS = 50;
var BALL_RADIUS = 1;
var BROWNIAN_INTERVAL_TIME = 16;
var BROWNIAN_FORCE_RANGE = 5;
var SPAWN_TIME = 50;
var spawnCount = 0;

var brownianMotionActivated = false;
var buttonOffColor = {
  red: 250,
  green: 10,
  blue: 10
};
var buttonOnColor = {
  red: 10,
  green: 200,
  blue: 100
};

var bounds = {
  xMin: center.x - FLOOR_SIZE / 2,
  xMax: center.x + FLOOR_SIZE / 2,
  zMin: center.z - FLOOR_SIZE / 2,
  zMax: center.z + FLOOR_SIZE / 2
};
var balls = [];

var screenSize = Controller.getViewportDimensions();
var BUTTON_SIZE = 32;
var PADDING = 3;

var brownianButton = Overlays.addOverlay("image", {
  x: screenSize.x / 2 - BUTTON_SIZE,
  y: screenSize.y - (BUTTON_SIZE + PADDING),
  width: BUTTON_SIZE,
  height: BUTTON_SIZE,
  imageURL: VIRCADIA_PUBLIC_CDN + "images/blocks.png",
  color: buttonOffColor,
  alpha: 1
});

var floor = Entities.addEntity({
  type: 'Box',
  position: Vec3.sum(center, {
    x: 0,
    y: FLOOR_HEIGHT_OFFSET,
    z: 0
  }),
  dimensions: {
    x: FLOOR_SIZE,
    y: WALL_WIDTH,
    z: FLOOR_SIZE
  },
  color: {
    red: 100,
    green: 100,
    blue: 100
  }
});

var rightWall = Entities.addEntity({
  type: 'Box',
  position: Vec3.sum(center, {
    x: FLOOR_SIZE / 2,
    y: FLOOR_HEIGHT_OFFSET / 2,
    z: 0
  }),
  dimensions: {
    x: WALL_WIDTH,
    y: WALL_HEIGHT,
    z: FLOOR_SIZE
  },
  color: {
    red: 120,
    green: 100,
    blue: 120
  }
});

var leftWall = Entities.addEntity({
  type: 'Box',
  position: Vec3.sum(center, {
    x: -FLOOR_SIZE / 2,
    y: FLOOR_HEIGHT_OFFSET / 2,
    z: 0
  }),
  dimensions: {
    x: WALL_WIDTH,
    y: WALL_HEIGHT,
    z: FLOOR_SIZE
  },
  color: {
    red: 120,
    green: 100,
    blue: 120
  }
});

var backWall = Entities.addEntity({
  type: 'Box',
  position: Vec3.sum(center, {
    x: 0,
    y: FLOOR_HEIGHT_OFFSET / 2,
    z: -FLOOR_SIZE / 2,
  }),
  dimensions: {
    x: FLOOR_SIZE,
    y: WALL_HEIGHT,
    z: WALL_WIDTH
  },
  color: {
    red: 120,
    green: 100,
    blue: 120
  }
});

var frontWall = Entities.addEntity({
  type: 'Box',
  position: Vec3.sum(center, {
    x: 0,
    y: FLOOR_HEIGHT_OFFSET / 2,
    z: FLOOR_SIZE / 2,
  }),
  dimensions: {
    x: FLOOR_SIZE,
    y: WALL_HEIGHT,
    z: WALL_WIDTH
  },
  color: {
    red: 120,
    green: 100,
    blue: 120
  }
});

var spawnInterval = Script.setInterval(spawnBalls, SPAWN_TIME);

function spawnBalls() {
  balls.push(Entities.addEntity({
    type: "Sphere",
    shapeType: "sphere",
    position: {
      x: randFloat(bounds.xMin, bounds.xMax),
      y: BALL_DROP_HEIGHT,
      z: randFloat(bounds.zMin, bounds.zMax)
    },
    dimensions: {
      x: BALL_RADIUS,
      y: BALL_RADIUS,
      z: BALL_RADIUS
    },
    color: {
      red: randFloat(100, 150),
      green: randFloat(20, 80),
      blue: randFloat(10, 180)
    },
    ignoreCollisions: false,
    dynamic: true,
    gravity: {
      x: 0,
      y: -9.9,
      z: 0
    },
    velocity: {
      x: 0,
      y: -.25,
      z: 0
    },
    collisionSoundURL: SOUND_PATH + soundURLS[randInt(0, soundURLS.length - 1)]
  }));
  spawnCount++;
  if (spawnCount === NUM_BALLS) {
    Script.clearInterval(spawnInterval);
  }
}

function mousePressEvent(event) {
  var clickedOverlay = Overlays.getOverlayAtPoint({
    x: event.x,
    y: event.y
  });
  if (clickedOverlay == brownianButton) {
    brownianMotionActivated = !brownianMotionActivated;
    if (brownianMotionActivated) {
      brownianInterval = Script.setInterval(bumpBalls, BROWNIAN_INTERVAL_TIME);
      Overlays.editOverlay(brownianButton, {
        color: buttonOnColor
      })
    } else {
      Script.clearInterval(brownianInterval);
      Overlays.editOverlay(brownianButton, {
        color: buttonOffColor
      })
    }
  }
}

function bumpBalls() {
  balls.forEach(function(ball) {
    var props = Entities.getEntityProperties(ball);
    var newVelocity = Vec3.sum(props.velocity, {
      x: (Math.random() - 0.5) * BROWNIAN_FORCE_RANGE,
      y: 0,
      z: (Math.random() - 0.5) * BROWNIAN_FORCE_RANGE
    });
    Entities.editEntity(ball, {
      velocity: newVelocity
    });
  });
}

function cleanup() {
  Entities.deleteEntity(floor);
  Entities.deleteEntity(rightWall);
  Entities.deleteEntity(leftWall);
  Entities.deleteEntity(backWall);
  Entities.deleteEntity(frontWall);
  balls.forEach(function(ball) {
    Entities.deleteEntity(ball);
  });
  Overlays.deleteOverlay(brownianButton);
}

function randFloat(low, high) {
  return Math.floor(low + Math.random() * (high - low));
}


function randInt(low, high) {
  return Math.floor(randFloat(low, high));
}

Script.scriptEnding.connect(cleanup);


Script.scriptEnding.connect(cleanup);
Controller.mousePressEvent.connect(mousePressEvent);
