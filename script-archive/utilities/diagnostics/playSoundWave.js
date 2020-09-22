//
//  playSoundWave.js
//  examples
//
//  Created by Ryan Huffman on 05/27/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var soundClip = SoundCache.getSound(Script.getExternalPath(Script.ExternalPaths.Assets, "sounds/Cocktail%20Party%20Snippets/Walken1.wav"));

function playSound() {
    Audio.playSound(soundClip, {
      position: MyAvatar.position,
      volume: 0.5
    });
}

Script.setInterval(playSound, 10000);
