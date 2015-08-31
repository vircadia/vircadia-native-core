//
//  Created by Bradley Austin Davis on 2015/08/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
LEFT_CONTROLLER = 0;
RIGHT_CONTROLLER = 1;

// FIXME add a customizable wand model and a mechanism to switch between wands
HandController = function(side) {
    this.side = side;
    this.palm = 2 * side;
    this.tip = 2 * side + 1;
    this.action = findAction(side ? "ACTION2" : "ACTION1");
    this.active = false;
    this.tipScale = 1.4;
    this.pointer = Overlays.addOverlay("sphere", {
        position: ZERO_VECTOR,
        size: 0.01,
        color: COLORS.YELLOW,
        alpha: 1.0,
        solid: true,
        visible: false,
    });

    // Connect to desired events
    var _this = this;
    Controller.actionEvent.connect(function(action, state) {
        _this.onActionEvent(action, state);
    });
    
    Script.update.connect(function(deltaTime) {
        _this.onUpdate(deltaTime);
    });
    
    Script.scriptEnding.connect(function() {
        _this.onCleanup();
    });
}

HandController.prototype.onActionEvent = function(action, state) {
    if (action == this.action) {
        if (state) {
            this.onClick();
        } else {
            this.onRelease();
        }
    }
}

HandController.prototype.setActive = function(active) {
    if (active == this.active) {
        return;
    }
    logDebug("Hand controller changing active state: " + active);
    this.active = active;
    Overlays.editOverlay(this.pointer, {
        visible: this.active
    });
    Entities.editEntity(this.wand, {
        visible: this.active
    });
}

HandController.prototype.updateControllerState = function() {
    this.palmPos = Controller.getSpatialControlPosition(this.palm);
    var tipPos = Controller.getSpatialControlPosition(this.tip);
    this.tipPosition = scaleLine(this.palmPos, tipPos, this.tipScale);
    // When on the base hydras report a position of 0
    this.setActive(Vec3.length(this.palmPos) > 0.001);
}

HandController.prototype.onCleanup = function() {
    Overlays.deleteOverlay(this.pointer);
}

HandController.prototype.onUpdate = function(deltaTime) {
    this.updateControllerState();
    if (this.active) {
        Overlays.editOverlay(this.pointer, {
            position: this.tipPosition
        });
        Entities.editEntity(this.wand, {
            position: this.tipPosition
        });
    }
}

HandController.prototype.onClick = function() {
    logDebug("Base hand controller does nothing on click");
}

HandController.prototype.onRelease = function() {
    logDebug("Base hand controller does nothing on release");
}
