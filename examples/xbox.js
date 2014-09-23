//
//  xbox.js
//  examples
//
//  Created by Stephen Birarda on September 23, 2014
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

gamepad = Controller.joystick();
print("THE GAMEPAD NAME is " + gamepad.name);
print("THE GAMEPAD HAS " + gamepad.numAxes + " AXES")

function printValues() {
  controllerAxes = gamepad.axes;
  for (i = 0; i < controllerAxes.size; i++) {
    // print("The value for axis " + i + " is " + controllerAxes[i]);
  }
}

Script.update.connect(printValues);