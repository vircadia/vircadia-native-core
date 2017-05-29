//
//  Created by Thijs Wenker on 3/31/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var _this;
    var DO_NOT_SNAP_WHEN_FARTHER_THAN_THIS = 0.25;

    function SnapToGrid() {
        _this = this;
    }

    SnapToGrid.prototype = {
        preload: function(id) {
            _this.entityID = id;
            _this.getAnchorPoints();
        },
        startGrab: function() {
            var userData = _this.getCurrentUserData();
            if (userData.gameTable.hasOwnProperty('attachedTo') && userData.gameTable.attachedTo !== null) {
                Entities.editEntity(userData.gameTable.attachedTo, {
                    userData: 'available'
                });
            }
        },
        releaseGrab: function() {
            _this.attachToNearestAnchor();
        },
        getAnchorPoints: function() {
            var availableAnchors = [];
            var results = _this.getEntitiesFromGroup('gameTable', 'anchor');
            results.forEach(function(item) {
                var properties = Entities.getEntityProperties(item, ['position', 'userData']);
                if (properties.userData === 'occupied') {
                    // don't put it on the stack
                } else if (properties.userData === 'available') {
                    availableAnchors.push(properties.position);
                }
            });
            return availableAnchors;
        },
        attachToNearestAnchor: function() {
            var myProps = Entities.getEntityProperties(_this.entityID, ['position', 'dimensions']);
            var anchors = _this.getAnchorPoints();
            var shortestDistance = DO_NOT_SNAP_WHEN_FARTHER_THAN_THIS;
            var nearestAnchor = null;
            anchors.forEach(function(anchor) {
                var howFar = Vec3.distance(myProps.position, anchor);
                if (howFar < shortestDistance) {
                    shortestDistance = howFar;
                    nearestAnchor = anchor;
                }
            });

            if (shortestDistance > DO_NOT_SNAP_WHEN_FARTHER_THAN_THIS) {
                _this.setCurrentUserData({
                    attachedTo: null
                });
            } else {
                if (nearestAnchor !== null) {
                    _this.positionOnAnchor(nearestAnchor, myProps);
                    _this.setCurrentUserData({
                        attachedTo: nearestAnchor
                    });
                    Entities.editEntity(nearestAnchor, {
                        userData: 'occupied'
                    });
                } else {
                    // there is no nearest anchor.  perhaps they are all occupied.
                    _this.setCurrentUserData({
                        attachedTo: null
                    });
                }
            }

        },
        getEntitiesFromGroup: function(groupName, entityName) {
            var position = Entities.getEntityProperties(_this.entityID, 'position').position;
            var nearbyEntities = Entities.findEntities(position, 7.5);
            var foundItems = [];
            nearbyEntities.forEach(function(entityID) {
                var description = Entities.getEntityProperties(entityID, 'description').description;
                var descriptionSplit = description.split(":");
                if (descriptionSplit[1] === groupName && descriptionSplit[2] === entityName) {
                    foundItems.push(entityID);
                }
            });
            return foundItems;
        },
        positionOnAnchor: function(anchor, myProps) {
            Entities.editEntity(_this.entityID, {
                position: {
                    x: anchor.x,
                    y: anchor.y + (0.5 * myProps.dimensions.y),
                    z: anchor.z
                }
            });
        },
        setCurrentUserData: function(data) {
            var userData = _this.getCurrentUserData();
            userData.gameTable = data;
            Entities.editEntity(_this.entityID, {
                userData: userData
            });
        },
        getCurrentUserData: function() {
            var userData = Entities.getEntityProperties(_this.entityID, 'userData').userData;
            try {
                return JSON.parse(userData);
            } catch (e) {
                // e;
            }
            return null;
        }
    };
});
