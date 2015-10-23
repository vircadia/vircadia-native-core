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

var crowd1 = SoundCache.getSound("atp:0e921b644464d56d5b412ea2ea1d83f8ff3f7506c4b0471ea336a4770daf3b82.wav");

function maybePlaySound(deltaTime) {
    if (crowd1.downloaded && !crowd1.isPlaying) {
        Audio.playSound(crowd1, { position: { x: 0, y: 0, z: 0}, loop: true});
        Script.update.disconnect(maybePlaySound);
    }
}

Script.update.connect(maybePlaySound);
