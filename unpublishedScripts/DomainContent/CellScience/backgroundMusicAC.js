var soundMap = [{
        name: 'Cells',
        url: "https://cdn-1.vircadia.com/us-e-1/DomainContent/CellScience/Audio/Cells.wav",
        audioOptions: {
            position: {
                x: 15850,
                y: 15850,
                z: 15850
            },
            volume: 0.03,
            loop: true
        }
    }, {
        name: 'Cell Layout',
        url: "https://cdn-1.vircadia.com/us-e-1/DomainContent/CellScience/Audio/CellLayout.wav",
        audioOptions: {
            position: {
                x: 15950,
                y: 15950,
                z: 15950
            },
            volume: 0.03,
            loop: true
        }
    }, {
        name: 'Ribsome',
        url: "https://cdn-1.vircadia.com/us-e-1/DomainContent/CellScience/Audio/Ribosome.wav",
        audioOptions: {
            position: {
                x: 15650,
                y: 15650,
                z: 15650
            },
            volume: 0.03,
            loop: true
        }
    }, {
        name: 'Hexokinase',
        url: "https://cdn-1.vircadia.com/us-e-1/DomainContent/CellScience/Audio/Hexokinase.wav",
        audioOptions: {
            position: {
                x: 15750,
                y: 15750,
                z: 15750
            },
            volume: 0.03,
            loop: true
        }
    }

];


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
        } catch (e) {
            print('error playing sound' + e)
        }
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