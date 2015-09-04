//
//  Created by Bradley Austin Davis on 2015/08/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
var SELECTION_OVERLAY = {
    position: {
        x: 0,
        y: 0,
        z: 0
    },
    color: {
        red: 255,
        green: 255,
        blue: 0
    },
    alpha: 1,
    size: 1.0,
    solid: false,
    //colorPulse: 1.0,
    //pulseMin: 0.5,
    //pulseMax: 1.0,
    visible: false,
    lineWidth: 1.0,
    borderSize: 1.4,
};

Highlighter = function() {
    this.highlightCube = Overlays.addOverlay("cube", this.SELECTION_OVERLAY);
    this.highlighted = null;
    var _this = this;
    Script.scriptEnding.connect(function() {
        _this.onCleanup();
    });
};

Highlighter.prototype.onCleanup = function() {
    Overlays.deleteOverlay(this.highlightCube);
}

Highlighter.prototype.highlight = function(entityIdOrPosition) {
    if (entityIdOrPosition != this.highlighted) {
        this.highlighted = entityIdOrPosition;
        this.updateHighlight();
    }
}

Highlighter.prototype.setSize = function(newSize) {
    Overlays.editOverlay(this.highlightCube, {
        size: newSize
    });
}

Highlighter.prototype.setColor = function(color) {
    Overlays.editOverlay(this.highlightCube, {
        color: color
    });
}


Highlighter.prototype.setRotation = function(newRotation) {
    Overlays.editOverlay(this.highlightCube, {
        rotation: newRotation
    });
}

Highlighter.prototype.updateHighlight = function() {
    if (this.highlighted) {
        var position = this.highlighted;
        if (typeof this.highlighted === "string") {
            var properties = Entities.getEntityProperties(this.highlighted);
            position = properties.position;
        } 
        // logDebug("Making highlight " + this.highlightCube + " visible @ " + vec3toStr(properties.position));
        Overlays.editOverlay(this.highlightCube, {
            position: position,
            visible: true
        });
    } else {
        // logDebug("Making highlight invisible");
        Overlays.editOverlay(this.highlightCube, {
            visible: false
        });
    }
}