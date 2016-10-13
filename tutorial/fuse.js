//
//  fuse.js
//
//  Created by Ryan Huffman on 9/1/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    Script.include('utils.js');

    var DEBUG = true;
    function debug() {
        if (DEBUG) {
            var args = Array.prototype.slice.call(arguments);
            args.unshift("fuse.js | ");
            print.apply(this, args);
        }
    }

    var active = false;

    var fuseSound = SoundCache.getSound("atp:/tutorial_sounds/fuse.wav");
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
            debug("Received light()", this.entityID);

            var visible = Entities.getEntityProperties(this.entityID, ['visible']).visible;
            if (!visible) {
                debug("Fuse is not visible, returning");
                return;
            }

            if (active) {
                debug("Fuse is active, returning");
                return;
            }
            active = true;

            Entities.editEntity(this.entityID, {
                animation: {
                    currentFrame: 1,
                    lastFrame: 150,
                    running: 1,
                    url: "atp:/tutorial_models/fuse.fbx",
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
                debug("Updating: ", childEntityID);
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
                debug("Setting off fireworks");
                //var spinnerID = Utils.findEntity({ name: "tutorial/equip/spinner" }, 20); 
                var spinnerID = "{dd13fcd5-616f-4749-ab28-2e1e8bc512e9}";
                Entities.callEntityMethod(spinnerID, "onLit");
                injector.stop();

                var childrenProps = getChildProperties(self.entityID, ['type']);
                for (var childEntityID in childrenProps) {
                    debug("Updating: ", childEntityID);
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

            Script.setTimeout(function() {
                debug("Setting fuse to inactive");
                active = false;
            }, 14000);
        },
        preload: function(entityID) {
            debug("Preload");
            this.entityID = entityID;
        },
    };
    return new Fuse();
});
