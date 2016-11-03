//
//  visiblePanel.js
//
//  Created by James Pollack @imgntn on 12/15/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Entity script that disables picking on this panel.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {

    function VisiblePanel() {
        return this;
    }

    VisiblePanel.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;

            var data = {
                action: 'add',
                id: this.entityID
            };
            Messages.sendMessage('Hifi-Hand-RayPick-Blacklist', JSON.stringify(data))
        },
        unload: function() {
            var data = {
                action: 'remove',
                id: this.entityID
            };
            Messages.sendMessage('Hifi-Hand-RayPick-Blacklist', JSON.stringify(data))
        }

    };

    return new VisiblePanel();
});