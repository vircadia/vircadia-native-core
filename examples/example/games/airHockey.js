// 
//  AirHockey.js 
//  
//  Created by Philip Rosedale on January 26, 2015
//  Copyright 2015 High Fidelity, Inc.
//  
//  AirHockey table and pucks
// 
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var debugVisible = false;

var FIELD_WIDTH = 1.21;
var FIELD_LENGTH = 1.92;
var FLOOR_THICKNESS = 0.20;
var EDGE_THICKESS = 0.10;
var EDGE_HEIGHT = 0.10;
var DROP_HEIGHT = 0.3;
var PUCK_SIZE = 0.15;
var PUCK_THICKNESS = 0.05;
var PADDLE_SIZE = 0.15;
var PADDLE_THICKNESS = 0.05;

var ENTITY_SEARCH_RANGE = 500;

var GOAL_WIDTH = 0.35;

var GRAVITY = -9.8;
var LIFETIME = 6000;
var PUCK_DAMPING = 0.02;
var PADDLE_DAMPING = 0.35;
var ANGULAR_DAMPING = 0.4;
var PADDLE_ANGULAR_DAMPING = 0.75;
var MODEL_SCALE = 1.52;
var MODEL_OFFSET = {
  x: 0,
  y: -0.19,
  z: 0
};

var LIGHT_OFFSET = {
  x: 0,
  y: 0.2,
  z: 0
};

var LIGHT_FLASH_TIME = 700;

var scoreSound = SoundCache.getSound("https://s3.amazonaws.com/hifi-public/sounds/Collisions-hitsandslaps/airhockey_score.wav");

var polyTable = "https://hifi-public.s3.amazonaws.com/ozan/props/airHockeyTable/airHockeyTableForPolyworld.fbx"
var normalTable = "https://hifi-public.s3.amazonaws.com/ozan/props/airHockeyTable/airHockeyTable.fbx"
var hitSound1 = "https://s3.amazonaws.com/hifi-public/sounds/Collisions-hitsandslaps/airhockey_hit1.wav"
var hitSound2 = "https://s3.amazonaws.com/hifi-public/sounds/Collisions-hitsandslaps/airhockey_hit2.wav"
var hitSideSound = "https://s3.amazonaws.com/hifi-public/sounds/Collisions-hitsandslaps/airhockey_hit3.wav"
var puckModel = "https://hifi-public.s3.amazonaws.com/ozan/props/airHockeyTable/airHockeyPuck.fbx"
var puckCollisionModel = "http://headache.hungry.com/~seth/hifi/airHockeyPuck-hull.obj"
var paddleModel = "https://hifi-public.s3.amazonaws.com/ozan/props/airHockeyTable/airHockeyPaddle.obj"
var paddleCollisionModel = "http://headache.hungry.com/~seth/hifi/paddle-hull.obj"

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";
var screenSize = Controller.getViewportDimensions();
var BUTTON_SIZE = 32;
var PADDING = 3;

var center;

var edgeRestitution = 0.9;
var floorFriction = 0.01;

var paddle1Pos, paddle2Pos;
var names = ['floor', 'table', 'paddle', 'edge', 'puck', 'hockeyLight'];

var deleteButton = Overlays.addOverlay("image", {
  x: screenSize.x / 2 - BUTTON_SIZE,
  y: screenSize.y - (BUTTON_SIZE * 2 + PADDING),
  width: BUTTON_SIZE,
  height: BUTTON_SIZE,
  imageURL: HIFI_PUBLIC_BUCKET + "images/delete.png",
  color: {
    red: 255,
    green: 255,
    blue: 255
  },
  alpha: 1
});

var spawnButton = Overlays.addOverlay("image", {
  x: screenSize.x / 2 + PADDING,
  y: screenSize.y - (BUTTON_SIZE * 2 + PADDING),
  width: BUTTON_SIZE,
  height: BUTTON_SIZE,
  imageURL: HIFI_PUBLIC_BUCKET + "images/puck.png",
  color: {
    red: 255,
    green: 255,
    blue: 255
  },
  alpha: 1
});



var floor, edge1, edge2, edge3a, edge3b, edge4a, edge4b, light;
var puck;
var paddle1, paddle2;

//  Create pucks 

function makeNewProp(which, position) {
  if (which == "puck") {
    return Entities.addEntity({
      name: 'puck',
      type: "Model",
      modelURL: puckModel,
      compoundShapeURL: puckCollisionModel,
      collisionSoundURL: hitSound1,
      position: Vec3.sum(center, {
        x: 0,
        y: DROP_HEIGHT,
        z: 0
      }),
      dimensions: {
        x: PUCK_SIZE,
        y: PUCK_THICKNESS,
        z: PUCK_SIZE
      },
      gravity: {
        x: 0,
        y: GRAVITY,
        z: 0
      },
      velocity: {
        x: 0,
        y: 0.05,
        z: 0
      },
      ignoreCollisions: false,
      damping: PUCK_DAMPING,
      angularDamping: ANGULAR_DAMPING,
      lifetime: LIFETIME,
      collisionsWillMove: true
    });
  } else if (which == "paddle1") {
    paddle1Pos = Vec3.sum(center, {
      x: 0,
      y: DROP_HEIGHT * 1.5,
      z: FIELD_LENGTH * 0.35
    });
    return Entities.addEntity({
      name: "paddle",
      type: "Model",
      modelURL: paddleModel,
      compoundShapeURL: paddleCollisionModel,
      collisionSoundURL: hitSound2,
      position: paddle1Pos,
      dimensions: {
        x: PADDLE_SIZE,
        y: PADDLE_THICKNESS,
        z: PADDLE_SIZE
      },
      gravity: {
        x: 0,
        y: GRAVITY,
        z: 0
      },
      velocity: {
        x: 0,
        y: 0.07,
        z: 0
      },
      ignoreCollisions: false,
      damping: PADDLE_DAMPING,
      angularDamping: PADDLE_ANGULAR_DAMPING,
      lifetime: LIFETIME,
      collisionsWillMove: true
    });
  } else if (which == "paddle2") {
    paddle2Pos = Vec3.sum(center, {
      x: 0,
      y: DROP_HEIGHT * 1.5,
      z: -FIELD_LENGTH * 0.35
    });
    return Entities.addEntity({
      name: "paddle",
      type: "Model",
      modelURL: paddleModel,
      compoundShapeURL: paddleCollisionModel,
      collisionSoundURL: hitSound2,
      position: paddle2Pos,
      dimensions: {
        x: PADDLE_SIZE,
        y: PADDLE_THICKNESS,
        z: PADDLE_SIZE
      },
      gravity: {
        x: 0,
        y: GRAVITY,
        z: 0
      },
      velocity: {
        x: 0,
        y: 0.07,
        z: 0
      },
      ignoreCollisions: false,
      damping: PADDLE_DAMPING,
      angularDamping: PADDLE_ANGULAR_DAMPING,
      lifetime: LIFETIME,
      collisionsWillMove: true
    });
  }
}



function update(deltaTime) {
  if (Math.random() < 0.1) {
    puckProps = Entities.getEntityProperties(puck);
    paddle1Props = Entities.getEntityProperties(paddle1);
    paddle2Props = Entities.getEntityProperties(paddle2);
    if (puckProps.position.y < (center.y - DROP_HEIGHT)) {
      score();
    }

    if (paddle1Props.position.y < (center.y - DROP_HEIGHT)) {
      Entities.deleteEntity(paddle1);
      paddle1 = makeNewProp("paddle1");
    }
    if (paddle2Props.position.y < (center.y - DROP_HEIGHT)) {
      Entities.deleteEntity(paddle2);
      paddle2 = makeNewProp("paddle2");
    }
  }
}

function score() {
  Audio.playSound(scoreSound, {
    position: center,
    volume: 1.0
  });
  puckDropPosition = Entities.getEntityProperties(puck).position;
  var newPosition;
  if (Vec3.distance(puckDropPosition, paddle1Pos) > Vec3.distance(puckDropPosition, paddle2Pos)) {
    newPosition = paddle2Pos;
  } else {
    newPosition = paddle1Pos;
  }
  Entities.editEntity(puck, {
    position: newPosition,
    velocity: {
      x: 0,
      y: 0.05,
      z: 0
    }
  });

  Entities.editEntity(light, {
    visible: true
  });
  Script.setTimeout(function() {
    Entities.editEntity(light, {
      visible: false
    });
  }, LIGHT_FLASH_TIME);
}

function mousePressEvent(event) {
  var clickedOverlay = Overlays.getOverlayAtPoint({
    x: event.x,
    y: event.y
  });
  if (clickedOverlay == spawnButton) {
    spawnAllTheThings();
  } else if (clickedOverlay == deleteButton) {
    deleteAllTheThings();
  }
}

function spawnAllTheThings() {
  center = Vec3.sum(MyAvatar.position, Vec3.multiply((FIELD_WIDTH + FIELD_LENGTH) * 0.60, Quat.getFront(Camera.getOrientation())));
  floor = Entities.addEntity({
    name: "floor",
    type: "Box",
    position: Vec3.subtract(center, {
      x: 0,
      y: 0,
      z: 0
    }),
    dimensions: {
      x: FIELD_WIDTH,
      y: FLOOR_THICKNESS,
      z: FIELD_LENGTH
    },
    color: {
      red: 128,
      green: 128,
      blue: 128
    },
    gravity: {
      x: 0,
      y: 0,
      z: 0
    },
    ignoreCollisions: false,
    locked: true,
    friction: floorFriction,
    visible: debugVisible,
    lifetime: LIFETIME
  });

  edge1 = Entities.addEntity({
    name: 'edge',
    type: "Box",
    collisionSoundURL: hitSideSound,
    position: Vec3.sum(center, {
      x: FIELD_WIDTH / 2.0,
      y: FLOOR_THICKNESS / 2.0,
      z: 0
    }),
    dimensions: {
      x: EDGE_THICKESS,
      y: EDGE_HEIGHT,
      z: FIELD_LENGTH + EDGE_THICKESS
    },
    color: {
      red: 100,
      green: 100,
      blue: 100
    },
    gravity: {
      x: 0,
      y: 0,
      z: 0
    },
    ignoreCollisions: false,
    visible: debugVisible,
    restitution: edgeRestitution,
    locked: true,
    lifetime: LIFETIME
  });

  edge2 = Entities.addEntity({
    name: 'edge',
    type: "Box",
    collisionSoundURL: hitSideSound,
    position: Vec3.sum(center, {
      x: -FIELD_WIDTH / 2.0,
      y: FLOOR_THICKNESS / 2.0,
      z: 0
    }),
    dimensions: {
      x: EDGE_THICKESS,
      y: EDGE_HEIGHT,
      z: FIELD_LENGTH + EDGE_THICKESS
    },
    color: {
      red: 100,
      green: 100,
      blue: 100
    },
    gravity: {
      x: 0,
      y: 0,
      z: 0
    },
    ignoreCollisions: false,
    visible: debugVisible,
    restitution: edgeRestitution,
    locked: true,
    lifetime: LIFETIME
  });

  edge3a = Entities.addEntity({
    name: 'edge',
    type: "Box",
    collisionSoundURL: hitSideSound,
    position: Vec3.sum(center, {
      x: FIELD_WIDTH / 4.0 + (GOAL_WIDTH / 4.0),
      y: FLOOR_THICKNESS / 2.0,
      z: -FIELD_LENGTH / 2.0
    }),
    dimensions: {
      x: FIELD_WIDTH / 2.0 - GOAL_WIDTH / 2.0,
      y: EDGE_HEIGHT,
      z: EDGE_THICKESS
    },
    color: {
      red: 100,
      green: 100,
      blue: 100
    },
    gravity: {
      x: 0,
      y: 0,
      z: 0
    },
    ignoreCollisions: false,
    visible: debugVisible,
    restitution: edgeRestitution,
    locked: true,
    lifetime: LIFETIME
  });

  edge3b = Entities.addEntity({
    name: 'edge',
    type: "Box",
    collisionSoundURL: hitSideSound,
    position: Vec3.sum(center, {
      x: -FIELD_WIDTH / 4.0 - (GOAL_WIDTH / 4.0),
      y: FLOOR_THICKNESS / 2.0,
      z: -FIELD_LENGTH / 2.0
    }),
    dimensions: {
      x: FIELD_WIDTH / 2.0 - GOAL_WIDTH / 2.0,
      y: EDGE_HEIGHT,
      z: EDGE_THICKESS
    },
    color: {
      red: 100,
      green: 100,
      blue: 100
    },
    gravity: {
      x: 0,
      y: 0,
      z: 0
    },
    ignoreCollisions: false,
    visible: debugVisible,
    restitution: edgeRestitution,
    locked: true,
    lifetime: LIFETIME
  });

  edge4a = Entities.addEntity({
    name: 'edge',
    type: "Box",
    collisionSoundURL: hitSideSound,
    position: Vec3.sum(center, {
      x: FIELD_WIDTH / 4.0 + (GOAL_WIDTH / 4.0),
      y: FLOOR_THICKNESS / 2.0,
      z: FIELD_LENGTH / 2.0
    }),
    dimensions: {
      x: FIELD_WIDTH / 2.0 - GOAL_WIDTH / 2.0,
      y: EDGE_HEIGHT,
      z: EDGE_THICKESS
    },
    color: {
      red: 100,
      green: 100,
      blue: 100
    },
    gravity: {
      x: 0,
      y: 0,
      z: 0
    },
    ignoreCollisions: false,
    visible: debugVisible,
    restitution: edgeRestitution,
    locked: true,
    lifetime: LIFETIME
  });

  edge4b = Entities.addEntity({
    name: 'edge',
    type: "Box",
    collisionSoundURL: hitSideSound,
    position: Vec3.sum(center, {
      x: -FIELD_WIDTH / 4.0 - (GOAL_WIDTH / 4.0),
      y: FLOOR_THICKNESS / 2.0,
      z: FIELD_LENGTH / 2.0
    }),
    dimensions: {
      x: FIELD_WIDTH / 2.0 - GOAL_WIDTH / 2.0,
      y: EDGE_HEIGHT,
      z: EDGE_THICKESS
    },
    color: {
      red: 100,
      green: 100,
      blue: 100
    },
    gravity: {
      x: 0,
      y: 0,
      z: 0
    },
    ignoreCollisions: false,
    visible: debugVisible,
    restitution: edgeRestitution,
    locked: true,
    lifetime: LIFETIME
  });

  table = Entities.addEntity({
    name: "table",
    type: "Model",
    modelURL: polyTable,
    dimensions: Vec3.multiply({
      x: 0.8,
      y: 0.45,
      z: 1.31
    }, MODEL_SCALE),
    position: Vec3.sum(center, MODEL_OFFSET),
    ignoreCollisions: false,
    visible: true,
    locked: true,
    lifetime: LIFETIME
  });

  light = Entities.addEntity({
    name: "hockeyLight",
    type: "Light",
    dimensions: {
      x: 5,
      y: 5,
      z: 5
    },
    position: Vec3.sum(center, LIGHT_OFFSET),
    intensity: 5,
    color: {
      red: 200,
      green: 20,
      blue: 200
    },
    visible: false
  });
  puck = makeNewProp("puck");
  paddle1 = makeNewProp("paddle1");
  paddle2 = makeNewProp("paddle2");

  Script.update.connect(update);

}

function deleteAllTheThings() {

  Script.update.disconnect(update);
  //delete all nearby entitites that are named any the names from our names array
  var nearbyEntities = Entities.findEntities(MyAvatar.position, ENTITY_SEARCH_RANGE);
  for (var i = 0; i < nearbyEntities.length; i++) {
    var entityName = Entities.getEntityProperties(nearbyEntities[i]).name;
    for (var j = 0; j < names.length; j++) {
      if (names[j] === entityName) {
        //We have a mach- delete this entity
        Entities.editEntity(nearbyEntities[i], {
          locked: false
        });
        Entities.deleteEntity(nearbyEntities[i]);
      }
    }
  }


}

function scriptEnding() {

  Overlays.deleteOverlay(spawnButton);
  Overlays.deleteOverlay(deleteButton);

  Entities.editEntity(edge1, {
    locked: false
  });
  Entities.editEntity(edge2, {
    locked: false
  });
  Entities.editEntity(edge3a, {
    locked: false
  });
  Entities.editEntity(edge3b, {
    locked: false
  });
  Entities.editEntity(edge4a, {
    locked: false
  });
  Entities.editEntity(edge4b, {
    locked: false
  });
  Entities.editEntity(floor, {
    locked: false
  });
  Entities.editEntity(table, {
    locked: false
  });



  Entities.deleteEntity(edge1);
  Entities.deleteEntity(edge2);
  Entities.deleteEntity(edge3a);
  Entities.deleteEntity(edge3b);
  Entities.deleteEntity(edge4a);
  Entities.deleteEntity(edge4b);
  Entities.deleteEntity(floor);
  Entities.deleteEntity(puck);
  Entities.deleteEntity(paddle1);
  Entities.deleteEntity(paddle2);
  Entities.deleteEntity(table);
  Entities.deleteEntity(light);

}

Controller.mousePressEvent.connect(mousePressEvent);
Script.scriptEnding.connect(scriptEnding);