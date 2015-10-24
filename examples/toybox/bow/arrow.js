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
        glowBox: null,
        preload: function(entityID) {
            this.entityID = entityID;
            if (this.glowBox === null) {
                this.createGlowBox();
                Script.update.connect(this.updateGlowBoxPosition);
            }
        },
        unload: function() {
            Script.update.disconnect(this.updateGlowBoxPosition);
            Entities.deleteEntity(this.glowBox);
        },
        createGlowBox: function() {
            var glowBowProperties = {
                name: 'Arrow Glow Box',
                type: 'Box',
                dimensions: {
                    x: 0.02,
                    y: 0.02,
                    z: 0.64
                },
                color: {
                    red: 255,
                    green: 0,
                    blue: 255
                },
            }
            _this.glowBow = Entities.addEntity(glowBowProperties);
        },
        updateGlowBoxPosition: function() {
            var arrowProperties = Entities.getEntityProperties(_this.entityID, ["position", "rotation"]);
            //once parenting is available, just attach the glowbow to the arrow
            Entities.editEntity(_this.entityID, {
                position: arrowProperties.position,
                rotation: arrowProperties.rotation
            })
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