//
//  Created by Bradley Austin Davis on 2015/08/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var UPDATE_INTERVAL = 0.1;


// A collection of balls and edges connecting them.
MagBalls = function() {
    Graph.call(this);
    this.MAX_ADJUST_ITERATIONS = 100;
    this.REFRESH_WAIT_TICKS = 10;
    this.MAX_VARIANCE = 0.25;
    this.lastUpdateAge = 0;
    this.stable = true;
    this.adjustIterations = 0;
    this.selectedNodes = {};
    this.edgeObjects = {};
    this.unstableEdges = {};

    this.refresh();

    var _this = this;
    Script.scriptEnding.connect(function() {
        _this.onCleanup();
    });
    
    Entities.addingEntity.connect(function(entityId) {
        _this.onEntityAdded(entityId);
    });
}

MagBalls.prototype = Object.create( Graph.prototype );

MagBalls.prototype.onUpdate = function(deltaTime) {
    this.lastUpdateAge += deltaTime;
    if (this.lastUpdateAge > UPDATE_INTERVAL) {
        this.lastUpdateAge = 0;
        if (this.refreshNeeded) {
            if (++this.refreshNeeded > this.REFRESH_WAIT_TICKS)  {
                logDebug("Refreshing");
                this.refresh();
                this.refreshNeeded = 0;
            }
        }
        if (!this.stable && !Object.keys(this.selectedNodes).length) {
            this.adjustIterations  += 1;
            var adjusted = false;
            var nodeAdjustResults = {};
            var fixupEdges = {};
            
            for(var edgeId in this.edges) {
                if (!this.unstableEdges[edgeId]) {
                    continue;
                }
                // FIXME need to add some randomness to this so that objects don't hit a
                // false equilibrium
                // FIXME should this be done node-wise, to more easily account for the number of edge
                // connections for a node?
                adjusted |= this.edgeObjects[edgeId].adjust(nodeAdjustResults, this.adjustIterations);
            }

            for (var nodeId in nodeAdjustResults) {
                var curPos = this.getNodePosition(nodeId);
                var newPos = nodeAdjustResults[nodeId];
                var distance = Vec3.distance(curPos, newPos);
                for (var edgeId in this.nodes[nodeId]) {
                    fixupEdges[edgeId] = true;
                }
                // logDebug("Moving node Id " + nodeId + " "  + (distance * 1000).toFixed(3) + " mm");
                Entities.editEntity(nodeId, {
                    position: newPos,
                    // DEBUGGING, flashes moved balls 
                    // color: COLORS.RED
                });
            }
            
            // DEBUGGING, flashes moved balls            
            //Script.setTimeout(function(){
            //    for (var nodeId in nodeAdjustResults) {
            //        Entities.editEntity(nodeId, { color: BALL_COLOR });
            //    }
            //}, ((UPDATE_INTERVAL * 1000) / 2));

            for (var edgeId in fixupEdges) {
                this.fixupEdge(edgeId);
            }

            
            if (!adjusted || this.adjustIterations > this.MAX_ADJUST_ITERATIONS) {
                if (adjusted) {
                    logDebug("Could not stabilized after " + this.MAX_ADJUST_ITERATIONS + " abandoning");
                }
                this.adjustIterations = 0;
                this.stable = true;
                this.unstableEdges = {};
            } 
        }
    }
}

MagBalls.prototype.createNodeEntity = function(customProperies) {
    var nodeId = Entities.addEntity(mergeObjects(BALL_PROTOTYPE, customProperies));
    return nodeId;
}

MagBalls.prototype.createEdgeEntity = function(nodeIdA, nodeIdB) {
    var apos = this.getNodePosition(nodeIdA);
    var bpos = this.getNodePosition(nodeIdB);
    var edgeId = Entities.addEntity(mergeObjects(EDGE_PROTOTYPE, {
        position: apos,
        linePoints: [ ZERO_VECTOR, Vec3.subtract(bpos, apos) ],
        userData: JSON.stringify({
            magBalls: {
                start: nodeIdA,
                end: nodeIdB,
                length: BALL_DISTANCE
            }
        })
    }));
    this.edgeObjects[edgeId] = new EdgeSpring(edgeId, this);
    return edgeId;
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
        
        // Check distance to attempt
        var distance = this.getNodeDistance(nodeId, otherNodeId);
        var variance = this.getVariance(distance);
        if (Math.abs(variance) > this.MAX_VARIANCE) {
            continue;
        }
        
        variances[otherNodeId] = variance;
    }
    return variances;
}

MagBalls.prototype.breakEdges = function(nodeId) {
    //var unstableNodes = this.findShape(Object.keys.target);
    //for (var node in unstableNodes) {
    //    this.unstableNodes[node] = true;
    //}
    Graph.prototype.breakEdges.call(this, nodeId);
}

MagBalls.prototype.createBall = function(position) {
    var created = this.createNode({ position: position });
    this.selectBall(created);
    return created;
}

MagBalls.prototype.selectBall = function(selected) {
    if (!selected) {
        return;
    }
    // stop updating shapes while manipulating
    this.stable = true;
    this.selectedNodes[selected] = true;
    this.breakEdges(selected);
}


MagBalls.prototype.releaseBall = function(releasedBall) {
    delete this.selectedNodes[releasedBall];
    logDebug("Released ball: " + releasedBall);
    
    var releasePosition = this.getNodePosition(releasedBall);
    this.stable = false;
    

    // iterate through the other balls and ensure we don't intersect with
    // any of them. If we do, just delete this ball and return.
    // FIXME (play a pop sound)
    for (var nodeId in this.nodes) {
        if (nodeId == releasedBall) {
            continue;
        }
        var distance = this.getNodeDistance(releasedBall, nodeId);
        if (distance < BALL_SIZE) {
            this.destroyNode(releasedBall);
            return;
        }
    }
    
    var targets = this.findPotentialEdges(releasedBall);
    if (!targets || !Object.keys(targets).length) {
//        this.destroyNode(releasedBall);
    }
    for (var otherBallId in targets) {
        this.createEdge(otherBallId, releasedBall);
    }

    var unstableNodes = this.findShape(releasedBall);
    for (var nodeId in unstableNodes) {
        for (var edgeId in this.nodes[nodeId]) {
            this.unstableEdges[edgeId] = true;
        }
    }
    this.validate();
}


MagBalls.prototype.findShape = function(nodeId) {
    var result = {};
    var queue = [ nodeId ];
    while (queue.length) {
        var curNode = queue.shift();
        if (result[curNode]) {
            continue;
        }
        result[curNode] = true;
        for (var otherNodeId in this.getConnectedNodes(curNode)) {
            queue.push(otherNodeId);
        }
    }
    return result;
}


MagBalls.prototype.getVariance = function(distance) {
    // FIXME different balls or edges might have different ideas of variance...
    // let something else handle this
    var offset = (BALL_DISTANCE - distance);
    var variance = offset / BALL_DISTANCE
    return variance;
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

MagBalls.prototype.destroyEdge = function(edgeId) {
    Graph.prototype.destroyEdge.call(this, edgeId);
    delete this.edgeObjects[edgeId];
}

MagBalls.prototype.destroyNode = function(nodeId) {
    Graph.prototype.destroyNode.call(this, nodeId);
}

// Scan the entity tree and load all the objects in range
MagBalls.prototype.refresh = function() {
    var ids = Entities.findEntities(MyAvatar.position, 50);
    for (var i in ids) {
        var id = ids[i];
        var properties = Entities.getEntityProperties(id);
        if (properties.name == BALL_NAME) {
            this.nodes[id] = {};
        }
    }

    var deleteEdges = [];
    for (var i in ids) {
        var id = ids[i];
        var properties = Entities.getEntityProperties(id);
        if (properties.name == EDGE_NAME) {
            var edgeId = id;
            this.edges[edgeId] = {};
            var magBallData = getMagBallsData(id);
            if (!magBallData.start || !magBallData.end) {
                logWarn("Edge information is missing for " + id);
                continue;
            }
            if (!this.nodes[magBallData.start] || !this.nodes[magBallData.end]) {
                logWarn("Edge " + id + " refers to unknown nodes: " + JSON.stringify(magBallData));
                Entities.editEntity(id, { color: COLORS.RED });
                deleteEdges.push(id);
                continue;
            }
            this.nodes[magBallData.start][edgeId] = true;
            this.nodes[magBallData.end][edgeId] = true;
            this.edges[edgeId][magBallData.start] = true;
            this.edges[edgeId][magBallData.end] = true;
            this.edgeObjects[id] = new EdgeSpring(id, this);
        }
    }

    if (deleteEdges.length) {
        Script.setTimeout(function() {
            for (var i in deleteEdges) {
                var edgeId = deleteEdges[i];
                //logDebug("deleting invalid edge " + edgeId);
                //Entities.deleteEntity(edgeId);
                Entities.editEntity(edgeId, {
                    color: COLORS.RED
                })
            }
        }, 1000);
    }

    var edgeCount = Object.keys(this.edges).length;
    var nodeCount = Object.keys(this.nodes).length;
    logDebug("Found " + nodeCount + " nodes and " + edgeCount + " edges ");
    this.validate();
}

		 
MagBalls.prototype.findEdgeParams = function(startBall, endBall) {
    var startBallPos = this.getNodePosition(startBall);	
    var endBallPos = this.getNodePosition(endBall);
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
 
MagBalls.prototype.onEntityAdded = function(entityId) {
    // We already have it
    if (this.nodes[entityId] || this.edges[entityId]) {
        return;
    }

    var properties = Entities.getEntityProperties(entityId);
    if (properties.name == BALL_NAME || properties.name == EDGE_NAME) {
        this.refreshNeeded = 1;
    }
}

findMatchingNode = function(position, nodePositions) {
    for (var nodeId in nodePositions) {
        var nodePos = nodePositions[nodeId];
        var distance = Vec3.distance(position, nodePos);
        if (distance < 0.03) {
            return nodeId;
        }
    }
}


MagBalls.prototype.repair = function() {
    // Find all the balls and record their positions
    var nodePositions = {};
    for (var nodeId in this.nodes) {
        nodePositions[nodeId] = this.getNodePosition(nodeId);
    }

    // Now check all the edges to see if they're valid (point to balls)
    // and ensure that the balls point back to them
    var ballsToEdges = {};
    
    // WARNING O(n^2) algorithm, every edge that is broken does
    // an O(N) search against the nodes
    for (var edgeId in this.edges) {
        var properties = Entities.getEntityProperties(edgeId);
        var startPos = properties.position;
        var endPos = Vec3.sum(startPos, properties.linePoints[1]);
        var magBallData = getMagBallsData(edgeId);
        var update = false;
        if (!magBallData.start) {
            var startNode = findMatchingNode(startPos, nodePositions);
            if (startNode) {
                logDebug("Found start node " + startNode)
                magBallData.start = startNode;
                update = true;
            }
        }
        if (!magBallData.end) {
            var endNode = findMatchingNode(endPos, nodePositions);
            if (endNode) {
                logDebug("Found end node " + endNode)
                magBallData.end = endNode;
                update = true;
            }
        }
        if (!magBallData.start || !magBallData.end) {
            logDebug("Didn't find both ends");
            this.destroyEdge(edgeId);
            continue;
        }
        if (!ballsToEdges[magBallData.start]) {
            ballsToEdges[magBallData.start] = [ edgeId ];
        } else {
            ballsToEdges[magBallData.start].push(edgeId);
        }
        if (!ballsToEdges[magBallData.end]) {
            ballsToEdges[magBallData.end] = [ edgeId ];
        } else {
            ballsToEdges[magBallData.end].push(edgeId);
        }
        if (update) {
            logDebug("Updating incomplete edge " + edgeId);
            magBallData.length = BALL_DISTANCE;
            setMagBallsData(edgeId, magBallData);
        }
    }
    for (var nodeId in ballsToEdges) {
        var magBallData = getMagBallsData(nodeId);
        var edges = magBallData.edges || [];
        var edgeHash = {};
        for (var i in edges) {
            edgeHash[edges[i]] = true;
        }
        var update = false;
        for (var i in ballsToEdges[nodeId]) {
            var edgeId = ballsToEdges[nodeId][i];
            if (!edgeHash[edgeId]) {
                update = true;
                edgeHash[edgeId] = true;
                edges.push(edgeId);
            }
        }
        if (update) {
            logDebug("Fixing node with missing edge data");
            magBallData.edges = edges;
            setMagBallsData(nodeId, magBallData);
        }
    }
}

