//
//  laserPointer.js
//  examples
//
//  Created by Clément Brisset on 7/18/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var LEFT = 0;
var RIGHT = 1;
var LEFT_HAND_FLAG = 1;
var RIGHT_HAND_FLAG = 2;

function update() {
    var state = ((Controller.getTriggerValue(LEFT) > 0.9) ? LEFT_HAND_FLAG : 0) +
                ((Controller.getTriggerValue(RIGHT) > 0.9) ? RIGHT_HAND_FLAG : 0);
    MyAvatar.setHandState(state);
}

Script.update.connect(update);