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
            this.inEntity = false;
            var userData = Entities.getEntityProperties(this.entityID, 'userData').userData;
            var data = parseJSON(userData);
            if (data !== undefined && data.gameChannel) {
                this.gameChannel = data.gameChannel
                Messages.subscribe(this.gameChannel);
                Messages.messageReceived.connect(this, this.onMessageReceived);
            } else {
                print("teamAreaEntity.js | ERROR: userData does not contain a game channel");
            }
        },
        onMessageReceived: function(channel, message, sender) {
            if (channel === this.gameChannel) {
                print("teamAreaEntity.js | Got game channel message:", message);
                if (message == "FIGHT" && this.inEntity) {
                    // Set position to top of entity
                    var props = Entities.getEntityProperties(this.entityID, ['position', 'dimensions', 'registrationPoint']);
                    var teleportPoint = MyAvatar.position;
                    teleportPoint.y = props.position.y + (props.dimensions.y * (1 - props.registrationPoint.y)) + 0.5;
                    MyAvatar.position = teleportPoint;
                }
            }
        },
        enterEntity: function() {
            print("teamAreaEntity.js | Entered");
            this.inEntity = true;
            var props = Entities.getEntityProperties(this.entityID, ['position', 'dimensions', 'registrationPoint']);
            var teleportPoint = MyAvatar.position;
            teleportPoint.y = props.position.y + (props.dimensions.y * (1 - props.registrationPoint.y)) + 0.5;
            MyAvatar.position = teleportPoint;
        },
        leaveEntity: function() {
            print("teamAreaEntity.js | Exited");
            this.inEntity = false;
        }
    };

    return new TeamArea();
});
