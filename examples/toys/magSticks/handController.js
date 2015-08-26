
LEFT_CONTROLLER = 0;
RIGHT_CONTROLLER = 1;


HandController = function(side) {
    this.side = side;
    this.palm = 2 * side;
    this.tip = 2 * side + 1;
    this.action = findAction(side ? "ACTION2" : "ACTION1");
    this.active = false;
    this.pointer = Overlays.addOverlay("sphere", {
        position: {
            x: 0,
            y: 0,
            z: 0
        },
        size: 0.01,
        color: {
            red: 255,
            green: 255,
            blue: 0
        },
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
    debugPrint("Setting active: " + active);
    this.active = active;
    Overlays.editOverlay(this.pointer, {
        position: this.tipPosition,
        visible: this.active
    });
}

HandController.prototype.updateControllerState = function() {
    var palmPos = Controller.getSpatialControlPosition(this.palm);
    // When on the base hydras report a position of 0
    this.setActive(Vec3.length(palmPos) > 0.001);
    if (!this.active) {
        return;
    }
    var tipPos = Controller.getSpatialControlPosition(this.tip);
    this.tipPosition = scaleLine(palmPos, tipPos, 1.4);
    Overlays.editOverlay(this.pointer, {
        position: this.tipPosition
    });
}

HandController.prototype.onCleanup = function() {
    Overlays.deleteOverlay(this.pointer);
}

HandController.prototype.onUpdate = function(deltaTime) {
    this.updateControllerState();
}

HandController.prototype.onClick = function() {
    debugPrint("Base hand controller does nothing on click");
}

HandController.prototype.onRelease = function() {
    debugPrint("Base hand controller does nothing on release");
}
