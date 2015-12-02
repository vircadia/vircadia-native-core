"use strict";
/*jslint vars: true, plusplus: true*/
/*global Agent, Avatar, Script, Entities, Vec3, Quat, print*/
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
var turnSpread = 90; // How many degrees should turn from front range over.
var animationData = {url: "https://hifi-public.s3.amazonaws.com/ozan/anim/standard_anims/walk_fwd.fbx", lastFrame: 35};
var models = [ // Commented-out avatars do not animate properly on AC's. Presumably because ScriptableAvatar doesn't use model pre-rotations.
    "https://hifi-public.s3.amazonaws.com/ozan/avatars/hifi_team/alan/alan.fst",
    "https://hifi-public.s3.amazonaws.com/ozan/avatars/hifi_team/andrew/andrew.fst",
    "https://hifi-public.s3.amazonaws.com/ozan/avatars/hifi_team/austin/austin.fst",
    "https://hifi-public.s3.amazonaws.com/ozan/avatars/hifi_team/birarda/birarda.fst",
    "https://hifi-public.s3.amazonaws.com/ozan/avatars/hifi_team/brad/brad.fst",
    "https://hifi-public.s3.amazonaws.com/ozan/avatars/hifi_team/chris/chris2.fst",
    "https://hifi-public.s3.amazonaws.com/ozan/avatars/hifi_team/clement/clement.fst",
    "https://hifi-public.s3.amazonaws.com/ozan/avatars/hifi_team/emily/emily.fst",
    "https://hifi-public.s3.amazonaws.com/ozan/avatars/hifi_team/ewing/ewing.fst",
    "https://hifi-public.s3.amazonaws.com/ozan/avatars/hifi_team/howard/howard.fst",
    "https://hifi-public.s3.amazonaws.com/ozan/avatars/hifi_team/huffman/huffman.fst",
    "https://hifi-public.s3.amazonaws.com/ozan/avatars/hifi_team/james/james.fst",
    "https://hifi-public.s3.amazonaws.com/ozan/avatars/hifi_team/philip/philip.fst",
    "https://hifi-public.s3.amazonaws.com/ozan/avatars/hifi_team/ryan/ryan.fst",
    "https://hifi-public.s3.amazonaws.com/ozan/avatars/hifi_team/sam/sam.fst",
    "https://hifi-public.s3.amazonaws.com/ozan/avatars/hifi_team/tony/tony.fst",

    "https://hifi-metaverse.s3-us-west-1.amazonaws.com/marketplace/contents/1e57c395-612e-4acd-9561-e79dbda0bc49/faafb83c63a3e5e265884d270fc29f8b.fst",
    "https://hifi-metaverse.s3-us-west-1.amazonaws.com/marketplace/contents/615ca447-06ee-4215-8dd1-a609c2fcd0cd/c7af6d4224c501fdd9cb54e0101ff281.fst",
    "https://hifi-metaverse.s3-us-west-1.amazonaws.com/marketplace/contents/731c39d7-559a-4ce2-947c-3e2768f5471c/8d5eba2fd5bf068259556aec1861c5dd.fst",
    "https://hifi-metaverse.s3-us-west-1.amazonaws.com/marketplace/contents/8bdaa1ec-99df-4a29-a249-6941c7fd1930/37351a18a3dea468088fc3d822aaf29c.fst",
    "https://hifi-metaverse.s3-us-west-1.amazonaws.com/marketplace/contents/0909d7b7-c67e-45fb-acd9-a07380dc6b9c/da76b8c59dbc680bdda90df4b9a46faa.fst",
    "https://hifi-metaverse.s3-us-west-1.amazonaws.com/marketplace/contents/ad0dffd7-f811-487b-a20a-2509235491ef/29106da1f7e6a42c7907603421fd7df5.fst",
    "https://hifi-metaverse.s3-us-west-1.amazonaws.com/marketplace/contents/3ebe5c84-8b88-4d91-86ac-f104f3620fe3/3534b032d079514aa8960a316500ce13.fst",
    "https://hifi-metaverse.s3-us-west-1.amazonaws.com/marketplace/contents/dd03b8e3-52fb-4ab3-9ac9-3b17e00cd85d/98baa90b3b66803c5d7bd4537fca6993.fst",
    "https://hifi-metaverse.s3-us-west-1.amazonaws.com/marketplace/contents/ff060580-2fc7-4b6c-8e12-f023d05363cf/dadef29b1e60f23b413d1850d7e0dd4a.fst",
    "https://hifi-metaverse.s3-us-west-1.amazonaws.com/marketplace/contents/b55d3baf-4eb3-4cac-af4c-0fb66d0c907b/ad2c9157f3924ab1f7f6ea87a8cce6cc.fst",
    "https://hifi-metaverse.s3-us-west-1.amazonaws.com/marketplace/contents/d029ae8d-2905-4eb7-ba46-4bd1b8cb9d73/4618d52e711fbb34df442b414da767bb.fst"
];
var nameMap = {
    faafb83c63a3e5e265884d270fc29f8b: 'albert',
    c7af6d4224c501fdd9cb54e0101ff281: 'boss',
    '8d5eba2fd5bf068259556aec1861c5dd': 'dougland',
    '37351a18a3dea468088fc3d822aaf29c': 'fightbot blue',
    da76b8c59dbc680bdda90df4b9a46faa: 'judd',
    '29106da1f7e6a42c7907603421fd7df5': 'kate',
    '3534b032d079514aa8960a316500ce13': 'lenny',
    '98baa90b3b66803c5d7bd4537fca6993': 'lovejoy',
    dadef29b1e60f23b413d1850d7e0dd4a: 'mery', // eyes no good
    ad2c9157f3924ab1f7f6ea87a8cce6cc: 'owen',
    '4618d52e711fbb34df442b414da767bb': 'will'
};

Avatar.skeletonModelURL = models[Math.round(Math.random() * (models.length - 1))]; // pick one
Avatar.displayName = Avatar.skeletonModelURL.match(/\/(\w*).fst/)[1];  // grab the file basename
Avatar.displayName = nameMap[Avatar.displayName] || Avatar.displayName;
var millisecondsToWaitBeforeStarting = 10 * 1000; // To give the various servers a chance to start.

Agent.isAvatar = true;
function coord() { return (Math.random() * spread) - (spread / 2); }  // randomly distribute a coordinate zero += spread/2.
Script.setTimeout(function () {
    Avatar.position = Vec3.sum(origin, {x: coord(), y: 0, z: coord()});
    Avatar.orientation = Quat.fromPitchYawRollDegrees(0, turnSpread * (Math.random() - 0.5), 0);
    print("Starting at", JSON.stringify(Avatar.position));
    Avatar.startAnimation(animationData.url, animationData.fps || 30, 1, true, false, animationData.firstFrame || 0, animationData.lastFrame);
}, millisecondsToWaitBeforeStarting);
