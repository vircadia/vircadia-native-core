Script.include("../../libraries/utils.js");


PickerTest = function() {
    // Switch every 5 seconds between normal IPD and 0 IPD (in seconds)
    this.UPDATE_INTERVAL = 10.0;
    this.lastUpdateInterval = 0;

    this.ballId = Overlays.addOverlay("sphere", {
        position: { x: 0, y: 0, z: 0 },
        color: { red: 0, green: 255, blue: 0 },
        size: 0.1,
        solid: true,
        alpha: 1.0,
        visible: true,
    });

    this.ballId2 = Overlays.addOverlay("sphere", {
        position: { x: 0, y: 0, z: 0 },
        color: { red: 255, green: 0, blue: 0 },
        size: 0.05,
        solid: true,
        alpha: 1.0,
        visible: true,
    });

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
    
    Controller.mousePressEvent.connect(function(event) {
        that.onMousePress(event);
    });
    
    Controller.mouseMoveEvent.connect(function(event) {
        that.onMouseMove(event);
    });
    
    Controller.mouseReleaseEvent.connect(function(event) {
        that.onMouseRelease(event);
    });
};

PickerTest.prototype.onCleanup = function() {
    Overlays.deleteOverlay(this.ballId)
    Overlays.deleteOverlay(this.ballId2)
}

PickerTest.prototype.updateOverlays = function() {
    var pickRay = Camera.computePickRay(this.x, this.y);
    var origin = pickRay.origin;
    var direction = pickRay.direction;
    var position = Vec3.sum(origin, direction)
    Overlays.editOverlay(this.ballId, {
        position: position
    });

    Overlays.editOverlay(this.ballId2, {
        position: origin
    });
}

PickerTest.prototype.onUpdate = function(deltaTime) {
    if (this.clicked) {
        this.updateOverlays();
    }
}

PickerTest.prototype.onMousePress = function(event) {
    if (event.button !== "LEFT") {
        return
    }
    this.clicked = true;
    this.x = event.x;
    this.y = event.y;
    this.updateOverlays();
}

PickerTest.prototype.onMouseRelease = function(event) {
    if (event.button !== "LEFT") {
        return
    }
    this.clicked = false;
}

PickerTest.prototype.onMouseMove = function(event) {
    if (this.clicked) {
        this.x = event.x;
        this.y = event.y;
        this.updateOverlays();
    }
}

var PickerTest = new PickerTest();
