//  junkyardResetEntityScript.js
//  
//  Script Type: Entity
//  Created by Eric Levin on 1/20/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  This entity script resets the junkyard when triggered
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    Script.include("../libraries/utils.js");
    var _this;
    var importURL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/arfs/junkyard.json";
    var JunkyardResetter = function() {
        _this = this;
    };

    JunkyardResetter.prototype = {

        clickReleaseOnEntity: function(entityId, mouseEvent) {
            if (!mouseEvent.isLeftButton) {
                return;
            }
            this.reset();

        },

        reset: function() {
            // Delete everything and re-import the junkyard arf
            var e = Entities.findEntities(MyAvatar.position, 1000);
            for (i = 0; i < e.length; i++) {
                // Don't delete our reset entity
                if (JSON.stringify(this.entityID) !== JSON.stringify(e[i])) {
                  Entities.deleteEntity(e[i]);  
                }
            }
            this.importArf();
        },

        importArf: function() {
            Clipboard.importEntities(importURL);
            Clipboard.pasteEntities({
                x: 0,
                y: 0,
                z: 0
            });
        },

        preload: function(entityID) {
            this.entityID = entityID;
        },
    };
    return new JunkyardResetter();
});