//
//  Created by Bradley Austin Davis on 2015/08/25
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("magSticks/constants.js");
Script.include("magSticks/utils.js");
Script.include("magSticks/graph.js");
Script.include("magSticks/magBalls.js");
Script.include("magSticks/highlighter.js");
Script.include("magSticks/handController.js");

var magBalls = new MagBalls();

// Clear any previous balls
// magBalls.clear();

// How close do we need to be to a ball to select it....  radius + 10%
var BALL_SELECTION_RADIUS = BALL_SIZE / 2.0 * 1.5;

BallController = function(side) {
    HandController.call(this, side);
    this.highlighter = new Highlighter();
    this.highlighter.setSize(BALL_SIZE);
    this.ghostEdges = {};
}

BallController.prototype = Object.create( HandController.prototype );

BallController.prototype.onUpdate = function(deltaTime) {
    HandController.prototype.onUpdate.call(this, deltaTime);
    if (!this.selected) {
        // Find the highlight target and set it.
        var target = magBalls.findNearestNode(this.tipPosition, BALL_SELECTION_RADIUS);
        this.highlighter.highlight(target);
        return;
    }
    this.highlighter.highlight(null);
    Entities.editEntity(this.selected, { position: this.tipPosition });
    var targetBalls = magBalls.findPotentialEdges(this.selected);
    for (var ballId in targetBalls) {
        if (!this.ghostEdges[ballId]) {
            // create the ovleray
            this.ghostEdges[ballId] = Overlays.addOverlay("line3d", {
                start: magBalls.getNodePosition(ballId),
                end: this.tipPosition,
                color: { red: 255, green: 0, blue: 0},
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
    this.selected = magBalls.grabBall(this.tipPosition, BALL_SELECTION_RADIUS);
    this.highlighter.highlight(null);
}

BallController.prototype.onRelease = function() {
    this.clearGhostEdges();
    magBalls.releaseBall(this.selected);
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

// FIXME resolve some of the issues with dual controllers before allowing both controllers active
var handControllers = [new BallController(LEFT_CONTROLLER)]; //, new HandController(RIGHT) ];
