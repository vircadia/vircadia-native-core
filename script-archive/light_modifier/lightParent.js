//
//  lightParent.js
//
//  Created by James Pollack @imgntn on 12/15/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Entity script that tells the light parent to update the selection tool when we move it.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

            (function() {

                function LightParent() {
                    return this;
                }

                LightParent.prototype = {
                    preload: function(entityID) {
                        this.entityID = entityID;
                        var entityProperties = Entities.getEntityProperties(this.entityID, "userData");
                        this.initialProperties = entityProperties
                        this.userData = JSON.parse(entityProperties.userData);
                    },
                    startNearGrab: function() {},
                    startDistantGrab: function() {

                    },
                    continueNearGrab: function() {
                        this.continueDistantGrab();
                    },
                    continueDistantGrab: function() {
                        Messages.sendMessage('entityToolUpdates', 'callUpdate');
                    },

                };

                return new LightParent();
            });