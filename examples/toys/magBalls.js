//
//  Created by Bradley Austin Davis on 2015/08/25
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// FIXME Script paths have to be relative to the caller, in this case libraries/OmniTool.js
Script.include("../toys/magBalls/constants.js");
Script.include("../toys/magBalls/graph.js");
Script.include("../toys/magBalls/edgeSpring.js");
Script.include("../toys/magBalls/magBalls.js");

OmniToolModuleType = "MagBallsController"


OmniToolModules.MagBallsController = function(omniTool, entityId) {
    this.omniTool = omniTool;
    this.entityId = entityId;
    this.highlighter = new Highlighter();
    this.magBalls = new MagBalls();
    this.highlighter.setSize(BALL_SIZE);
    this.ghostEdges = {};
}

var MAG_BALLS_DATA_NAME = "magBalls";

getMagBallsData = function(id) {
    return getEntityCustomData(MAG_BALLS_DATA_NAME, id, {});
}

setMagBallsData = function(id, value) {
    setEntityCustomData(MAG_BALLS_DATA_NAME, id, value);
}

//var magBalls = new MagBalls();
// DEBUGGING ONLY - Clear any previous balls
// magBalls.clear();

OmniToolModules.MagBallsController.prototype.onClick = function() {
    logDebug("MagBallsController onClick: " + vec3toStr(this.tipPosition));
    
    this.selected = this.highlighter.hightlighted;
    logDebug("This selected: " + this.selected);
    if (!this.selected) {
        this.selected = this.magBalls.createBall(this.tipPosition);
    } 
    this.magBalls.selectBall(this.selected);
    this.highlighter.highlight(null);
    logDebug("Selected " + this.selected);
}

OmniToolModules.MagBallsController.prototype.onRelease = function() {
    logDebug("MagBallsController onRelease: " + vec3toStr(this.tipPosition));
    this.clearGhostEdges();
    if (this.selected) {
        this.magBalls.releaseBall(this.selected);
        this.selected = null;
    }
}

OmniToolModules.MagBallsController.prototype.onUpdate = function(deltaTime) {
    this.tipPosition = this.omniTool.getPosition();
    if (!this.selected) {
        // Find the highlight target and set it.
        var target = this.magBalls.findNearestNode(this.tipPosition, BALL_SELECTION_RADIUS);
        this.highlighter.highlight(target);
        if (!target) {
            this.magBalls.onUpdate(deltaTime);
        }
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

OmniToolModules.MagBallsController.prototype.clearGhostEdges = function() {
    for(var ballId in this.ghostEdges) {
        Overlays.deleteOverlay(this.ghostEdges[ballId]);
        delete this.ghostEdges[ballId];
    }
}


BallController.prototype.onUnload = function() {
    this.clearGhostEdges();
}
