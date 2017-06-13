//
// hmdRollControlEnable.js
//
// Created by David Rowe on 4 Jun 2017.
// Copyright 2017 High Fidelity, Inc.
//
// Distributed under the Apache License, Version 2.0
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var hmdRollControlEnabled = true;
var hmdRollControlDeadZone = 8.0;  // deg
var hmdRollControlSpeed = 1.0;  // deg/sec/deg

print("HMD roll control: " + hmdRollControlEnabled + ", " + hmdRollControlDeadZone + ", " + hmdRollControlSpeed);

MyAvatar.hmdRollControlEnabled = hmdRollControlEnabled;
MyAvatar.hmdRollControlDeadZone = hmdRollControlDeadZone;
MyAvatar.hmdRollControlSpeed = hmdRollControlSpeed;

Script.stop();
