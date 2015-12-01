"use strict";
/*jslint vars: true, plusplus: true*/
/*global Agent, Avatar, Script, Entities, Vec3, print*/
//
//  animatedAvatar.js
//  examples/acScripts
//
//  Created by Howard Stearns 11/6/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
// An assignment client script that animates one avatar at random location within 'spread' meters of 'origin'.
// In Domain Server Settings, go to scripts and give the url of this script. Press '+', and then 'Save and restart'.

var origin = {x: 500, y: 500, z: 500};
var spread = 20; // meters
var animationData = {url: "https://hifi-public.s3.amazonaws.com/ozan/anim/standard_anims/walk_fwd.fbx", lastFrame: 35};
Avatar.skeletonModelURL = "https://hifi-public.s3.amazonaws.com/marketplace/contents/dd03b8e3-52fb-4ab3-9ac9-3b17e00cd85d/98baa90b3b66803c5d7bd4537fca6993.fst"; //lovejoy
Avatar.displayName = "'Bot";
var millisecondsToWaitBeforeStarting = 10 * 1000; // To give the various servers a chance to start.

Agent.isAvatar = true;
function coord() { return (Math.random() * spread) - (spread / 2); }  // randomly distribute a coordinate zero += spread/2.
Script.setTimeout(function () {
    Avatar.position = Vec3.sum(origin, {x: coord(), y: 0, z: coord()});
    print("Starting at", JSON.stringify(Avatar.position));
    Avatar.startAnimation(animationData.url, animationData.fps || 30, 1, true, false, animationData.firstFrame || 0, animationData.lastFrame);
}, millisecondsToWaitBeforeStarting);
