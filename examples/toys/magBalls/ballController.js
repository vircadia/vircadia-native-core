Script.include("handController.js");
Script.include("highlighter.js");

BallController = function(side, magBalls) {
    HandController.call(this, side);
    this.magBalls = magBalls;
    this.highlighter = new Highlighter();
    this.highlighter.setSize(BALL_SIZE);
    this.ghostEdges = {};
}

BallController.prototype = Object.create( HandController.prototype );

BallController.prototype.onUpdate = function(deltaTime) {
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

BallController.prototype.onClick = function() {
    this.selected = this.magBalls.grabBall(this.tipPosition, BALL_SELECTION_RADIUS);
    this.highlighter.highlight(null);
}

BallController.prototype.onRelease = function() {
    this.clearGhostEdges();
    this.magBalls.releaseBall(this.selected);
    this.selected = null;
}

BallController.prototype.clearGhostEdges = function() {
    for(var ballId in this.ghostEdges) {
        Overlays.deleteOverlay(this.ghostEdges[ballId]);
        delete this.ghostEdges[ballId];
    }
}

BallController.prototype.onCleanup = function() {
    HandController.prototype.onCleanup.call(this);
    this.clearGhostEdges();
}

BallController.prototype.onAltClick = function() {
    return;
    var target = this.magBalls.findNearestNode(this.tipPosition, BALL_SELECTION_RADIUS);
    if (!target) {
        logDebug(target);
        return;
    }
    
    // FIXME move to delete shape
    var toDelete = {};
    var deleteQueue = [ target ];
    while (deleteQueue.length) {
        var curNode = deleteQueue.shift();
        if (toDelete[curNode]) {
            continue;
        }
        toDelete[curNode] = true;
        for (var nodeId in this.magBalls.getConnectedNodes(curNode)) {
            deleteQueue.push(nodeId);
        }
    }
    for (var nodeId in toDelete) {
        this.magBalls.destroyNode(nodeId);
    }
}



BallController.prototype.onAltRelease = function() {
}
