Script.include("handController.js");

MenuController = function(side, magBalls) {
    HandController.call(this, side);
}

MenuController.prototype = Object.create( HandController.prototype );

MenuController.prototype.onUpdate = function(deltaTime) {
    HandController.prototype.onUpdate.call(this, deltaTime);
    if (!this.selected) {
        // Find the highlight target and set it.
        var target = this.magBalls.findNearestNode(this.tipPosition, BALL_SELECTION_RADIUS);
        this.highlighter.highlight(target);
        return;
    } 
    this.highlighter.highlight(null);
    Entities.editEntity(this.selected, { position: this.tipPosition });
    var targetBalls = this.magBalls.findPotentialEdges(this.selected);
    for (var ballId in targetBalls) {
        if (!this.ghostEdges[ballId]) {
            // create the ovleray
            this.ghostEdges[ballId] = Overlays.addOverlay("line3d", {
                start: this.magBalls.getNodePosition(ballId),
                end: this.tipPosition,
                color: COLORS.RED,
                alpha: 1,
                lineWidth: 5,
                visible: true,
            });
        } else {
            Overlays.editOverlay(this.ghostEdges[ballId], {
                end: this.tipPosition,
            });
        }
    }
    for (var ballId in this.ghostEdges) {
        if (!targetBalls[ballId]) {
            Overlays.deleteOverlay(this.ghostEdges[ballId]);
            delete this.ghostEdges[ballId];
        }
    }
}

MenuController.prototype.onClick = function() {
    this.selected = this.magBalls.grabBall(this.tipPosition, BALL_SELECTION_RADIUS);
    this.highlighter.highlight(null);
}

MenuController.prototype.onRelease = function() {
    this.clearGhostEdges();
    this.magBalls.releaseBall(this.selected);
    this.selected = null;
}

MenuController.prototype.clearGhostEdges = function() {
    for(var ballId in this.ghostEdges) {
        Overlays.deleteOverlay(this.ghostEdges[ballId]);
        delete this.ghostEdges[ballId];
    }
}

MenuController.prototype.onCleanup = function() {
    HandController.prototype.onCleanup.call(this);
    this.clearGhostEdges();
}
