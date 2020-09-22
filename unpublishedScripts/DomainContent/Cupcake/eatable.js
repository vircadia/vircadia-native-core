//
// eatable.js
//
//  Created by Alan-Michael Moody on 7/24/2017
//  Copyright 2017 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {
    var NOMNOM_SOUND = SoundCache.getSound(Script.getExternalPath(Script.ExternalPaths.HF_Content, "/DomainContent/production/audio/vegcrunch.wav"));

    var _entityID;

    this.preload = function (entityID) {
        _entityID = entityID;
    };

    this.collisionWithEntity = function(entityUuid, collisionEntityID, collisionInfo) {
        var entity = Entities.getEntityProperties(collisionEntityID, ['userData', 'name']);
        var isClone = (entity.name.substring(1).split('-')[0] === 'clone');
        var isEatable = (JSON.parse(entity.userData).eatable);

        if (isEatable && isClone) {
            Audio.playSound(NOMNOM_SOUND, {
                position: Entities.getEntityProperties(_entityID, ['position']).position,
                volume: 0.2,
                localOnly: false
            });

            Entities.deleteEntity(collisionEntityID);
        }
    };
});