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