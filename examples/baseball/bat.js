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

Script.include("pitching.js");

(function() {
    var pitchingMachine = null;

    this.startNearGrab = this.continueNearGrab = function() {
        if (!pitchingMachine) {
            pitchingMachine = getOrCreatePitchingMachine();
            Script.update.connect(function(dt) { pitchingMachine.update(dt); });
        }
        pitchingMachine.start();
        MyAvatar.shouldRenderLocally = false;
    };

    this.releaseGrab = function() {
        if (pitchingMachine) {
            pitchingMachine.stop();
        }
        MyAvatar.shouldRenderLocally = true;
    };
});
