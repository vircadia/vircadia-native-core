(function() {
    Script.include('../utils.js');
    var _this;

    var d = new Date();
    var h = d.getHours();
    h = h % 12;

    CuckooClockMinuteHand = function() {
        _this = this;
        _this.TIME_CHECK_REFRACTORY_PERIOD = 1000;
        _this.checkTime = true;

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

            if (_this.checkTime === false) {
                print("We are in our refractory period. Please wait.");
                return;
            }
            var date = new Date();
            // Check to see if we are at the top of the hour
            var seconds = date.getSeconds();
            var minutes = date.getMinutes();

            if (seconds === 0 && minutes === 0) {
                _this.popCuckooOut();
            }

        },

        popCuckooOut: function() {
            // We are at the top of the hour!
            Entities.editEntity(_this.clockBody, {
                animation: {
                    running: true
                }
            });
            _this.checkTime = false;
            Script.setTimeout(function() {
                _this.checkTime = true;
            }, _this.TIME_CHECK_REFRACTORY_PERIOD);
        }


    };

    // entity scripts always need to return a newly constructed object of our type
    return new CuckooClockMinuteHand();
});