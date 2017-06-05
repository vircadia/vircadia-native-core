//
//  stopwatchStartStop.js
//
//  Created by David Rowe on 26 May 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {
    var messageChannel;
    this.preload = function (entityID) {
        var properties = Entities.getEntityProperties(entityID, "userData");
        this.messageChannel = "STOPWATCH-" + JSON.parse(properties.userData).stopwatchID;
    };
    function click() {
        Messages.sendMessage(this.messageChannel, "startStop");
    }
    this.startNearTrigger = click;
    this.startFarTrigger = click;
    this.clickDownOnEntity = click;
});
