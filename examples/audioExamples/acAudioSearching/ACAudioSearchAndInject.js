//
//  ACAudioSearchAndInject.js
//  audio
//
//  Created by Eric Levin 2/1/2016
//  Copyright 2016 High Fidelity, Inc.

//  This AC script constantly searches for entities with a special userData field that specifies audio settings, and then
//  injects the sound with the specified URL with other specified settings (playback volume) or playback at interval, or random interval
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("https://rawgit.com/highfidelity/hifi/master/examples/libraries/utils.js");
var SOUND_DATA_KEY = "soundKey";


EntityViewer.setPosition({
    x: 0,
    y: 0,
    z: 0
});
EntityViewer.setKeyholeRadius(60000);
EntityViewer.queryOctree();
Entities.setPacketsPerSecond(6000);

Script.setInterval(searchForSoundEntities, 1000);

function searchForSoundEntities() {
    var entities = Entities.findEntities({
        x: 0,
        y: 0,
        z: 0
    }, 16000)
    print("EBL ENTITIES FOUND " + entities.length);
    entities.forEach(function(entity) {
        var soundData = getEntityCustomData(SOUND_DATA_KEY, entity);
        if (soundData && soundData.url) {
            playSound(soundData);
        }
    });
}

function playSound(soundData) {
    var sound = SoundCache.getSound(soundData.url);
    sound.ready.connect(function() {
        print("EBL PLAY SOUND")
        Audio.playSound(sound, {
            position: {
                x: 0,
                y: 0,
                z: 0
            },
            volume: 1.0,
            loop: false
        });
    })

}