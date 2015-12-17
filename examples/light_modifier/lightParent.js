            //
            //  slider.js
            //
            //  Created by James Pollack @imgntn on 12/15/2015
            //  Copyright 2015 High Fidelity, Inc.
            //
            //  Entity script that sends a scaled value to a light based on its distance from the start of its constraint axis.
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
                        print('LIGHT PARENT SCRIPT GO')
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
                        print('distant grab, should send message!')
                        Messages.sendMessage('entityToolUpdates', 'callUpdate');
                    },

                };

                return new LightParent();
            });