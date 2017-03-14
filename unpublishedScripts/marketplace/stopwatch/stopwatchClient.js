//
//  stopwatchServer.js
//
//  Created by Ryan Huffman on 1/20/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var messageChannel;
    this.preload = function(entityID) {
        this.messageChannel = "STOPWATCH-" + entityID;
    };
    function click() {
        Messages.sendMessage(this.messageChannel, 'click');
    }
    this.startNearTrigger = click;
    this.startFarTrigger = click;
    this.clickDownOnEntity = click;
});
