var BALL_NAME = "MagBall"

var EDGE_NAME = "MagStick"

var BALL_DIMENSIONS = {
    x: BALL_SIZE,
    y: BALL_SIZE,
    z: BALL_SIZE
};

var BALL_COLOR = {
    red: 128,
    green: 128,
    blue: 128
};

var STICK_DIMENSIONS = {
    x: STICK_LENGTH / 6,
    y: STICK_LENGTH / 6,
    z: STICK_LENGTH
};

var BALL_DISTANCE = STICK_LENGTH + BALL_SIZE;

var BALL_PROTOTYPE = {
    type: "Sphere",
    name: BALL_NAME,
    dimensions: BALL_DIMENSIONS,
    color: BALL_COLOR,
    ignoreCollisions: true,
    collisionsWillMove: false
};

// A collection of balls and edges connecting them.
MagBalls = function() {
    /*
        this.balls: {
            ballId1: {
                edgeId1: true
            }
            ballId2: {
                edgeId1: true
            },
            ballId3: {
                edgeId2: true
                edgeId3: true
                edgeId4: true
                edgeId5: true
            },
            ...
        }
    */
    this.balls = {};
    /*
        this.edges: {
            edgeId1: {
                ballId1: true
                ballId2: true
            },
            edgeId2: {
                ballId3: true
                ballId4: true
            },
            ...
        }
    */
    
    // FIXME initialize from nearby entities
    this.edges = {};
    this.selectedBalls = {};
    
    var _this = this;
    Script.update.connect(function(deltaTime) {
        _this.onUpdate(deltaTime);
    });

    Script.scriptEnding.connect(function() {
        _this.onCleanup();
    });
}

MagBalls.prototype.findNearestBall = function(position, maxDist) {
    var resultId = null;
    var resultDist = 0;
    for (var id in this.balls) {
        var properties = Entities.getEntityProperties(id);
        var curDist = Vec3.distance(properties.position, position);
        if (!maxDist || curDist <= maxDist) {
            if (!resultId || curDist < resultDist) {
                resultId = id;
                resultDist = curDist;
            }
        }
    }
    return resultId;
}

// FIXME move to a physics based implementation as soon as bullet
// is exposed to entities 
MagBalls.prototype.onUpdate = function(deltaTime) {
}

function mergeObjects(proto, custom) {
    var result = {};
    for (var attrname in proto) {
        result[attrname] = proto[attrname];
    }
    for (var attrname in custom) {
        result[attrname] = custom[attrname];
    }
    return result;
}

MagBalls.prototype.createBall = function(customProperies) {
    var ballId = Entities.addEntity(mergeObjects(BALL_PROTOTYPE, customProperies));
    this.balls[ballId] = {};
    this.validate();
    return ballId;
}

MagBalls.prototype.grabBall = function(position, maxDist) {
    var selected = this.findNearestBall(position, maxDist);
    if (!selected) {
        selected = this.createBall({ position: position });
    } 
    if (selected) {
        this.breakEdges(selected);
        this.selectedBalls[selected] = true;
    }
    return selected;
}

MagBalls.prototype.findMatches = function(ballId) {
    var variances = {};
    for (var otherBallId in this.balls) {
        if (otherBallId == ballId || this.areConnected(otherBallId, ballId)) {
            // can't self connect or doubly connect
            continue;
        }
        var variance = this.getStickLengthVariance(ballId, otherBallId);
        if (variance > BALL_DISTANCE / 4) {
            continue;
        }
        variances[otherBallId] = variance;
    }
    return variances;
}

MagBalls.prototype.releaseBall = function(releasedBall) {
    delete this.selectedBalls[releasedBall];
    debugPrint("Released ball: " + releasedBall);

    // sort other balls by distance from the stick length
    var edgeTargetBall = null;
    do {
        var releasePosition = this.getBallPosition(releasedBall);
        // Get the list of candidate connections
        var variances = this.findMatches(releasedBall);
        // sort them by the difference from an ideal distance
        var sortedBalls = Object.keys(variances);
        if (!sortedBalls.length) {
            return;
        }
        sortedBalls.sort(function(a, b){
            return variances[a] - variances[b];
        });
        // find the nearest matching unconnected ball
        edgeTargetBall = sortedBalls[0];
        // note that createEdge will preferentially move the second parameter, the target ball
    } while(this.createEdge(edgeTargetBall, releasedBall))

    this.clean();
}

// FIXME the Quat should be able to do this
function findOrientation(from, to) {
    //float m = sqrt(2.f + 2.f * dot(u, v));
    //vec3 w = (1.f / m) * cross(u, v);
    //return quat(0.5f * m, w.x, w.y, w.z);
    var v2 = Vec3.normalize(Vec3.subtract(to, from));
    var v1 = { x: 0.0, y: 1.0, z: 0.0 };
    var m = Math.sqrt(2 + 2 * Vec3.dot(v1, v2));
    var w = Vec3.multiply(1.0 / m, Vec3.cross(v1, v2));
    return {
        w: 0.5 * m,
        x: w.x,
        y: w.y,
        z: w.z
    };
}

var LINE_DIMENSIONS = 5;

var EDGE_PROTOTYPE = {
    type: "Line",
    name: EDGE_NAME,
    color: { red: 0, green: 255, blue: 255 },
    dimensions: {
      x: LINE_DIMENSIONS,
      y: LINE_DIMENSIONS,
      z: LINE_DIMENSIONS
    },
    lineWidth: 5,
    visible: true,
    ignoreCollisions: true,
    collisionsWillMove: false
}

var ZERO_VECTOR = {
    x: 0,
    y: 0,
    z: 0
};

//var EDGE_PROTOTYPE = {
//    type: "Sphere",
//    name: EDGE_NAME,
//    color: { red: 0, green: 255, blue: 255 },
//    //dimensions: STICK_DIMENSIONS,
//    dimensions: { x: 0.02, y: 0.02, z: 0.02 },
//    rotation: rotation,
//    visible: true,
//    ignoreCollisions: true,
//    collisionsWillMove: false
//}

MagBalls.prototype.createEdge = function(from, to) {
    // FIXME find the constraints on from an to and determine if there is an intersection.
    // Do only first order scanning for now, unless we can expose a mechanism for interacting
    // to reach equilibrium via Bullet
    //  * a ball zero edges is fully free...
    //  * a ball with one edge free to move on a sphere section
    //  * a ball with two edges is free to move in a circle
    //  * a ball with more than two edges is not free
    
    var fromPos = this.getBallPosition(from);
    var toPos = this.getBallPosition(to);
    var vector = Vec3.subtract(toPos, fromPos);
    var originalLength = Vec3.length(originalLength);

    // if they're already at a close enough distance, just create the edge
    if ((originalLength - BALL_DISTANCE) > (BALL_DISTANCE * 0.01)) {
        //code
    } else {
        // Attempt to move the ball to match the distance
        vector = Vec3.multiply(BALL_DISTANCE, Vec3.normalize(vector));
        //  Zero edges for the destination
        var edgeCount = Object.keys(this.balls[to]).length;
        if (!edgeCount) {
            // update the entity
            var newPosition = Vec3.sum(vector, fromPos);
            Entities.editEntity(to, { position: newPosition });
        } else if (1 == edgeCount) {
            // FIXME
            // find the other end of the edge already connected, call it ball2
            // given two spheres of radius BALL_DISTANCE centered at fromPos and ball2.position,
            // find the closest point of intersection to toPos
            // move the ball to toPos
        } else if (2 == edgeCount) {
            // FIXME
        } else {
            // FIXME check for the ability to move fromPos
            return false;
        }
    }

    
    // Fixup existing edges
    for (var edgeId in this.balls[to]) {
        this.fixupEdge(edgeId);
    }

    // FIXME, find the correct orientation for a box or model between the two balls
    // for now use a line
    var newEdge = Entities.addEntity(mergeObjects(EDGE_PROTOTYPE, this.findEdgeParams(from, to)));

    this.edges[newEdge] = {};
    this.edges[newEdge][from] = true;
    this.edges[newEdge][to] = true;
    this.balls[from][newEdge] = true;
    this.balls[to][newEdge] = true;
    this.validate();
    return true;
}

MagBalls.prototype.findEdgeParams = function(startBall, endBall) {
    var startBallPos = this.getBallPosition(startBall);
    var endBallPos = this.getBallPosition(endBall);
    var vector = Vec3.subtract(endBallPos, startBallPos);
    return {
        position: startBallPos,
        linePoints: [ ZERO_VECTOR, vector ]
    };
}

MagBalls.prototype.fixupEdge = function(edgeId) {
    var ballsInEdge = Object.keys(this.edges[edgeId]);
    Entities.editEntity(edgeId, this.findEdgeParams(ballsInEdge[0], ballsInEdge[1]));
}

// Given two balls, how big is the difference between their distances and the stick length
MagBalls.prototype.getStickLengthVariance = function(a, b) {
    var apos = this.getBallPosition(a);
    var bpos = this.getBallPosition(b);
    var distance = Vec3.distance(apos, bpos);
    var variance = Math.abs(distance - BALL_DISTANCE);
    return variance;
}

// FIXME remove unconnected balls 
MagBalls.prototype.clean = function() {
    //var deletedBalls = {};
    //if (Object.keys(this.balls).length > 1) {
    //    for (var ball in this.balls) {
    //        if (!this.getConnections(ball)) {
    //            deletedBalls[ball] = true;
    //            Entities.deleteEntity(ball);
    //        }
    //    }
    //}
    //for (var ball in deletedBalls) {
    //    delete this.balls[ball];
    //}
}

MagBalls.prototype.getBallPosition = function(ball) {
    var properties = Entities.getEntityProperties(ball);
    return properties.position;
}

MagBalls.prototype.breakEdges = function(ballId) {
    for (var edgeId in this.balls[ballId]) {
        this.destroyEdge(edgeId);
    }
    // This shouldn't be necessary
    this.balls[ballId] = {};
}

MagBalls.prototype.destroyEdge = function(edgeId) {
    logDebug("Deleting edge " + edgeId);
    // Delete the edge from other balls
    for (var edgeBallId in this.edges[edgeId]) {
        delete this.balls[edgeBallId][edgeId];
    }
    delete this.edges[edgeId];
    Entities.deleteEntity(edgeId);
    this.validate();
}

MagBalls.prototype.destroyBall = function(ballId) {
    logDebug("Deleting ball " + ballId);
    breakEdges(ballId);
    Entities.deleteEntity(ballId);
}

MagBalls.prototype.clear = function() {
    if (DEBUG_MAGSTICKS) {
        var ids = Entities.findEntities(MyAvatar.position, 50);
        var result = [];
        ids.forEach(function(id) {
            var properties = Entities.getEntityProperties(id);
            if (properties.name == BALL_NAME || properties.name == EDGE_NAME) {
                Entities.deleteEntity(id);
            }
        }, this);
    }
}

MagBalls.prototype.areConnected = function(a, b) {
    for (var edge in this.balls[a]) {
        // edge already exists
        if (this.balls[b][edge]) {
            return true;
        }
    }
    return false;
}

MagBalls.prototype.validate = function() {
    var error = false;
    for (ballId in this.balls) {
        for (edgeId in this.balls[ballId]) {
            var edge = this.edges[edgeId];
            if (!edge) {
                logError("Error: ball " + ballId + " refers to unknown edge " + edgeId);
                error = true;
                continue;
            }
            if (!edge[ballId]) {
                logError("Error: ball " + ballId + " refers to edge " + edgeId + " but not vice versa");
                error = true;
                continue;
            }
        }
    }
    
    for (edgeId in this.edges) {
        for (ballId in this.edges[edgeId]) {
            var ball = this.balls[ballId];
            if (!ball) {
                logError("Error: edge " + edgeId + " refers to unknown ball " + ballId);
                error = true;
                continue;
            }
            if (!ball[edgeId]) {
                logError("Error: edge " + edgeId + " refers to ball " + ballId + " but not vice versa");
                error = true;
                continue;
            }
        }
    }
    if (error) {
        logDebug(JSON.stringify({ edges: this.edges, balls: this.balls }, null, 2));
    }
}

// FIXME fetch from a subkey of user data to support non-destructive modifications
MagBalls.prototype.setUserData = function(id, data) {
    Entities.editEntity(id, { userData: JSON.stringify(data) });    
}

// FIXME do non-destructive modification of the existing user data
MagBalls.prototype.getUserData = function(id) {
    var results = null;
    var properties = Entities.getEntityProperties(id);
    if (properties.userData) {
        results = JSON.parse(this.properties.userData);    
    }
    return results;
}

//MagBalls.prototype.findBalls = function() {
//    var ids = Entities.findEntities(MyAvatar.position, 50);
//    var result = [];
//    ids.forEach(function(id) {
//        var properties = Entities.getEntityProperties(id);
//        if (properties.name == BALL_NAME) {
//            result.push(id);
//        }
//    }, this);
//    return result;
//};
//
//MagBalls.prototype.findEdges = function() {
//    var ids = Entities.findEntities(MyAvatar.position, 50);
//    var result = [];
//    ids.forEach(function(id) {
//        var properties = Entities.getEntityProperties(id);
//        if (properties.name == EDGE_NAME) {
//            result.push(id);
//        }
//    }, this);
//    return result;
//};
