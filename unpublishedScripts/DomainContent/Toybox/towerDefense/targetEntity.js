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

    Target = function() {
    }
    Target.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;
            Script.addEventHandler(entityID, "collisionWithEntity", this.onCollide.bind(this));

            this.entityIDsThatHaveCollidedWithMe = [];

            var userData = Entities.getEntityProperties(this.entityID, 'userData').userData;
            var data = parseJSON(userData);
            if (data !== undefined && data.gameChannel !== undefined && data.teamNumber !== undefined) {
                this.gameChannel = data.gameChannel;
                this.teamNumber = data.teamNumber;
            } else {
                print("targetEntity.js | ERROR: userData does not contain a game channel and/or team number");
            }
        },
        onCollide: function(entityA, entityB, collision) {
            print("Collided with: ", entityB);
            var colliderName = Entities.getEntityProperties(entityB, 'name').name;

            // If the other entity's name includes 'projectile' and we haven't hit it before,
            // continue on.
            if (colliderName.indexOf("projectile") > -1
                    && this.entityIDsThatHaveCollidedWithMe.indexOf(entityB) === -1) {
                this.entityIDsThatHaveCollidedWithMe.push(entityB);
                Messages.sendMessage(this.gameChannel, JSON.stringify({
                    type: "target-hit",
                    entityID: this.entityID,
                    teamNumber: this.teamNumber
                }));
            }
        }
    };

    return new Target();
});
