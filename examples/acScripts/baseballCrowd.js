//
//  baseballCrowd.js
//  examples/acScripts
//
//  Created by Stephen Birarda on 10/20/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var chatter = SoundCache.getSound("atp:d9978e693035d4e2b5c7b546c8cccfb2dde5677834d9eed5206ccb2da55b4732.wav");

function playChatter() {
    if (chatter.downloaded && !chatter.isPlaying) {
        Audio.playSound(chatter, { position: { x: 0, y: 0, z: 0}, loop: true, volume: 0.5 });
    }
}

chatter.ready.connect(playChatter);
