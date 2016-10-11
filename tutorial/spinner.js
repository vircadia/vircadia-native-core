//
//  spinner.js
//
//  Created by Ryan Huffman on 9/1/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var DEBUG = false;
    function debug() {
        if (DEBUG) {
            print.apply(self, arguments);
        }
    }

    var spinnerSound = SoundCache.getSound("atp:/tutorial_sounds/Pinwheel.L.wav");
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
            debug("LIT SPINNER", this.entityID);
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

            var childrenProps = getChildProperties(this.entityID, ['type']);
            for (var childEntityID in childrenProps) {
                var props = childrenProps[childEntityID];
                if (props.type == "ParticleEffect") {
                    Entities.editEntity(childEntityID, {
                        emitRate: 35,
                    });
                }
            }
            Messages.sendLocalMessage("Tutorial-Spinner", "wasLit");

            var self = this;
            Script.setTimeout(function() {
                debug("BLOW UP");
                injector.stop();

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
