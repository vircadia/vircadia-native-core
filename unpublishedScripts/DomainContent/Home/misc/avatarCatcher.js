(function() {
    var _this;

    var LIVING_ROOM_POSITION = {
        "x": 1101.7781982421875,
        "y": 460.5820007324219,
        "z": -77.89060974121094
    }

    var ROTATION = {
        "x": -0.04497870057821274,
        "y": -0.4520675241947174,
        "z": -0.02283225581049919,
        "w": 0.890556275844574
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