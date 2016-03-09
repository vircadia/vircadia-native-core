(function() {
    var _this;
    Script.include("../libraries/utils.js");
    var NULL_UUID = "{00000000-0000-0000-0000-000000000000}";
    var ZERO_VEC = {x: 0, y: 0, z: 0};
    VRVJVisualEntity = function() {
        _this = this;
        _this.SOUND_LOOP_NAME = "VRVJ-Sound-Cartridge";
        _this.SOUND_CARTRIDGE_SEARCH_RANGE = 0.1;

    };

    VRVJVisualEntity.prototype = {

        releaseGrab: function() {
            print("RELEASE GRAB")
                // search for nearby sound loop entities and if found, add it as a parent
            Script.setTimeout(function() {
                _this.searchForNearbySoundLoops();
            }, 100);
        },

        searchForNearbySoundLoops: function() {
            _this.position = Entities.getEntityProperties(_this.entityID, "position").position;
            var entities = Entities.findEntities(_this.position, _this.SOUND_CARTRIDGE_SEARCH_RANGE);
            for (var i = 0; i < entities.length; i++) {
                var entity = entities[i];
                var props = Entities.getEntityProperties(entity, ["name", "color"]);
                if (props.name.indexOf(_this.SOUND_LOOP_NAME) !== -1) {
                    // Need to set a timeout to wait for grab script to stop messing with entity
                    Entities.editEntity(_this.entityID, {
                        parentID: entity,
                        dynamic: false
                    });
                    Script.setTimeout(function() {
                        Entities.editEntity(_this.entityID, {dynamic: true, velocity: ZERO_VEC, color: props.color});
                    }, 100);
                    return;
                }

            }
            Entities.editEntity(_this.entityID, {
                parentID: NULL_UUID,
                color: _this.originalColor
            });
        },

        preload: function(entityID) {
            print("YAAAA")
            _this.entityID = entityID;
            _this.originalColor = Entities.getEntityProperties(_this.entityID, "color").color;

        },
    };

    // entity scripts always need to return a newly constructed object of our type
    return new VRVJVisualEntity();
});