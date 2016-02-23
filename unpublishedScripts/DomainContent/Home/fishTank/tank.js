(function() {

    function FishTank() {
        _this = this;
    }

    FishTank.prototype = {

        preload: function(entityID) {
            print("preload");
            this.entityID = entityID;
            //we only need to do this once since tank is static
            _this.currentProperties = Entities.getEntityProperties(entityID);
            Script.update.connect(this.update)
        },

        unload: function() {

        },

        update: function() {
            _this.updateWithoutSort();
        },

        updateWithSort: function() {
            var avatars = AvatarList.getAvatarIdentifiers();
            var distances = [];
            avatars.forEach(function(avatar) {
                var _avatar = AvatarList.getAvatar(avatar);
                var distanceFromTank = Vec3.distance(_this.currentProperties.position, _avatar.position);
                distances.push([distanceFromTank, avatar]);
            })
            var sortedDistances = distances.sort(function(a, b) {
                return a[0] - b[0]
            });
            //only the closest person should run the update loop
            if (sortedDistances[0][1] !== MyAvatar.sessionUUID) {
                return;
            } else {

            }
        },

        updateWithoutSort: function() {
            var avatars = AvatarList.getAvatarIdentifiers();
            var minDistance = 100000;
            var closest;
            print('AVATARS' + avatars)
            avatars.forEach(function(avatar) {
                    var _avatar = AvatarList.getAvatar(avatar);
                    var distanceFromTank = Vec3.distance(_this.currentProperties.position, _avatar.position);
                    if (distanceFromTank < minDistance) {
                        minDistance = distanceFromTank;
                        closest = avatar;
                    }
                })
                //only the closest person should run the update loop
               // print('closest:', closest)
               // print('me', MyAvatar.sessionUUID)
            if (closest !== MyAvatar.sessionUUID) {
                //print('not me')
                return;
            } else {
                print('i should run this entity because im closest:' + _this.entityID)
            }
        }


    };

    return new FishTank();
});