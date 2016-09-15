(function() {
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
        onLit: function() {
            print("LIT", this.entityID);
            Entities.editEntity(this.entityID, {
                animation: {
                    currentFrame: 0,
                    //"lastFrame": 130,
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
                Entities.callEntityMethod("{dd13fcd5-616f-4749-ab28-2e1e8bc512e9}", "onLit");
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
