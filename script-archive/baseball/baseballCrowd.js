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
var extras = [
    SoundCache.getSound("atp:1ee58f4d929fdef7c2989cd8be964952a24cdd653d80f57b6a89a2ae8e3029e1.wav"), // zorba
    SoundCache.getSound("atp:cefaba2d5f1a378382ee046716bffcf6b6a40676649b23a1e81a996efe22d7d3.wav"), // charge
    SoundCache.getSound("atp:fb41e37f8f8f7b78e546ac78800df6e39edaa09b2df4bfa0afdd8d749dac38b8.wav"), // take me out to the ball game
    SoundCache.getSound("atp:44a83a788ccfd2924e35c902c34808b24dbd0309d000299ce01a355f91cf8115.wav") // clapping
];

var CHATTER_VOLUME = 0.20
var EXTRA_VOLUME = 0.25

function playChatter() {
    if (chatter.downloaded && !chatter.playing) {
        Audio.playSound(chatter, { loop: true, volume: CHATTER_VOLUME });
    }
}

chatter.ready.connect(playChatter);

var currentInjector = null;

function playRandomExtras() {
    if ((!currentInjector || !currentInjector.playing) && (Math.random() < (1.0 / 1800.0))) {
        // play a random extra sound about every 30s
        currentInjector = Audio.playSound(
            extras[Math.floor(Math.random() * extras.length)],
            { volume: EXTRA_VOLUME }
        );
    }
}

Script.update.connect(playRandomExtras);
