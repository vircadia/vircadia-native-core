//
//  largeHall.js
//  examples
//
//  Created by Freidrica on 4/1/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  This entity script invokes reverb upon entering an entity acting as a trigger zone

//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
(function() {
    var _this = this;
    print("EBL PRELOADING NEW VERSION ")
    var audioOptions = new AudioEffectOptions({
        bandwidth: 7000,
        preDelay: 80,
        lateDelay: 0,
        reverbTime: 3,
        earlyDiffusion: 100,
        lateDiffusion: 100,
        roomSize: 50,
        density: 100,
        bassMult: 1.5,
        bassFreq: 250,
        highGain: -12,
        highFreq: 3000,
        modRate: 2.3,
        modDepth: 50,
        earlyGain: -12,
        lateGain: -12,
        earlyMixLeft: 20,
        earlyMixRight: 20,
        lateMixLeft: 90,
        lateMixRight: 90,
        wetDryMix: 90,
    });

    function setter(name) {
        return function(value) {
            audioOptions[name] = value;
            AudioDevice.setReverbOptions(audioOptions);
        }
    }

    function getter(name) {
        return function() {
            return audioOptions[name];
        }
    }

    function displayer(units) {
        return function(value) {
            return (value).toFixed(1) + units;
        }
    }

    function scriptEnding() {
        AudioDevice.setReverb(false);
        print("Reverb is OFF.");
    }
    _this.enterEntity = function(entityID) {
        print('EBL I am insiude');

        AudioDevice.setReverbOptions(audioOptions);
        AudioDevice.setReverb(true);
        print("Reverb is ON.");
    };

    _this.leaveEntity = function(entityID) {
        print('EBL I am outsidee');
        AudioDevice.setReverb(false);
        print("Reverb is OFF.");
        // Messages.sendMessage('PlayBackOnAssignment', 'BowShootingGameWelcome');
    };
});