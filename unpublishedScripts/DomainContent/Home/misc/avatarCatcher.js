//
//
//  Created by The Content Team 4/10/216
//  Copyright 2016 High Fidelity, Inc.
//
//
//if someone happens to fall below the home, they'll get caught by this surface and teleported back to the living room
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var _this;

    var LIVING_ROOM_POSITION = {
        "x": 1102.35,
        "y": 460.122,
        "z": -78.924
    }

    var ROTATION = {
        "x": 0,
        "y": -0.447182,
        "z": 0,
        "w": 0.894443
    }

    var AvatarCatcher = function() {
        _this = this;
    }

    AvatarCatcher.prototype = {
        enterEntity: function() {
            this.teleportToLivingRoom();
        },
        teleportToLivingRoom: function() {
            MyAvatar.goToLocation(LIVING_ROOM_POSITION, true, ROTATION);
        }
    }
    return new AvatarCatcher();
});