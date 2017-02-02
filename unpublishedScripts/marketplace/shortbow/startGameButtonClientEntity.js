(function() {
    Script.include('utils.js');

    StartButton = function() {
    };
    StartButton.prototype = {
        preload: function(entityID) {
            print("Preloading start button");
            this.entityID = entityID;
            this.commChannel = "shortbow-" + Entities.getEntityProperties(entityID, 'parentID').parentID;
            Script.addEventHandler(entityID, "collisionWithEntity", this.onCollide.bind(this));
        },
        signalAC: function() {
            print("Button pressed");
            print("Sending message to: ", this.commChannel);
            Messages.sendMessage(this.commChannel, JSON.stringify({
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
