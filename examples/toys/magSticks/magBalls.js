
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

// 2 millimeters
var EPSILON = (.002) / BALL_DISTANCE;

var LINE_DIMENSIONS = {
    x: 5,
    y: 5,
    z: 5
}

var LINE_PROTOTYPE = {
    type: "Line",
    name: EDGE_NAME,
    color: COLORS.CYAN,
    dimensions: LINE_DIMENSIONS,
    lineWidth: 5,
    visible: true,
    ignoreCollisions: true,
    collisionsWillMove: false,
    script: "file:/Users/bdavis/Git/hifi/examples/toys/magSticks/springEdgeEntity.js"
}

var EDGE_PROTOTYPE = LINE_PROTOTYPE;

// var EDGE_PROTOTYPE = {
// type: "Sphere",
// name: EDGE_NAME,
// color: { red: 0, green: 255, blue: 255 },
// //dimensions: STICK_DIMENSIONS,
// dimensions: { x: 0.02, y: 0.02, z: 0.02 },
// rotation: rotation,
// visible: true,
// ignoreCollisions: true,
// collisionsWillMove: false
// }


// A collection of balls and edges connecting them.
MagBalls = function() {
    this.selectedNodes = {};

    Graph.call(this);

    var _this = this;
    Script.update.connect(function(deltaTime) {
        _this.onUpdate(deltaTime);
    });

    Script.scriptEnding.connect(function() {
        _this.onCleanup();
    });
}

MagBalls.prototype = Object.create( Graph.prototype );

MagBalls.prototype.onUpdate = function(deltaTime) {
    // FIXME move to a physics based implementation as soon as bullet
    // is exposed to entities
}

MagBalls.prototype.createNodeEntity = function(customProperies) {
    return Entities.addEntity(mergeObjects(BALL_PROTOTYPE, customProperies));
}

MagBalls.prototype.getEdgeProperties = function(nodeIdA, nodeIdB) {
    var apos = this.getNodePosition(nodeIdA);
    var bpos = this.getNodePosition(nodeIdB);
    return {
        position: apos,
        linePoints: [ ZERO_VECTOR, Vec3.subtract(bpos, apos) ],
        userData: JSON.stringify({
            magBalls: {
                start: nodeIdA,
                end: nodeIdB,
                length: BALL_DISTANCE
            }
        })
    };
}

MagBalls.prototype.createEdgeEntity = function(nodeIdA, nodeIdB) {
    var customProperties = this.getEdgeProperties(nodeIdA, nodeIdB)
    return Entities.addEntity(mergeObjects(EDGE_PROTOTYPE, customProperties));
}

MagBalls.prototype.findPotentialEdges = function(nodeId) {
    var variances = {};
    for (var otherNodeId in this.nodes) {
        // can't self connect 
        if (otherNodeId == nodeId) {
            continue;
        }
        
        // can't doubly connect
        if (this.areConnected(otherNodeId, nodeId)) {
            continue;
        }
        
        // Too far to attempt
        var distance = this.getNodeDistance(nodeId, otherNodeId);
        var variance = this.getVariance(distance);
        if (variance > 0.25) {
            continue;
        }
        
        variances[otherNodeId] = variance;
    }
    return variances;
}

MagBalls.prototype.grabBall = function(position, maxDist) {
    var selected = this.findNearestNode(position, maxDist);
    if (!selected) {
        selected = this.createNode({ position: position });
    } 
    if (selected) {
        this.breakEdges(selected);
        this.selectedNodes[selected] = true;
    }
    return selected;
}

MagBalls.prototype.releaseBall = function(releasedBall) {
    delete this.selectedNodes[releasedBall];
    logDebug("Released ball: " + releasedBall);
    
    // FIXME iterate through the other balls and ensure we don't intersect with
    // any of them. If we do, just delete this ball and return. (play a pop
    // sound)
    
    var releasePosition = this.getNodePosition(releasedBall);

    // Don't overlap other balls
    for (var nodeId in this.nodes) {
        if (nodeId == releasedBall) {
            continue;
        }
        var distance = this.getNodeDistance(releasedBall, nodeId);
        if (distance < BALL_SIZE / 2.0) {
            this.destroyNode(nodeId);
            return;
        }
    }
    
    var targets = this.findPotentialEdges(releasedBall);
    for (var otherBallId in targets) {
        this.createEdge(otherBallId, releasedBall);
    }
    this.clean();
    return;

    // sort other balls by distance from the stick length
    var createdEdge = false;
    while (true) {
        // Get the list of candidate connections
        var variances = this.findPotentialEdges(releasedBall);
        // sort them by the difference from an ideal distance
        var targetBalls = Object.keys(variances);
        if (!targetBalls.length) {
            break;
        }

        // special case when there are 2 potential matches,
        // try to create a ring on a plane
        if (targetBalls.length == 2 && this.tryCreateRing(targetBalls, releasedBall)) {
            createdEdge = true;
            break;
        }
        
        // special case when there are 3 potential matches,
        // try create a fan
        if (targetBalls.length == 3 && this.tryCreateFan(targetBalls, releasedBall)) {
            createdEdge = true;
            break;
        }
        
        targetBalls.sort(function(a, b){
            return variances[a] - variances[b];
        });

        // find the nearest matching unconnected ball and create an edge to it
        // if possible
        // note that createEdge will preferentially move the second entity, the
        // released ball
        // in order to create a fit
        var str = "Attempt to create edge between " + targetBalls[0] + " and " + releasedBall;
        if (!this.tryCreateEdge(targetBalls[0], releasedBall)) {
            logDebug(str + "failed");
            var nodeDistance = this.getNodeDistance(targetBalls[0], releasedBall);
            var variance = this.getVariance(nodeDistance);
            logDebug("Distance was " + (nodeDistance * 100).toFixed(2) + "cm with a variance of " + variance);
            break;
        }
        logDebug(str + " succeeded");
       
        releasePosition = this.getNodePosition(releasedBall);

        // Record that we created at least one edge
        createdEdge = true;
    }
    
    if (createdEdge) {
        // FIXME play a snap sound
    }

    this.clean();
    this.validate();
}

MagBalls.prototype.tryCreateEdge = function(from, to) {
    var fromPos = this.getNodePosition(from);
    var toPos = this.getNodePosition(to);
    var vector = Vec3.subtract(toPos, fromPos);
    var originalLength = Vec3.length(vector);
    
    var variance = this.getVariance(originalLength);
    // if they're already at a close enough distance, just create the edge
    if (variance < EPSILON) {
        logDebug("Length " + originalLength + " with variance of " + (variance * 100).toFixed(2) + " is within epislon " + EPSILON) ;
        // close enough for government work
        this.createEdge(from, to);
        return true;
    }

    // FIXME find the constraints on `from` and `to` and determine if there is a
    // new positiong
    // for 'to' that keeps it's current connections and connects with 'from'
    // Do only first order scanning for now, unless we can expose a mechanism
    // for interacting
    // to reach equilibrium via Bullet
    // * a ball zero edges is fully free...
    // * a ball with one edge free to move on a sphere section
    // * a ball with two edges is free to move in a circle
    // * a ball with more than two edges is not free to move

    // Zero edges for the destination
    var existingEdges = Object.keys(this.nodes[to]);
    var edgeCount = existingEdges.length;
    if (!edgeCount) {
        // Easy case 1: unconnected ball
        // Move the ball along it's current path to match the desired distance
        vector = Vec3.multiply(BALL_DISTANCE, Vec3.normalize(vector));
        // Add the vector to the starting position to find the new position
        var newPosition = Vec3.sum(vector, fromPos);
        // update the entity
        Entities.editEntity(to, { position: newPosition });
        moved = true;
    } else if (edgeCount > 2) {
        // Easy case 2: locked position ball
        // FIXME should check the target ball to see if it can be moved.
        // Possible easy solution is to recurse into this.createEdge and swap
        // the parameters,
        // but need to prevert infinite recursion
        // for now...
        return false;
    } else {
        var connectedBalls = this.getConnectedNodes(to);
        // find the other balls connected, will be either 1 or 2
        var origin = { x: 0, y: 0, z: 0 };
        for (var nodeId in connectedBalls) {
            origin = Vec3.sum(origin, this.getNodePosition(nodeId));
        }

        if (edgeCount > 1) {
            origin = Vec3.multiply(origin, 1 / edgeCount);
        }
        // logDebug("Using origin " + vec3toStr(origin));
        
        if (edgeCount == 1) {
            // vectors from the temp origin to the two balls.
            var v1 = Vec3.subtract(toPos, origin);
            var v2 = Vec3.subtract(fromPos, origin);
            
            // ortogonal to the solution plane
            var o1 = Vec3.normalize(Vec3.cross(Vec3.normalize(v2), Vec3.normalize(v1)));
            // addLine(origin, o1, COLORS.RED); // debugging
            
            // orthogonal to o1, lying on the solution plane
            var o2 = Vec3.normalize(Vec3.cross(o1, Vec3.normalize(v2)));
            // addLine(origin, o2, COLORS.YELLOW); // debugging
            
            // The adjacent side of a right triangle containg the
            // solution as one of the points
            var v3 = Vec3.multiply(0.5, v2);

            // The length of the adjacent side of the triangle
            var l1 = Vec3.length(v3);
            // The length of the hypotenuse
            var r = BALL_DISTANCE;
            
            // No connection possible
            if (l1 > r) {
                return false;
            }
            
            // The length of the opposite side
            var l2 = Math.sqrt(r * r - l1 * l1);

            // vector with the length and direction of the opposite side
            var v4 = Vec3.multiply(l2, Vec3.normalize(o2));
            
            // Add the adjacent vector and the opposite vector to get the
            // hypotenuse vector
            var result = Vec3.sum(v3, v4);
            // move back into world space
            result = Vec3.sum(origin, result);
            // update the entity
            Entities.editEntity(to, { position: result });
        } else {
            // Has a bug of some kind... validation fails after this

            // Debugging marker
             //Entities.addEntity(mergeObjects(BALL_PROTOTYPE, {
             //   position: origin,
             //   color: COLORS.YELLOW,
             //   dimensions: Vec3.multiply(0.4, BALL_DIMENSIONS)
             //}));
            
            var v1 = Vec3.subtract(fromPos, origin);
            // addLine(origin, v1, COLORS.RED); // debugging

            
            var v2 = Vec3.subtract(toPos, origin);
            // addLine(origin, v2, COLORS.GREEN); // debugging

            // the lengths of v1 and v2 represent the lengths of two sides
            // of the triangle we need to build.
            var l1 = Vec3.length(v1);
            var l2 = Vec3.length(v2);
            // The remaining side is the edge we are trying to create, so
            // it will be of length BALL_DISTANCE
            var l3 = BALL_DISTANCE;
            // given this triangle, we want to know the angle between l1 and l2
            // (this is NOT the same as the angle between v1 and v2, because we
            // are trying to rotate v2 around o1 to find a solution
            // Use law of cosines to find the angle: cos A = (b^2 + c^2 − a^2) /
            // 2bc
            var cosA = (l1 * l1 + l2 * l2 - l3 * l3) / (2.0 * l1 * l2);
            
            // Having this angle gives us all three angles of the right triangle
            // containing
            // the solution, along with the length of the hypotenuse, which is
            // l2
            var hyp = l2;
            // We need to find the length of the adjacent and opposite sides
            // since cos(A) = adjacent / hypotenuse, then adjacent = hypotenuse
            // * cos(A)
            var adj = hyp * cosA;
            // Pythagoras gives us the opposite side length
            var opp = Math.sqrt(hyp * hyp - adj * adj);
            
            // v1 is the direction vector we need for the adjacent side, so
            // resize it to
            // the proper length
            v1 = Vec3.multiply(adj, Vec3.normalize(v1));
            // addLine(origin, v1, COLORS.GREEN); // debugging

            // FIXME, these are not the right normals, because the ball needs to rotate around the origin
            
            // This is the normal to the plane on which our solution lies
            var o1 = Vec3.cross(v1, v2);

            // Our final side is a normal to the plane defined by o1 and v1
            // and is of length opp
            var o2 = Vec3.multiply(opp, Vec3.normalize(Vec3.cross(o1, v1)));

            // Our final result is the sum of v1 and o2 (opposite side vector +
            // adjacent side vector)
            var result = Vec3.sum(v1, o2);
            // Move back into world space
            result = Vec3.sum(origin, result);
            // update the entity
            Entities.editEntity(to, { position: result });
        }
    }

    // Fixup existing edges if we moved the ball
    for (var edgeId in this.nodes[to]) {
        this.fixupEdge(edgeId);
    }
    
    this.createEdge(from, to);
    return true;
}

MagBalls.prototype.tryCreateRing = function(fromBalls, to) {
    // FIXME, if the user tries to connect two points, attempt to
    // walk the graph and see if they're creating a ring of 4 or
    // more vertices, if so and they're within N percent of lying
    // on a plane, then adjust them all so they lie on a plance
    return false;
}

function pausecomp(millis) {
    var date = new Date();
    var curDate = null;
    do { curDate = new Date(); } while(curDate-date < millis);
}

// find a normal between three points
function findNormal(a, b, c) {
    var aa = Vec3.subtract(a, b);
    var cc = Vec3.subtract(c, b);
    return Vec3.cross(aa, cc);
}

MagBalls.prototype.tryCreateFan = function(fromBalls, to) {
    logDebug("Attempting to create fan");
    // if the user tries to connect three points, attempt to
    // walk the graph and see if they're creating fan, adjust all the
    // points to lie on a plane an equidistant from the shared vertex

    // A fan may exist if given three potential connections, two of the connection
    // share and edge with the third
    var a = fromBalls[0];
    var b = fromBalls[1];
    var c = fromBalls[2];
    var ab = this.areConnected(a, b);
    var bc = this.areConnected(b, c);
    var ca = this.areConnected(c, a);
    if (ab && bc && ca) {
        // tetrahedron, let the generic code handle it
        return false;
    }
    var crux = null;
    var left = null;
    var right = null;
    if (ab && bc) {
        crux = b;
        left = a;
        right = c;
    } else if (bc && ca) {
        crux = a;
        left = b;
        right = a;
    } else if (ca && ab) {
        crux = a;
        left = c;
        right = b;
    }
    if (crux == null) {
        // we don't have two nodes which share edges with the third, so fail
        return false;
    }
    var loop = this.findShortestPath(left, right, { exclude: crux });
    if (!loop) {
        return false;
    }
    
    // find the normal to the target plane
    var origin = this.getNodePosition(crux);
    var normals = [];
    var averageNormal = ZERO_VECTOR;
    for (var i = 0; i < loop.length - 2; ++i) {
        var a = loop[i];
        var b = loop[i + 1];
        var c = loop[i + 2];
        var apos = this.getNodePosition(a);
        var bpos = this.getNodePosition(b);
        var cpos = this.getNodePosition(c);
        var normal = Vec3.normalize(findNormal(apos, bpos, cpos));
        averageNormal = Vec3.sum(averageNormal, normal);
        addLine(bpos, normal, COLORS.YELLOW);
        normals.push(normal);
    }
    averageNormal = Vec3.normalize(Vec3.multiply(1 / normals.length, averageNormal));
    
    addLine(origin, averageNormal, COLORS.RED);

    // FIXME need to account for locked nodes... if there are 3 locked nodes on the loop,
    // then find their cross product
    // if there are more than 3 locked nodes on the loop, check if they have matching cross
    // products, otherwise fail
    
    return false;
}

MagBalls.prototype.fixupEdge = function(edgeId) {
    var ballsInEdge = Object.keys(this.edges[edgeId]);
    var customProperties = this.getEdgeProperties(ballsInEdge[0], ballsInEdge[1]);
    Entities.editEntity(edgeId, customProperties);
}

MagBalls.prototype.getVariance = function(distance) {
    // Given two points, how big is the difference between their distance
    // and the desired length length
    return (Math.abs(distance - BALL_DISTANCE)) / BALL_DISTANCE;
}

// remove unconnected balls
MagBalls.prototype.clean = function() {
    // do nothing unless there are at least 2 balls and one edge
    if (Object.keys(this.nodes).length < 2 || !Object.keys(this.edges).length) {
        return;
    }
    var disconnectedNodes = {};
    for (var nodeId in this.nodes) {
        if (!Object.keys(this.nodes[nodeId]).length) {
            disconnectedNodes[nodeId] = true;
        }
    }
    for (var nodeId in disconnectedNodes) {
        this.destroyNode(nodeId);
    }
}

// remove all balls
MagBalls.prototype.clear = function() {
    if (DEBUG_MAGSTICKS) {
        this.deleteAll();
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

// Override to check lengths as well as connection consistency
MagBalls.prototype.validate = function() {
    var error = Graph.prototype.validate.call(this);

    if (!error) {
        for (edgeId in this.edges) {
            var length = this.getEdgeLength(edgeId);
            var variance = this.getVariance(length);
            if (variance > EPSILON) {
                Entities.editEntity(edgeId, { color: COLORS.RED });
                
                logDebug("Edge " + edgeId + " length " + (length * 100).toFixed(2) + " cm  variance " + (variance * 100).toFixed(3) + "%");
                error = true;
            }
        }
        if (error) {
            logDebug(EPSILON);
        }
    }
}
