(function() {
    var _this;
    var DONT_SNAP_WHEN_FARTHER_THAN_THIS = 0.25;

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
            var results = _this.getEntityFromGroup('gameTable', 'anchor');
            results.forEach(function(item) {
                var props = Entities.getEntityProperties(item);
                if (props.userData === 'occupied') {
                    //don't put it on the stack
                } else if (props.userData === 'available') {
                    availableAnchors.push(props.position);
                }
            });
            return availableAnchors;
        },
        attachToNearestAnchor: function() {
            var myProps = Entities.getEntityProperties(_this.entityID);
            var anchors = _this.getAnchorPoints();
            var shortestDistance = DONT_SNAP_WHEN_FARTHER_THAN_THIS;
            var nearestAnchor = null;
            anchors.forEach(function(anchor) {
                var howFar = Vec3.distance(myProps.position, anchor);
                if (howFar < shortestDistance) {
                    shortestDistance = howFar;
                    nearestAnchor = anchor;
                }
            });

            if (shortestDistance > DONT_SNAP_WHEN_FARTHER_THAN_THIS) {
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
                    //there is no nearest anchor.  perhaps they are all occupied.
                    _this.setCurrentUserData({
                        attachedTo: null
                    });
                }
            }

        },
        getEntityFromGroup: function(groupName, entityName) {
            var props = Entities.getEntityProperties(_this.entityID);
            var results = Entities.findEntities(props.position, 7.5);
            var found;
            results.forEach(function(item) {
                var itemProps = Entities.getEntityProperties(item);
                var descriptionSplit = itemProps.description.split(":");
                if (descriptionSplit[1] === groupName && descriptionSplit[2] === entityName) {
                    return item;
                }
            });
        },
        positionOnAnchor: function(anchor, myProps) {
            Entities.editEntity(_this.entityID, {
                position: {
                    x: anchor.x,
                    y: anchor.y + (0.5 * myProps.dimensions.y),
                    z: anchor.z
                }
            })
        },
        setCurrentUserData: function(data) {
            var userData = getCurrentUserData();
            userData.gameTable = data;
            Entities.editEntity(_this.entityID, {
                userData: userData
            });
        },
        getCurrentUserData: function() {
            var props = Entities.getEntityProperties(_this.entityID);
            var json = null;
            try {
                json = JSON.parse(props.userData);
            } catch (e) {
                return;
            }
            return json;
        }
    }
});
