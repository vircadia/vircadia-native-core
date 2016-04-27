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

    this.pitchAndHideAvatar = function() {
        if (!pitchingMachine) {
            pitchingMachine = getOrCreatePitchingMachine();
            Script.update.connect(function(dt) { pitchingMachine.update(dt); });
        }
        pitchingMachine.start();
        MyAvatar.shouldRenderLocally = false;
    };

    this.startNearGrab = function() {
        // send the avatar to the baseball location so that they're ready to bat
        location = "/baseball"

        this.pitchAndHideAvatar()
    };

    this.continueNearGrab = function() {
        this.pitchAndHideAvatar()
    };

    this.releaseGrab = function() {
        if (pitchingMachine) {
            pitchingMachine.stop();
        }
        MyAvatar.shouldRenderLocally = true;
    };
});
