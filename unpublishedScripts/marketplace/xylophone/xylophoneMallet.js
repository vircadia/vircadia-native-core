//
//  xylophoneMallet.js
//
//  Created by Johnathan Franck on 07/30/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

(function() {
    function XylophoneMallet() {
    }

    XylophoneMallet.prototype = {
        startEquip: function(entityID, args) {
            var LEFT_HAND = 0;
            var RIGHT_HAND = 1;
            var userData = JSON.parse(Entities.getEntityProperties(entityID, 'userData').userData);
            userData.hand = args[0] === "left" ? LEFT_HAND : RIGHT_HAND;
            Entities.editEntity(entityID, {userData: JSON.stringify(userData)});
        }
    };

    return new XylophoneMallet();
});
