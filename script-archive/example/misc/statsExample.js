//
//  statsExample.js
//  examples/example/misc
//
//  Created by Thijs Wenker on 24 Sept 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Prints the stats to the console.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// The stats to be displayed
var stats = [
    'serverCount',
    'framerate',   // a.k.a. FPS
    'simrate',
    'avatarSimrate',
    'avatarCount',
    'packetInCount',
    'packetOutCount',
    'mbpsIn',
    'mbpsOut',
    'audioPing',
    'avatarPing',
    'entitiesPing',
    'assetPing',
    'velocity',
    'yaw',
    'avatarMixerKbps',
    'avatarMixerPps',
    'audioMixerKbps',
    'audioMixerPps',
    'downloads',
    'downloadsPending',
    'triangles',
    'quads',
    'materialSwitches',
    'meshOpaque',
    'meshTranslucent',
    'opaqueConsidered',
    'opaqueOutOfView',
    'opaqueTooSmall',
    'translucentConsidered',
    'translucentOutOfView',
    'translucentTooSmall',
    'sendingMode',
    'packetStats',
    'lodStatus',
    'timingStats',
    'serverElements',
    'serverInternal',
    'serverLeaves',
    'localElements',
    'localInternal',
    'localLeaves'
];

// Force update the stats, in case the stats panel is invisible
Stats.forceUpdateStats();

// Loop through the stats and display them
for (var i in stats) {
    var stat = stats[i];
    print(stat + " = " + Stats[stat]);
}
