
EdgeSpring = function(edgeId, graph) {
    this.edgeId = edgeId;
    this.graph = graph;

    var magBallsData = getMagBallsData(this.edgeId);
    this.start = magBallsData.start;
    this.end = magBallsData.end;
    this.desiredLength = magBallsData.length || BALL_DISTANCE;
}

// FIXME as iterations increase, start introducing some randomness
// to the adjustment so that we avoid false equilibriums
// Alternatively, larger iterations could increase the acceptable variance
EdgeSpring.prototype.adjust = function(results, iterations) {
    var startPos = this.getAdjustedPosition(this.start, results);
    var endPos = this.getAdjustedPosition(this.end, results);
    var vector = Vec3.subtract(endPos, startPos);
    var length = Vec3.length(vector);
    var variance = this.getVariance(length);

    if (Math.abs(variance) <= this.MAX_VARIANCE) {
        return false;
    }

    // adjust by halves until we fall below our variance
    var adjustmentVector = Vec3.multiply(variance / 4, vector);

    var newStartPos = Vec3.sum(Vec3.multiply(-1, adjustmentVector), startPos);
    var newEndPos = Vec3.sum(adjustmentVector, endPos);
    results[this.start] = newStartPos;
    results[this.end] = newEndPos;
    return true;
}

EdgeSpring.prototype.MAX_VARIANCE = 0.005;

EdgeSpring.prototype.getAdjustedPosition = function(nodeId, results) {
    if (results[nodeId]) {
        return results[nodeId];
    }
    return this.graph.getNodePosition(nodeId);
}

EdgeSpring.prototype.getVariance = function(length) {
    var difference = this.desiredLength - length;
    return difference / this.desiredLength;
}
