//
//  Created by Bradley Austin Davis on 2015/08/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function(){
    this.preload = function(entityId) { 
        this.MIN_CHECK_INTERVAL = 0.05;
        this.MAX_VARIANCE = 0.005;
        this.ZERO_VECTOR = { x: 0, y: 0, z: 0 };

        this.entityId = entityId;
        var properties = Entities.getEntityProperties(this.entityId);
        var userData = JSON.parse(properties.userData);
        this.start = userData.magBalls.start;
        this.end = userData.magBalls.end;
        this.desiredLength = userData.magBalls.length;
        this.timeSinceLastUpdate = 0;
        this.nextCheckInterval = this.MIN_CHECK_INTERVAL;

        // FIXME do I really need to do this nonsense?
        var _this = this;
        this.updateWrapper = function(deltaTime) {
            _this.onUpdate(deltaTime);
        };
        Script.update.connect(this.updateWrapper);

        Script.scriptEnding.connect(function() {
            _this.onCleanup();
        });
        Entities.deletingEntity.connect(function(entityId) {
            _this.onCleanup();
        });
    }; 
    
    this.onUpdate = function(deltaTime) {
        this.timeSinceLastUpdate += deltaTime;
        if (this.timeSinceLastUpdate > this.nextCheckInterval) {
            this.updateProperties();
            this.timeSinceLastUpdate = 0;
            var length = this.getLength();
            if (length == 0) {
                this.onCleanup();
                return;
            }
            var variance = this.getVariance(length);
            if (Math.abs(variance) <= this.MAX_VARIANCE) {
                this.incrementCheckInterval();
                return;
            }
            this.decrementCheckInterval();
            var adjustmentVector = Vec3.multiply(variance / 4, this.vector);
            var newPosition = Vec3.sum(Vec3.multiply(-1, adjustmentVector), this.position);
            var newVector = Vec3.sum(Vec3.multiply(2, adjustmentVector), this.vector);
            var newLength = Vec3.length(newVector);
            var newVariance = this.getVariance(newLength);
            Entities.editEntity(this.entityId, {
                position: newPosition,
                linePoints: [ this.ZERO_VECTOR, newVector ]
            });
            Entities.editEntity(this.start, {
                position: newPosition
            });
            Entities.editEntity(this.end, {
                position: Vec3.sum(newPosition, newVector)
            });
        }
    }
    
    this.incrementCheckInterval = function() {
        this.nextCheckInterval = Math.min(this.nextCheckInterval * 2.0, 1.0);        
    }

    this.decrementCheckInterval = function() {
        this.nextCheckInterval = 0.05;
    }
    
    this.onCleanup = function() {
        if (this.updateWrapper) {
            Script.update.disconnect(this.updateWrapper);
            delete this.updateWrapper;
        }
    }
    
    this.getVariance = function(length) {
        if (!length) {
            length = this.getLength();
        }
        var difference = this.desiredLength - length;
        return difference / this.desiredLength;
    }

    this.getLength = function() {
        return Vec3.length(this.vector);
    }

    this.getPosition = function(entityId) {
        var properties = Entities.getEntityProperties(entityId);
        return properties.position;
    }
    
    this.updateProperties = function() {
        var properties = Entities.getEntityProperties(this.entityId);
        var curStart = properties.position;
        var curVector = properties.linePoints[1]
        var curEnd = Vec3.sum(curVector, curStart);
        var startPos = this.getPosition(this.start);
        var endPos = this.getPosition(this.end);
        var startError = Vec3.distance(curStart, startPos);
        var endError = Vec3.distance(curEnd, endPos);
        this.vector = Vec3.subtract(endPos, startPos);
        if (startError > 0.005 || endError > 0.005) {
            Entities.editEntity(this.entityId, {
                position: startPos,
                linePoints: [ this.ZERO_VECTOR, this.vector ]
            });
        }
        this.position = startPos;
    }
});
