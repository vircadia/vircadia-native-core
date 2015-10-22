//
//  baseball.js
//  examples/toys
//
//  Created by Stephen Birarda on 10/20/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var ROBOT_MODEL = "atp:ea02100c2ee63a8b9c0495557f32041be18ec94def157592e84a816665ce2f6e.fbx";
var ROBOT_POSITION = { x: -0.54, y: 1.21, z: 2.57 }

var BAT_MODEL = "atp:07bdd769a57ff15ebe9331ae4e2c2eae8886a6792b4790cce03b4716eb3a81c7.fbx"
var BAT_COLLISION_MODEL = "atp:1211ee12bc8ab0bb744e8582e15e728a00ca70a808550fc46d7284799b9a868a.obj"

// add the fresh robot at home plate
var robot = Entities.addEntity({
    name: 'Robot',
    type: 'Model',
    modelURL: ROBOT_MODEL,
    position: ROBOT_POSITION,
    animation: {
      url: ROBOT_MODEL,
      running: true
    }
});

// add the bat
var bat = Entities.addEntity({
  name: 'Bat',
  type: 'Model',
  modelURL: BAT_MODEL
})

var lastTriggerValue = 0.0;

function checkTriggers() {
  var rightTrigger = Controller.getTriggerValue(1);

  if (rightTrigger == 0) {
    if (lastTriggerValue > 0) {
      // the trigger was just released, play out to the last frame of the swing
      Entities.editEntity(robot, {
        animation: {
          running: true,
          currentFrame: 21,
          lastFrame: 115
        }
      });
    }
  } else {
    if (lastTriggerValue == 0) {
      // the trigger was just depressed, start the swing
      Entities.editEntity(robot, {
        animation: {
          running: true,
          currentFrame: 0,
          lastFrame: 21
        }
      });
    }
  }

  lastTriggerValue = rightTrigger;
}

var DISTANCE_HOLDING_ACTION_TIMEFRAME = 0.1; // how quickly objects move to their new position
var ACTION_LIFETIME = 15; // seconds

function moveBat() {
  var forearmPosition = Entities.getJointPosition(robot, 40);
  var forearmRotation = Entities.getJointRotation(robot, 40);

  Vec3.print("forearmPosition=", forearmPosition);

  // Entities.addAction("spring", bat, {
  //   targetPosition: forearmPosition,
  //   targetRotation: forearmRotation,
  //   linearTimeScale: DISTANCE_HOLDING_ACTION_TIMEFRAME,
  //   angularTimeScale: DISTANCE_HOLDING_ACTION_TIMEFRAME,
  //   lifetime: ACTION_LIFETIME
  // });

  Entities.editEntity(bat, {
    position: forearmPosition,
    rotation: forearmRotation
  });
}

function update() {
  checkTriggers();
  moveBat();
}

// hook the update so we can check controller triggers
Script.update.connect(update);
