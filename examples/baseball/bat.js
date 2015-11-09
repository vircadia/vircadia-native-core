//
//  bat.js
//  examples/baseball/
//
//  Created by Ryan Huffman on Nov 9, 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    Script.include("pitching.js");
    var pitchingMachine = null;
    this.startNearGrab = function() {
        print("Started near grab!");
        if (!pitchingMachine) {
            pitchingMachine = getOrCreatePitchingMachine();
            Script.update.connect(function(dt) { pitchingMachine.update(dt); });
        }
        pitchingMachine.start();
        MyAvatar.shouldRenderLocally = false;
    };
    this.continueNearGrab = function() {
        if (!pitchingMachine) {
            pitchingMachine = getOrCreatePitchingMachine();
            Script.update.connect(function(dt) { pitchingMachine.update(dt); });
        }
        pitchingMachine.start();
        MyAvatar.shouldRenderLocally = false;
    }
    this.releaseGrab = function() {
        print("Stopped near grab!");
        if (pitchingMachine) {
            pitchingMachine.stop();
        }
        MyAvatar.shouldRenderLocally = true;
    };
});
