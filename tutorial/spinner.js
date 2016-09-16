(function() {
    var spinnerSound = SoundCache.getSound("http://hifi-content.s3.amazonaws.com/DomainContent/Tutorial/Sounds/Pinwheel.L.wav");
    var Spinner = function() {
    };
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
    Spinner.prototype = {
        onLit: function() {
            print("LIT SPINNER", this.entityID);
            Entities.editEntity(this.entityID, {
                "angularDamping": 0.1,
                "angularVelocity": {
                    "x": 20.471975326538086,
                    "y": 0,
                    "z": 0
                },
            });
            var injector = Audio.playSound(spinnerSound, {
                position: Entities.getEntityProperties(this.entityID, 'position').position,
                volume: 1.0,
                loop: false 
            });

                print("HERE2");
            var childrenProps = getChildProperties(this.entityID, ['type']);
            for (var childEntityID in childrenProps) {
                var props = childrenProps[childEntityID];
                if (props.type == "ParticleEffect") {
                    Entities.editEntity(childEntityID, {
                        emitRate: 140,
                    });
                }
            }
            Messages.sendLocalMessage("Tutorial-Spinner", "wasLit");

            var self = this;
            Script.setTimeout(function() {
                print("BLOW UP");
                injector.stop();

                print("HERE");
                var childrenProps = getChildProperties(self.entityID, ['type']);
                for (var childEntityID in childrenProps) {
                    var props = childrenProps[childEntityID];
                    if (props.type == "ParticleEffect") {
                        Entities.editEntity(childEntityID, {
                            emitRate: 0,
                        });
                    }
                }
            }, 4900);
        },
        preload: function(entityID) {
            this.entityID = entityID;
        },
    };
    return new Spinner();
});
