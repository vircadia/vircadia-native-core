//
//  Created by Bradley Austin Davis on 2015/08/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// A collection of nodes and edges connecting them.
Graph = function() {
    /* Structure of nodes tree
        this.nodes: {
            nodeId1: {
                edgeId1: true
            }
            nodeId2: {
                edgeId1: true
            },
            // Nodes can many edges
            nodeId3: {
                edgeId2: true
                edgeId3: true
                edgeId4: true
                edgeId5: true
            },
            // Nodes can have 0 edges
            nodeId5: {
            },
            ...
        }
    */
    this.nodes = {};
    /* Structure of edge tree
        this.edges: {
            edgeId1: {
                // Every edge should have exactly two 
                nodeId1: true
                nodeId2: true
            },
            edgeId2: {
                nodeId3: true
                nodeId4: true
            },
            ...
        }
    */
    this.edges = {};
}

Graph.prototype.createNodeEntity = function(properties) {
    throw "Unimplemented";
}

Graph.prototype.createNode = function(properties) {
    var nodeId = this.createNodeEntity(properties);
    this.nodes[nodeId] = {};
    this.validate();
    return nodeId;
}

Graph.prototype.createEdgeEntity = function(nodeA, nodeB) {
    throw "Unimplemented";
}

Graph.prototype.createEdge = function(nodeA, nodeB) {
    if (nodeA == nodeB) {
        throw "Error: self connection not supported";
    }
    var newEdgeId = this.createEdgeEntity(nodeA, nodeB);

    // Create the bidirectional linkage
    this.edges[newEdgeId] = {};
    this.edges[newEdgeId][nodeA] = true;
    this.edges[newEdgeId][nodeB] = true;
    this.nodes[nodeA][newEdgeId] = true;
    this.nodes[nodeB][newEdgeId] = true;
    
    this.validate();
}

Graph.prototype.getEdges = function(nodeId) {
    var edges = this.nodes[nodeId];
    var result = {};
    for (var edgeId in edges) {
        for (var otherNodeId in this.edges[edgeId]) {
            if (otherNodeId != nodeId) {
                result[edgeId] = otherNodeId;
            }
        }
    }
    return result;
}

Graph.prototype.getConnectedNodes = function(nodeId) {
    var edges = this.getEdges(nodeId);
    var result = {};
    for (var edgeId in edges) {
        var otherNodeId = edges[edgeId];
        result[otherNodeId]  = edgeId;
    }
    return result;
}

Graph.prototype.getNodesForEdge = function(edgeId) {
    return Object.keys(this.edges[edgeId]);    
}

Graph.prototype.getEdgeLength = function(edgeId) {
    var nodesInEdge = Object.keys(this.edges[edgeId]);
    return this.getNodeDistance(nodesInEdge[0], nodesInEdge[1]);
}

Graph.prototype.getNodeDistance = function(a, b) {
    var apos = this.getNodePosition(a);
    var bpos = this.getNodePosition(b);
    return Vec3.distance(apos, bpos);
}

Graph.prototype.getNodePosition = function(node) {
    var properties = Entities.getEntityProperties(node);
    return properties.position;
}

Graph.prototype.breakEdges = function(nodeId) {
    for (var edgeId in this.nodes[nodeId]) {
        this.destroyEdge(edgeId);
    }
}

Graph.prototype.findNearestNode = function(position, maxDist) {
    var resultId = null;
    var resultDist = 0;
    for (var nodeId in this.nodes) {
        var nodePosition = this.getNodePosition(nodeId);
        var curDist = Vec3.distance(nodePosition, position);
        if (!maxDist || curDist <= maxDist) {
            if (!resultId || curDist < resultDist) {
                resultId = nodeId;
                resultDist = curDist;
            }
        }
    }
    return resultId;
}

Graph.prototype.findMatchingNodes = function(selector) {
    var result = {};
    for (var nodeId in this.nodes) {
        if (selector(nodeId)) {
            result[nodeId] = true;
        }
    }
    return result;
}

Graph.prototype.destroyEdge = function(edgeId) {
    logDebug("Deleting edge " + edgeId);
    for (var nodeId in this.edges[edgeId]) {
        delete this.nodes[nodeId][edgeId];
    }
    delete this.edges[edgeId];
    Entities.deleteEntity(edgeId);
    this.validate();
}

Graph.prototype.destroyNode = function(nodeId) {
    logDebug("Deleting node " + nodeId);
    this.breakEdges(nodeId);
    delete this.nodes[nodeId];
    Entities.deleteEntity(nodeId);
    this.validate();
}

Graph.prototype.deleteAll = function() {
    var nodeIds = Object.keys(this.nodes);
    for (var i in nodeIds) {
        var nodeId = nodeIds[i];
        this.destroyNode(nodeId);
    }
}

Graph.prototype.areConnected = function(nodeIdA, nodeIdB) {
    for (var edgeId in this.nodes[nodeIdA]) {
        if (this.nodes[nodeIdB][edgeId]) {
            return true;
        }
    }
    return false;
}

forEachValue = function(val, operation) {
    if( typeof val === 'string' ) {
        operation(val);
    } else if (typeof val === 'object') {
        if (val.constructor === Array) {
            for (var i in val) {
                operation(val[i]);
            }
        } else {
            for (var v in val) {
                operation(v);
            }
        }
    }
}

Graph.prototype.findShortestPath = function(start, end, options) {
    var queue = [ start ];
    var prev = {};
    if (options && options.exclude) {
        forEachValue(options.exclude, function(value) {
            prev[value] = value;
        });
        logDebug("exclude " + prev);
    }
    var found = false;
    while (!found && Object.keys(queue).length) {
        var current = queue.shift();
        for (var ballId in this.getConnectedNodes(current)) {
            if (prev[ballId]) {
                // already visited node
                continue;
            }
            // record optimal path
            prev[ballId] = current;
            if (ballId == end) {
                found = true;
                break;
            }
            queue.push(ballId);
        }
    }
    
    if (!found) {
        logDebug("Exhausted search");
        return;
    }
    
    var result = [ end ];
    while (result[0] != start) {
        result.unshift(prev[result[0]]);
    }

    return result;
}

Graph.prototype.validate = function() {
    var error = false;
    for (nodeId in this.nodes) {
        for (edgeId in this.nodes[nodeId]) {
            var edge = this.edges[edgeId];
            if (!edge) {
                logError("Error: node " + nodeId + " refers to unknown edge " + edgeId);
                error = true;
                continue;
            }
            if (!edge[nodeId]) {
                logError("Error: node " + nodeId + " refers to edge " + edgeId + " but not vice versa");
                error = true;
                continue;
            }
        }
    }
    
    for (edgeId in this.edges) {
        for (nodeId in this.edges[edgeId]) {
            var node = this.nodes[nodeId];
            if (!node) {
                logError("Error: edge " + edgeId + " refers to unknown node " + nodeId);
                error = true;
                continue;
            }
            if (!node[edgeId]) {
                logError("Error: edge " + edgeId + " refers to node " + nodeId + " but not vice versa");
                error = true;
                continue;
            }
        }
    }

    if (error) {
        logDebug(JSON.stringify({ edges: this.edges, balls: this.nodes }, null, 2));
    }
    return error;
}