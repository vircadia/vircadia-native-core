
(function() {
    var _this;
    MyEntity = function() {
        _this = this;
     
    };

    MyEntity.prototype = {


        preload: function(entityID) {
            this.entityID = entityID;
            print("EBL PRELOAD ENTITY SCRIPT!!!")

        },

   
    };

    // entity scripts always need to return a newly constructed object of our type
    return new MyEntity();
});