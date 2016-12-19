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

function parseJSON(json) {
    try {
        return JSON.parse(json);
    } catch (e) {
        return undefined;
    }
}

// The destructible entity is expected to have a few different properties:
//   It is a model
//   It has a set of "originalTextures" that follows the format:
//
//       tex.health1: "atp:/..."
//       tex.health2: "atp:/..."
//       tex.health3: "atp:/..."
//       tex.health4: "atp:/..."
//       ...
//
//   The model can have as many textures as you would like. Each time the entity
//   is hit, the next texture in the list will be set. If the entity is hit and
//   it is at the last texture, it will be destroyed. The first texture is always
//   tex.health1.

// This is the key used to set the current texture.
var TEXTURE_NAME = "tex.health1";

(function() {
    Script.include("block.js");
    DestructibleBlock = function() {
    }
    DestructibleBlock.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;
            Script.addEventHandler(entityID, "collisionWithEntity", this.onCollide.bind(this));

            this.entityIDsThatHaveCollidedWithMe = [];
        },
        onCollide: function(entityA, entityB, collision) {
            print("Collided with: ", entityB);
            var colliderName = Entities.getEntityProperties(entityB, 'name').name;

            // If the other entity's name includes 'projectile' and we haven't hit it before,
            // continue on.
            print('len', this.entityIDsThatHaveCollidedWithMe.length);
            if (colliderName.indexOf("projectile") > -1
                    && this.entityIDsThatHaveCollidedWithMe.indexOf(entityB) === -1) {
                this.entityIDsThatHaveCollidedWithMe.push(entityB);
                this.decrementHealth();
            }
        },
        decrementHealth: function() {
            // FIXME This doesn't need to be recalculated, but it can't be done in preload
            // Textures stores a list of the texture paths that the model contains
            // The first texture indicates full_health, the next indicates full_health-1,
            //   and so on and so forth.
            this.textures = [];
            var originalTextures = parseJSON(
                Entities.getEntityProperties(this.entityID, 'originalTextures').originalTextures);
            for (var i = 0;; ++i) {
                var nextTextureName = "tex.health" + (i + 1);
                if (nextTextureName in originalTextures) {
                    print(i, originalTextures[nextTextureName]);
                    this.textures.push(originalTextures[nextTextureName]);
                } else {
                    break;
                }
            }
            print("Decrementing health");

            var texturesJSON = Entities.getEntityProperties(this.entityID, 'textures').textures;
            var textures = parseJSON(texturesJSON);

            var currentTextureIndex = 0;

            if (textures === undefined) {
                print("ERROR (tdBlock.js) | Textures contains invalid json");
            } else {
                const currentTexture = textures[TEXTURE_NAME];

                var found = false;

                // Go through each of the known textures to see which is currently set
                for (var i = 0; i < this.textures.length; ++i) {
                    print("tdBlock.js | Checking ", i, this.textures[i], currentTexture);
                    if (this.textures[i].indexOf(currentTexture) > -1) {
                        currentTextureIndex = i;
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    print("ERROR (tdBlock.js) | Could not find current texture index");
                }
            }
            print("tdBlock.js | Current texture index is:", currentTextureIndex);

            // FIXME DELETE ME
            if (currentTextureIndex === this.textures.length - 1) {
                //currentTextureIndex = -1;
            }

            if (currentTextureIndex === this.textures.length - 1) {
                // We've reached the last texture, let's destroy the entity
                Entities.deleteEntity(this.entityID);
                print("tdBlock.js | Destroying entity");
            } else {
                var newTextures = {};
                newTextures[TEXTURE_NAME] = this.textures[currentTextureIndex + 1];
                var newTexturesJSON = JSON.stringify(newTextures);
                Entities.editEntity(this.entityID, { textures: newTexturesJSON });
                print("tdBlock.js | Setting texture to: ", newTexturesJSON);
            }
        },
        growBlock: function() {
            var props = getBlockProperties();
            props.position = Entities.getEntityProperties(this.entityID, 'position').position;
            props.position.y += props.dimensions.y;
            Entities.addEntity(props);
        },
        startNearTrigger: function () {
            print("launch.js | got start near trigger");
            this.growBlock();
        },
        startFarTrigger: function () {
            print("launch.js | got start far trigger");
            this.growBlock();
        },
        clickDownOnEntity: function () {
            print("launch.js | got click down");
            this.growBlock();
        }
    };

    return new DestructibleBlock();
});
