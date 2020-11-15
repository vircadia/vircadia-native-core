//
//  toys/AC_scripts/toybox_sounds.js
//
//  This script adds several sounds to the correct locations for toybox.
//  By James B. Pollack @imgntn 10/21/2015
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var soundMap = [{
    name: 'river water',
    url: "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/sounds/Water_Lap_River_Edge_Gentle.L.wav",
    audioOptions: {
        position: {
            x: 580,
            y: 493,
            z: 528
        },
        volume: 0.4,
        loop: true
    }
}, {
    name: 'windmill',
    url: "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/sounds/WINDMILL_Mono.wav",
    audioOptions: {
        position: {
            x: 530,
            y: 516,
            z: 518
        },
        volume: 0.08,
        loop: true
    }
}, {
    name: 'insects',
    url: "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/sounds/insects3.wav",
    audioOptions: {
        position: {
            x: 560,
            y: 495,
            z: 474
        },
        volume: 0.25,
        loop: true
    }
}, {
    name: 'fireplace',
    url: "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/sounds/0619_Fireplace__Tree_B.L.wav",
    audioOptions: {
        position: {
            x: 551.61,
            y: 494.88,
            z: 502.00
        },
        volume: 0.25,
        loop: true
    }
}, {
    name: 'cat purring',
    url: "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/sounds/Cat_Purring_Deep_Low_Snor.wav",
    audioOptions: {
        position: {
            x: 551.48,
            y: 495.60,
            z: 502.08
        },
        volume: 0.03,
        loop: true
    }
}, {
    name: 'dogs barking',
    url: "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/sounds/dogs_barking_1.L.wav",
    audioOptions: {
        position: {
            x: 523,
            y: 494.88,
            z: 469
        },
        volume: 0.05,
        loop: false
    },
    playAtInterval: 60 * 1000
}, {
    name: 'arcade game',
    url: "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/sounds/ARCADE_GAMES_VID.L.L.wav",
    audioOptions: {
        position: {
            x: 543.77,
            y: 495.07,
            z: 502.25
        },
        volume: 0.01,
        loop: false,
    },
    playAtInterval: 90 * 1000
}];

function loadSounds() {
    soundMap.forEach(function(soundData) {
        soundData.sound = SoundCache.getSound(soundData.url);
    });
}

function playSound(soundData) {
    if (soundData.injector) {
        // try/catch in case the injector QObject has been deleted already
        try {
            soundData.injector.stop();
        } catch (e) {}
    }
    soundData.injector = Audio.playSound(soundData.sound, soundData.audioOptions);
}

function checkDownloaded(soundData) {
    if (soundData.sound.downloaded) {

        Script.clearInterval(soundData.downloadTimer);

        if (soundData.hasOwnProperty('playAtInterval')) {
            soundData.playingInterval = Script.setInterval(function() {
                playSound(soundData)
            }, soundData.playAtInterval);
        } else {
            playSound(soundData);
        }

    }
}

function startCheckDownloadedTimers() {
    soundMap.forEach(function(soundData) {
        soundData.downloadTimer = Script.setInterval(function() {
            checkDownloaded(soundData);
        }, 1000);
    });
}

Script.scriptEnding.connect(function() {
    soundMap.forEach(function(soundData) {

        if (soundData.hasOwnProperty("injector")) {
            soundData.injector.stop();
        }

        if (soundData.hasOwnProperty("downloadTimer")) {
            Script.clearInterval(soundData.downloadTimer);
        }

        if (soundData.hasOwnProperty("playingInterval")) {
            Script.clearInterval(soundData.playingInterval);
        }

    });

});

loadSounds();
startCheckDownloadedTimers();