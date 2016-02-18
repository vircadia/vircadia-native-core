//
//  ballDetector.js
//
//  Script Type: Entity
//
//  Created by James B. Pollack @imgntn on 2/15/2016
//  Copyright 2016 High Fidelity, Inc.
//
//
//  This script resets a ball to its original position when the ball enters it.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
(function() {
    var BALL_DISTANCE_THRESHOLD;
    var _this;

    function BallDetctor() {
        _this = this;
        return;
    }

    BallDetctor.prototype = {
        preload:function(entityID){
            this.entityID = entityID,
            this.maze = Entities.getEntityProperties(entityID,'parentID').parentID;
        },
        enterEntity: function() {
            print('BALL ENTERED BALL DETECTOR!!')
             Entities.callEntityMethod(this.maze,'destroyBall');
             Entities.callEntityMethod(this.maze,'createBall');
        }
    };

    return new BallDetctor();
});