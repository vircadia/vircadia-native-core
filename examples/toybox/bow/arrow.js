//
//  arrow.js 
//
//  This script attaches to an arrow to make it stop and stick when it hits something.  Could use this to make it a fire arrow or really give any kind of property to itself or the entity it hits.
//
//  Created by James B. Pollack @imgntn on 10/19/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {

    var _this;

    function Arrow() {
        _this = this;
        return;
    }

    Arrow.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;
        },
        collisionWithEntity: function(me, otherEntity, collision) {

            Vec3.print('penetration = ', collision.penetration);
            Vec3.print('collision contact point = ', collision.contactPoint);

            Entities.editEntity(this.entityID, {
                velocity: {
                    x: 0,
                    y: 0,
                    z: 0
                },
                gravity: {
                    x: 0,
                    y: 0,
                    z: 0
                },
                collisionsWillMove: false

            })
        }
    }

    return new Arrow;
})