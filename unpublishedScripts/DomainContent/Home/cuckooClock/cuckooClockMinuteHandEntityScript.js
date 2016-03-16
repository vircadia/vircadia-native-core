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
            _this.userData = getEntityUserData(entityID);
            // print("ANIMATION!!! " + JSON.stringify(_this.animationURL));
            if (!_this.userData || !_this.userData.clockBody) {
                print("THIS CLOCK HAND IS NOT ATTACHED TO A CLOCK BODY!");
                return;
            }
            _this.clockBody = _this.userData.clockBody;
            Entities.editEntity(_this.clockBody, {animation: {running: true}});
            Script.update.connect(_this.update);
        },

        unload: function() {
            Script.update.disconnect(_this.update);
        },
        

        update: function() {
            _this.clockBodyAnimationProps = Entities.getEntityProperties(_this.clockBody, "animation").animation;
            if (!_this.clockBodyAnimationProps) {
                print("NO CLOCK BODY ANIMATION PROPS! RETURNING");
                return;
            }
            // print("current Frame " + _this.clockBodyAnimationProps.currentFrame);
        }


    };

    // entity scripts always need to return a newly constructed object of our type
    return new CuckooClockMinuteHand();
});