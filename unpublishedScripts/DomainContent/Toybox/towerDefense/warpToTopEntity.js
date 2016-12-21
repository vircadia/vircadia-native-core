(function() {
    function parseJSON(json) {
        try {
            return JSON.parse(json);
        } catch(e) {
            return undefined;
        }
    }

    var TeamArea = function() {
    };
    TeamArea.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;
        },
        enterEntity: function() {
            print("teamAreaEntity.js | Entered");
            var props = Entities.getEntityProperties(this.entityID, ['position', 'dimensions', 'registrationPoint']);
            var teleportPoint = props.position;
            teleportPoint.y += (props.dimensions.y * (1 - props.registrationPoint.y)) + 0.5;
            MyAvatar.position = teleportPoint;
        }
    };

    return new TeamArea();
});
