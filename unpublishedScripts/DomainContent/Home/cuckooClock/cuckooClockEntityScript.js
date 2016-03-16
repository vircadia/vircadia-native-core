(function() {

    var _this;

    var d = new Date();
    var h = d.getHours();
    h = h % 12;

    CuckooClock = function() {
        _this = this;

    };

    CuckooClock.prototype = {


        preload: function(entityID) {
            _this.entityID = entityID; // this.animation.isRunning = true;
            // print("ANIMATION!!! " + JSON.stringify(_this.animationURL));
            Entities.editEntity(_this.entityID, {animation: {running: true}})

        },


    };

    // entity scripts always need to return a newly constructed object of our type
    return new CuckooClock();
});