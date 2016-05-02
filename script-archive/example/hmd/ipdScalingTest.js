//
//  Created by Bradley Austin Davis on 2015/10/04
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

IPDScalingTest = function() {
    // Switch every 5 seconds between normal IPD and 0 IPD (in seconds)
    this.UPDATE_INTERVAL = 10.0;
    this.lastUpdateInterval = 0;
    this.scaled = false;

    var that = this;
    Script.scriptEnding.connect(function() {
        that.onCleanup();
    });

    Script.update.connect(function(deltaTime) {
        that.lastUpdateInterval += deltaTime;
        if (that.lastUpdateInterval >= that.UPDATE_INTERVAL) {
            that.onUpdate(that.lastUpdateInterval);
            that.lastUpdateInterval = 0;
        }
    });
}

IPDScalingTest.prototype.onCleanup = function() {
    HMD.setIPDScale(1.0);
}

IPDScalingTest.prototype.onUpdate = function(deltaTime) {
    this.scaled = !this.scaled;
    if (this.scaled) {
        HMD.ipdScale = 0.0;
    } else {
        HMD.ipdScale = 1.0;
    }
}

new IPDScalingTest();