MouseTracker = function() {
    this.WIDTH = 60;
    this.HEIGHT = 60;

    this.overlay = Overlays.addOverlay("image", {
        imageURL: Script.resolvePath("dot.png"),
        width: this.HEIGHT,
        height: this.WIDTH,
        x: 100,
        y: 100,
        visible: true
    });

    var that = this;
    Script.scriptEnding.connect(function() {
        that.onCleanup();
    });

    Controller.mousePressEvent.connect(function(event) {
        that.onMousePress(event);
    });

    Controller.mouseMoveEvent.connect(function(event) {
        that.onMouseMove(event);
    });
}

MouseTracker.prototype.onCleanup = function() {
    Overlays.deleteOverlay(this.overlay);
}

MouseTracker.prototype.onMousePress = function(event) {
}

MouseTracker.prototype.onMouseMove = function(event) {
    var width = Overlays.width();
    var height = Overlays.height();
    var x = Math.max(event.x, 0);
    x = Math.min(x, width);
    var y = Math.max(event.y, 0);
    y = Math.min(y, height);
    Overlays.editOverlay(this.overlay, {x: x - this.WIDTH / 2.0, y: y - this.HEIGHT / 2.0});
} 


new MouseTracker();