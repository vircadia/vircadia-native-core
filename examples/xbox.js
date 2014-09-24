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

joysticks = Joysticks.availableJoystickNames;

for (i = 0; i < joysticks.length; i++) {
  print("Joystick " + i + " is " + joysticks[i]);
}