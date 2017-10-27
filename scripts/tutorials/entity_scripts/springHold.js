//
//  springHold.js
//  
//  Created by Philip Rosedale on March 18, 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Attach this entity script to a model or basic shape and grab it.  The object will 
//  follow your hand like a spring, allowing you, for example, to swordfight with others.    
//  
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {

    var TIMESCALE = 0.03;
    var ACTION_TTL = 10;
    var _this;

    function SpringHold() {
        _this = this;
    }

    function updateSpringAction(timescale) {
        
        var targetProps = Entities.getEntityProperties(_this.entityID);
        var props = {
            targetPosition: targetProps.position,
            targetRotation: targetProps.rotation,
            linearTimeScale: timescale,
            angularTimeScale: timescale,
            ttl: ACTION_TTL
        };
        Entities.updateAction(_this.copy, _this.actionID, props);
    }

    function createSpringAction(timescale) {

        var targetProps = Entities.getEntityProperties(_this.entityID);
        var props = {
            targetPosition: targetProps.position,
            targetRotation: targetProps.rotation,
            linearTimeScale: timescale,
            angularTimeScale: timescale,
            ttl: ACTION_TTL
        };
        _this.actionID = Entities.addAction("spring", _this.copy, props);
    }

    function createCopy() {
        var originalProps = Entities.getEntityProperties(_this.entityID);
        var props = {
            type: originalProps.type,
            color: originalProps.color,
            modelURL: originalProps.modelURL,
            dimensions: originalProps.dimensions,
            dynamic: true,
            damping: 0.0,
            angularDamping: 0.0,
            collidesWith: 'dynamic,static,kinematic',
            rotation: originalProps.rotation,
            position: originalProps.position,
            shapeType: originalProps.shapeType,
            visible: true,
            userData:JSON.stringify({
                grabbableKey:{
                    grabbable:false
                }
            })
        };
        _this.copy = Entities.addEntity(props);
    }

    function deleteCopy() {
        Entities.deleteEntity(_this.copy);
    }

    function makeOriginalInvisible() {
        Entities.editEntity(_this.entityID, {
            visible: false,
            collisionless: true
        });
    }

    function makeOriginalVisible() {
        Entities.editEntity(_this.entityID, {
            visible: true,
            collisionless: false
        });
    }

    function deleteSpringAction() {
        Entities.deleteAction(_this.copy, _this.actionID);
    }

    SpringHold.prototype = {
        preload: function(entityID) {
            _this.entityID = entityID;
        },
        startNearGrab: function(entityID, data) {
            createCopy();
            createSpringAction(TIMESCALE);
            makeOriginalInvisible();
        },
        continueNearGrab: function() {
            updateSpringAction(TIMESCALE);
        },
        releaseGrab: function() {
            deleteSpringAction();
            deleteCopy();
            makeOriginalVisible();
        }
    };

    return new SpringHold();
});
