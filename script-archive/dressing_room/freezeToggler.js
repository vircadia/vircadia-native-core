//
//  dopppelgangerEntity.js
//
//  Created by James B. Pollack @imgntn on 1/6/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  for freezing / unfreezing the doppelganger
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var COUNTDOWN_LENGTH = 0;
    var _this;

    Dopppelganger = function() {

        _this = this;
    };

    Dopppelganger.prototype = {
        isFrozen: false,
        startNearTrigger: function() {
            print('DOPPELGANGER NEAR TRIGGER')
        },
        startFarTrigger: function() {
            print('DOPPELGANGER FAR TRIGGER')
            if (this.isFrozen === false) {
                this.freeze();
            } else {
                this.unfreeze();
            }

        },
        clickReleaseOnEntity: function(entityID, mouseEvent) {
            print('DOPPELGANGER CLICK')
            if (!mouseEvent.isLeftButton) {
                return;
            }
            if (this.isFrozen === false) {
                this.freeze();
            } else {
                this.unfreeze();
            }

        },
        freeze: function() {
            print('FREEZE YO')
            this.isFrozen = true;
            var data = {
                action: 'freeze'
            }

            Script.setTimeout(function() {
                Messages.sendMessage('Hifi-Doppelganger-Freeze', JSON.stringify(data));
            }, COUNTDOWN_LENGTH)

        },
        unfreeze: function() {
            this.isFrozen = false;
            var data = {
                action: 'unfreeze'
            }
            Messages.sendMessage('Hifi-Doppelganger-Freeze', JSON.stringify(data));
        },

        preload: function(entityID) {
            this.entityID = entityID;
            this.initialProperties = Entities.getEntityProperties(this.entityID);
            this.userData = JSON.parse(this.initialProperties.userData);
        },
    };

    return new Dopppelganger();
})