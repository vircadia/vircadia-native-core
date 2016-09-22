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

    var DEBUG = false;
    function debug() {
        if (DEBUG) {
            print.apply(self, arguments);
        }
    }

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
            debug("LIT", this.entityID);
            var anim = Entities.getEntityProperties(this.entityID, ['animation']).animation;

            if (anim.currentFrame < 140) {
                return;
            }
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
                debug("BLOW UP");
                var spinnerID = Utils.findEntity({ name: "tutorial/equip/spinner" }, 20); 
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
