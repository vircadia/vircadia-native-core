// When user holds down trigger on model for enough time, the model with do a cool animation and swap out with the low or high version of it



(function() {

    var _this;
    ModelSwaper = function() {
        _this = this;
    };

    ModelSwaper.prototype = {

        startFarTrigger: function() {
            print("START TRIGGER")

            //make self invisible and make the model's counterpart visible!
            var dimensions = Entities.getEntityProperties(this.entityID, "dimensions").dimensions;
            Entities.editEntity(this.entityID, {
                visible: false,
                dimensions: Vec3.multiply(dimensions, 0.5)
            });
            dimensions = Entities.getEntityProperties(this.modelCounterpartId, "dimensions").dimensions;
            Entities.editEntity(this.modelCounterpartId, {
                visible: true,
                dimensions: Vec3.multiply(dimensions, 2)
            });

        },

        preload: function(entityID) {
            this.entityID = entityID;
            var props = Entities.getEntityProperties(this.entityID, ["userData"]);
            this.modelCounterpartId = JSON.parse(props.userData).modelCounterpart.modelCounterpartId;
        }

    };

    // entity scripts always need to return a newly constructed object of our type
    return new ModelSwaper();
});