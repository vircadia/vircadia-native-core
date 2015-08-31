findMatchingNode = function(position, nodePositions) {
    for (var nodeId in nodePositions) {
        var nodePos = nodePositions[nodeId];
        var distance = Vec3.distance(position, nodePos);
        if (distance < 0.03) {
            return nodeId;
        }
    }
}

repairConnections = function() {
    var ids = Entities.findEntities(MyAvatar.position, 50);

    // Find all the balls and record their positions
    var nodePositions = {};
    for (var i in ids) {
        var id = ids[i];
        var properties = Entities.getEntityProperties(id);
        if (properties.name == BALL_NAME) {
            nodePositions[id] = properties.position;
        }
    }

    // Now check all the edges to see if they're valid (point to balls)
    // and ensure that the balls point back to them
    var ballsToEdges = {};
    for (var i in ids) {
        var id = ids[i];
        var properties = Entities.getEntityProperties(id);
        if (properties.name == EDGE_NAME) {
            var startPos = properties.position;
            var endPos = Vec3.sum(startPos, properties.linePoints[1]);
            var magBallData = getMagBallsData(id);
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
                Entities.deleteEntity(id);
                continue;
            }
            if (!ballsToEdges[magBallData.start]) {
                ballsToEdges[magBallData.start] = [ id ];
            } else {
                ballsToEdges[magBallData.start].push(id);
            }
            if (!ballsToEdges[magBallData.end]) {
                ballsToEdges[magBallData.end] = [ id ];
            } else {
                ballsToEdges[magBallData.end].push(id);
            }
            if (update) {
                logDebug("Updating incomplete edge " + id);
                magBallData.length = BALL_DISTANCE;
                setMagBallsData(id, magBallData);
            }
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
