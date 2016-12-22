(function() {
    Script.include('utils.js');

    Enemy = function() {
    };
    Enemy.prototype = {
        preload: function(entityID) {
            print("Loaded enemy entity");
            this.entityID = entityID;
            Script.addEventHandler(entityID, "collisionWithEntity", this.onCollide.bind(this));

            this.entityIDsThatHaveCollidedWithMe = [];

            var userData = Entities.getEntityProperties(this.entityID, 'userData').userData;
            var data = utils.parseJSON(userData);
            if (data !== undefined && data.gameChannel !== undefined) {
                this.gameChannel = data.gameChannel;
            } else {
                print("targetEntity.js | ERROR: userData does not contain a game channel and/or team number");
            }
        },
        onCollide: function(entityA, entityB, collision) {
            if (this.entityIDsThatHaveCollidedWithMe.indexOf(entityB) > -1) {
                return;
            }
            this.entityIDsThatHaveCollidedWithMe.push(entityB);

            var colliderName = Entities.getEntityProperties(entityB, 'name').name;

            // If the other entity's name includes 'projectile' and we haven't hit it before,
            // continue on.
            if (colliderName.indexOf("projectile") > -1) {
                print("Collided with: ", entityB);
                Messages.sendMessage(this.gameChannel, JSON.stringify({
                    type: "enemy-killed",
                    entityID: this.entityID,
                }));
                Entities.deleteEntity(this.entityID);
            } else if (colliderName.indexOf("goal") > -1) {
                Messages.sendMessage(this.gameChannel, JSON.stringify({
                    type: "enemy-escaped",
                    entityID: this.entityID,
                    position: Entities.getEntityProperties(this.entityID, 'position').position
                }));
                Entities.deleteEntity(this.entityID);
            }
        }
    };

    return new Enemy();
});
