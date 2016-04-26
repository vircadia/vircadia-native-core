(function() {
    
    var _this;

    MyEntity = function() {
        _this = this;

    };

    MyEntity.prototype = {

        preload: function(entityID) {
            this.entityID = entityID;
            var randNum = Math.random().toFixed(3);
            print("PRELOAD ENTITY SCRIPT!!!", randNum)

        },

    };

    // entity scripts always need to return a newly constructed object of our type
    return new MyEntity();
});