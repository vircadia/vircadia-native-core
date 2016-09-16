(function() {
    var findEntity = function(properties, searchRadius, filterFn) {
        var entities = findEntities(properties, searchRadius, filterFn);
        return entities.length > 0 ? entities[0] : null;
    }

    // Return all entities with properties `properties` within radius `searchRadius`
    var findEntities = function(properties, searchRadius, filterFn) {
        if (!filterFn) {
            filterFn = function(properties, key, value) {
                return value == properties[key];
            }
        }
        searchRadius = searchRadius ? searchRadius : 100000;
        var entities = Entities.findEntities({ x: 0, y: 0, z: 0 }, searchRadius);
        var matchedEntities = [];
        var keys = Object.keys(properties);
        for (var i = 0; i < entities.length; ++i) {
            var match = true;
            var candidateProperties = Entities.getEntityProperties(entities[i], keys);
            for (var key in properties) {
                if (!filterFn(properties, key, candidateProperties[key])) {
                    // This isn't a match, move to next entity
                    match = false;
                    break;
                }
            }
            if (match) {
                matchedEntities.push(entities[i]);
            }
        }

        return matchedEntities;
    }

    var fuseSound = SoundCache.getSound("https://hifi-content.s3.amazonaws.com/DomainContent/Tutorial/Sounds/fuse.wav");
    function getChildProperties(entityID, propertyNames) {
        var childEntityIDs = Entities.getChildrenIDs(entityID);
        var results = {}
        for (var i = 0; i < childEntityIDs.length; ++i) {
            var childEntityID = childEntityIDs[i];
            var properties = Entities.getEntityProperties(childEntityID, propertyNames);
            results[childEntityID] = properties;
        }
        return results;
    }
    var Fuse = function() {
    };
    Fuse.prototype = {
        light: function() {
            print("LIT", this.entityID);
            var anim = Entities.getEntityProperties(this.entityID, ['animation']).animation;
            print("anim: ", anim.currentFrame, Object.keys(anim));

            if (anim.currentFrame < 140) {
                return;
            }
            Entities.editEntity(this.entityID, {
                animation: {
                    currentFrame: 1,
                    lastFrame: 150,
                    running: 1,
                    url: "https://hifi-content.s3.amazonaws.com/ozan/dev/anim/fuse/fuse.fbx",
                    loop: 0
                },
            });
            var injector = Audio.playSound(fuseSound, {
                position: Entities.getEntityProperties(this.entityID, 'position').position,
                volume: 0.7,
                loop: true 
            });

            var childrenProps = getChildProperties(this.entityID, ['type']);
            for (var childEntityID in childrenProps) {
                var props = childrenProps[childEntityID];
                if (props.type == "ParticleEffect") {
                    Entities.editEntity(childEntityID, {
                        emitRate: 140,
                    });
                } else if (props.type == "Light") {
                    Entities.editEntity(childEntityID, {
                        visible: true,
                    });
                }
            }

            var self = this;
            Script.setTimeout(function() {
                print("BLOW UP");
                var spinnerID = findEntity({ name: "tutorial/equip/spinner" }, 20); 
                Entities.callEntityMethod(spinnerID, "onLit");
                injector.stop();

                var childrenProps = getChildProperties(self.entityID, ['type']);
                for (var childEntityID in childrenProps) {
                    var props = childrenProps[childEntityID];
                    if (props.type == "ParticleEffect") {
                        Entities.editEntity(childEntityID, {
                            emitRate: 0,
                        });
                    } else if (props.type == "Light") {
                        Entities.editEntity(childEntityID, {
                            visible: false,
                        });
                    }
                }

            }, 4900);
        },
        preload: function(entityID) {
            this.entityID = entityID;
        },
    };
    return new Fuse();
});
