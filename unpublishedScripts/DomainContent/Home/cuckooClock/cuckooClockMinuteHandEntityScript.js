(function() {
    Script.include('../utils.js');
    var _this;

    var d = new Date();
    var h = d.getHours();
    h = h % 12;

    CuckooClockMinuteHand = function() {
        _this = this;

    };

    CuckooClockMinuteHand.prototype = {


        preload: function(entityID) {
            _this.entityID = entityID; // this.animation.isRunning = true;
            var userData = getEntityUserData(entityID);
            var clockBody = userData.clockBody;
            // print("ANIMATION!!! " + JSON.stringify(_this.animationURL));
            Entities.editEntity(_this.entityID, {animation: {running: true}})

        },


    };

    // entity scripts always need to return a newly constructed object of our type
    return new CuckooClockMinuteHand();
});