if (!Function.prototype.bind) {
  Function.prototype.bind = function(oThis) {
    if (typeof this !== 'function') {
      // closest thing possible to the ECMAScript 5
      // internal IsCallable function
      throw new TypeError('Function.prototype.bind - what is trying to be bound is not callable');
    }

    var aArgs   = Array.prototype.slice.call(arguments, 1),
        fToBind = this,
        fNOP    = function() {},
        fBound  = function() {
          return fToBind.apply(this instanceof fNOP
                 ? this
                 : oThis,
                 aArgs.concat(Array.prototype.slice.call(arguments)));
        };

    if (this.prototype) {
      // Function.prototype doesn't have a prototype property
      fNOP.prototype = this.prototype;
    }
    fBound.prototype = new fNOP();

    return fBound;
  };
}

(function() {
    function parseJSON(json) {
        try {
            return JSON.parse(json);
        } catch(e) {
            return undefined;
        }
    }

    Enemy = function() {
    };
    Enemy.prototype = {
        preload: function(entityID) {
            print("Loaded enemy entity");
            this.entityID = entityID;
            Script.addEventHandler(entityID, "collisionWithEntity", this.onCollide.bind(this));

            this.entityIDsThatHaveCollidedWithMe = [];

            var userData = Entities.getEntityProperties(this.entityID, 'userData').userData;
            var data = parseJSON(userData);
            if (data !== undefined && data.gameChannel !== undefined) {
                this.gameChannel = data.gameChannel;
            } else {
                print("targetEntity.js | ERROR: userData does not contain a game channel and/or team number");
            }
        },
        onCollide: function(entityA, entityB, collision) {
            print("Collided with: ", entityB);
            if (this.entityIDsThatHaveCollidedWithMe.indexOf(entityB) > -1) {
                return;
            }
            this.entityIDsThatHaveCollidedWithMe.push(entityB);

            var colliderName = Entities.getEntityProperties(entityB, 'name').name;

            // If the other entity's name includes 'projectile' and we haven't hit it before,
            // continue on.
            if (colliderName.indexOf("projectile") > -1) {
                Messages.sendMessage(this.gameChannel, JSON.stringify({
                    type: "enemy-killed",
                    entityID: this.entityID,
                }));
                Entities.deleteEntity(this.entityID);
            } else if (colliderName.indexOf("goal") > -1) {
                Messages.sendMessage(this.gameChannel, JSON.stringify({
                    type: "enemy-escaped",
                    entityID: this.entityID,
                }));
                Entities.deleteEntity(this.entityID);
            }
        }
    };

    return new Enemy();
});
