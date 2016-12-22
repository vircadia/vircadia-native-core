(function() {
    Script.include('utils.js');

    StartButton = function() {
    };
    StartButton.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;
            Script.addEventHandler(entityID, "collisionWithEntity", this.onCollide.bind(this));
        },
        signalAC: function() {
            print("Button pressed");
            var userData = Entities.getEntityProperties(this.entityID, ["userData"]).userData;
            print("Sending message to: ", JSON.parse(userData).gameChannel);
            Messages.sendMessage(JSON.parse(userData).gameChannel, JSON.stringify({
                type: 'start-game'
            }));
        },
        onCollide: function(entityA, entityB, collision) {
            print("Collided with: ", entityB);

            var colliderName = Entities.getEntityProperties(entityB, 'name').name;

            if (colliderName.indexOf("projectile") > -1) {
                this.signalAC();
            }
        }
    };

    StartButton.prototype.startNearTrigger = StartButton.prototype.signalAC;
    StartButton.prototype.startFarTrigger = StartButton.prototype.signalAC;
    StartButton.prototype.clickDownOnEntity = StartButton.prototype.signalAC;

    return new StartButton();
});
