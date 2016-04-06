//
//  smallRoom.js
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
    Script.include("http://public.highfidelity.io/scripts/utilities/tools/cookies.js");
    var audioOptions = new AudioEffectOptions({
        bandwidth: 7000,
        preDelay: 20,
        lateDelay: 0,
        reverbTime: 1.5,
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
        earlyGain: -24,
        lateGain: -24,
        earlyMixLeft: 20,
        earlyMixRight: 20,
        lateMixLeft: 90,
        lateMixRight: 90,
        wetDryMix: 70
    });

    var panel = new Panel(10, 160);

    function setter(name) {
        return function(value) {
            audioOptions[name] = value;
            AudioDevice.setReverbOptions(audioOptions);
        }
    }


});