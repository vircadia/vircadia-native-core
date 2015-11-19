//
//  Created by Bradley Austin Davis on 2015/08/25
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// FIXME Script paths have to be relative to the caller, in this case libraries/OmniTool.js
Script.include("magBalls/constants.js");
Script.include("magBalls/graph.js");
Script.include("magBalls/edgeSpring.js");
Script.include("magBalls/magBalls.js");
Script.include("libraries/avatarRelativeOverlays.js");

OmniToolModuleType = "MagBallsController"

getMagBallsData = function(id) {
    return getEntityCustomData(MAG_BALLS_DATA_NAME, id, {});
}

setMagBallsData = function(id, value) {
    setEntityCustomData(MAG_BALLS_DATA_NAME, id, value);
}

var UI_BALL_RADIUS = 0.01;
var MODE_INFO = { };

MODE_INFO[BALL_EDIT_MODE_ADD] = {
    uiPosition: {
        x: 0.15,
        y: -0.08,
        z: -0.35,
    },
    colors: [ COLORS.GREEN, COLORS.BLUE ],
    // FIXME use an http path or find a way to get the relative path to the file
    url: Script.resolvePath('html/magBalls/addMode.html'),
};

MODE_INFO[BALL_EDIT_MODE_DELETE] = {
    uiPosition: {
        x: 0.20,
        y: -0.08,
        z: -0.32,
    },
    colors: [ COLORS.RED, COLORS.BLUE ],
    // FIXME use an http path or find a way to get the relative path to the file
    url: Script.resolvePath('html/magBalls/deleteMode.html'),
};

var UI_POSITION_MODE_LABEL = Vec3.multiply(0.5,
    Vec3.sum(MODE_INFO[BALL_EDIT_MODE_ADD].uiPosition,
             MODE_INFO[BALL_EDIT_MODE_DELETE].uiPosition));

UI_POSITION_MODE_LABEL.y = -0.02;

var UI_BALL_PROTOTYPE = {
    size: UI_BALL_RADIUS * 2.0,
    alpha: 1.0,
    solid: true,
    visible: true,
}

OmniToolModules.MagBallsController = function(omniTool, entityId) {
    this.omniTool = omniTool;
    this.entityId = entityId;

    // In hold mode, holding a ball requires that you keep the action
    // button pressed, while if this is false, clicking on a ball selects
    // it and clicking again will drop it.  
    this.holdMode = true;

    this.highlighter = new Highlighter();
    this.magBalls = new MagBalls();
    this.highlighter.setSize(BALL_SIZE);
    this.ghostEdges = {};
    this.selectionRadiusMultipler = 1.5;
    this.uiOverlays = new AvatarRelativeOverlays();
    

    // create the overlay relative to the avatar
    this.uiOverlays.addOverlay("sphere", mergeObjects(UI_BALL_PROTOTYPE, {
        color: MODE_INFO[BALL_EDIT_MODE_ADD].colors[0],
        position: MODE_INFO[BALL_EDIT_MODE_ADD].uiPosition,
    }));
    this.uiOverlays.addOverlay("sphere", mergeObjects(UI_BALL_PROTOTYPE, {
        color: MODE_INFO[BALL_EDIT_MODE_DELETE].colors[0],
        position: MODE_INFO[BALL_EDIT_MODE_DELETE].uiPosition,
    }));

    // FIXME find the proper URLs to use
    this.modeLabel = this.uiOverlays.addOverlay("web3d", {
        isFacingAvatar: true,
        alpha: 1.0,
        dimensions: { x: 0.16, y: 0.12, z: 0.001},
        color: "White",
        position: UI_POSITION_MODE_LABEL,
    });
    
    this.setMode(BALL_EDIT_MODE_ADD);

    // DEBUGGING ONLY - Fix old, bad edge bounding boxes
    //for (var edgeId in this.magBalls.edges) {
    //    Entities.editEntity(edgeId, {
    //        dimensions: LINE_DIMENSIONS,
    //    });
    //}
    // DEBUGGING ONLY - Clear any previous balls
    // this.magBalls.clear();
    // DEBUGGING ONLY - Attempt to fix connections between balls
    // and delete bad connections.  Warning... if you haven't looked around
    // and caused the domain server to send you all the nearby balls as well as the connections,
    // this can break your structures
    // this.magBalls.repair();
}

OmniToolModules.MagBallsController.prototype.onUnload = function() {
    this.clearGhostEdges();
    this.uiOverlays.deleteAll();
}


OmniToolModules.MagBallsController.prototype.setMode = function(mode) {
    if (mode === this.mode) {
        return;
    }

    logDebug("Changing mode to '" + mode + "'");
    Overlays.editOverlay(this.modeLabel, {
        url: MODE_INFO[mode].url
    });

    this.mode = mode;
    var color1;
    var color2;
    switch (this.mode) {
        case BALL_EDIT_MODE_ADD:
            color1 = COLORS.BLUE;
            color2 = COLORS.GREEN;
            break;

        case BALL_EDIT_MODE_MOVE:
            color1 = COLORS.GREEN;
            color2 = COLORS.LIGHT_GREEN;
            break;
        
        case BALL_EDIT_MODE_DELETE:
            color1 = COLORS.RED;
            color2 = COLORS.BLUE;
            break;
        
        case BALL_EDIT_MODE_DELETE_SHAPE:
            color1 = COLORS.RED;
            color2 = COLORS.YELLOW;
            break;
    }
    this.omniTool.model.setTipColors(color1, color2);
    
}

OmniToolModules.MagBallsController.prototype.findUiBallHit = function() {
    var result = null;
    for (var mode in MODE_INFO) {
        var modeInfo = MODE_INFO[mode];
        var spherePoint = getEyeRelativePosition(modeInfo.uiPosition);
        if (findSpherePointHit(spherePoint, UI_BALL_RADIUS * 2, this.tipPosition)) {
            this.highlighter.highlight(spherePoint);
            this.highlighter.setColor("White");
            // FIXME why doesn't this work?
            this.highlighter.setSize(UI_BALL_RADIUS * 4);
            return mode;
        }
    }
    return;
}

OmniToolModules.MagBallsController.prototype.onUpdateSelected = function(deltaTime) {
    if (!this.selected) {
        return;
    }
    
    Entities.editEntity(this.selected, { position: this.tipPosition });
    var targetBalls = this.magBalls.findPotentialEdges(this.selected);
    for (var ballId in targetBalls) {
        var targetPosition = this.magBalls.getNodePosition(ballId);
        var distance = Vec3.distance(targetPosition, this.tipPosition);
        var variance = this.magBalls.getVariance(distance);
        var mix = Math.abs(variance) / this.magBalls.MAX_VARIANCE;
        var color = colorMix(COLORS.YELLOW, COLORS.RED, mix);
        if (!this.ghostEdges[ballId]) {
            // create the ovleray
            this.ghostEdges[ballId] = Overlays.addOverlay("line3d", {
                start: this.magBalls.getNodePosition(ballId),
                end: this.tipPosition,
                color: color,
                alpha: 1,
                lineWidth: 5,
                visible: true,
            });
        } else {
            Overlays.editOverlay(this.ghostEdges[ballId], {
                end: this.tipPosition,
                color: color,
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

OmniToolModules.MagBallsController.prototype.onUpdate = function(deltaTime) {
    this.tipPosition = this.omniTool.getPosition();
    this.uiOverlays.onUpdate(deltaTime);

    this.onUpdateSelected();

    if (this.findUiBallHit()) {
        return;
    }

    if (!this.selected) {
        // Find the highlight target and set it.
        var target = this.magBalls.findNearestNode(this.tipPosition, BALL_RADIUS * this.selectionRadiusMultipler);
        this.highlighter.highlight(target);
        this.highlighter.setColor(MODE_INFO[this.mode].colors[0]);
        if (!target) {
            this.magBalls.onUpdate(deltaTime);
        }
        return;
    }
}

OmniToolModules.MagBallsController.prototype.deselect = function() {
    if (!this.selected) {
        return false
    }
    this.clearGhostEdges();
    this.magBalls.releaseBall(this.selected);
    this.selected = null;
    return true;
}


OmniToolModules.MagBallsController.prototype.onClick = function() {
    var newMode = this.findUiBallHit();
    if (newMode) {
        if (this.selected) {
            this.magBalls.destroyNode(highlighted);
            this.selected = null;
        }
        this.setMode(newMode);
        return;
    }

    if (this.deselect()) {
        return;
    }

    logDebug("MagBallsController onClick: " + vec3toStr(this.tipPosition));

    // TODO add checking against UI shapes for adding or deleting balls.
    var highlighted = this.highlighter.highlighted;
    if (this.mode == BALL_EDIT_MODE_ADD && !highlighted) {
        highlighted = this.magBalls.createBall(this.tipPosition);
    }
    
    // Nothing to select or create means we're done here.
    if (!highlighted) {
        return;
    }
    
    switch (this.mode) {
        case BALL_EDIT_MODE_ADD:
        case BALL_EDIT_MODE_MOVE:
            this.magBalls.selectBall(highlighted);
            this.selected = highlighted;
            logDebug("Selected " + this.selected);
            break;

        case BALL_EDIT_MODE_DELETE:
            this.magBalls.destroyNode(highlighted);
            break;
        
        case BALL_EDIT_MODE_DELETE_SHAPE:
            logDebug("Not implemented yet");
            break;
    }
    
    if (this.selected) {
        this.highlighter.highlight(null);
    }
}

OmniToolModules.MagBallsController.prototype.onRelease = function() {
    if (this.holdMode) {
        this.deselect();
    }
}

OmniToolModules.MagBallsController.prototype.clearGhostEdges = function() {
    for(var ballId in this.ghostEdges) {
        Overlays.deleteOverlay(this.ghostEdges[ballId]);
        delete this.ghostEdges[ballId];
    }
}


