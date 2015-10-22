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

// add the fresh bat at home plate
var robot = Entities.addEntity({
    name: 'Robot',
    type: "Model",
    modelURL: ROBOT_MODEL,
    position: ROBOT_POSITION,
    animation: {
      url: ROBOT_MODEL
    }
});

// hook the update so we can check controller triggers
Script.update.connect(checkTriggers);
