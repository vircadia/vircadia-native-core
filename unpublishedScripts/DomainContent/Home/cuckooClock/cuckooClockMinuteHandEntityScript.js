(function() {
    Script.include('../utils.js');
    Script.include('../../../../examples/libraries/virtualBaton.js');
    var _this;

    var d = new Date();
    var h = d.getHours();
    h = h % 12;

    // Only one person should similate the clock at a time- so we pass a virtual baton
    var baton;
    var iOwn = false;
    var connected = false;

    CuckooClockMinuteHand = function() {
        _this = this;
        _this.TIME_CHECK_REFRACTORY_PERIOD = 5000;
        _this.checkTime = true;

    };


    function startUpdate() {
        //when the baton is claimed;
        //   print('trying to claim the object' + _entityID)
        iOwn = true;
        connected = true;
        Script.update.connect(_this.update);
    }

    function stopUpdateAndReclaim() {
        //when the baton is released;
        // print('i released the object ' + _entityID)
        iOwn = false;
        if (connected === true) {
            connected = false;
            Script.update.disconnect(_this.update);
        }
        //hook up callbacks to the baton
        baton.claim(startUpdate, stopUpdateAndReclaim);
        stopUpdateAndReclaim();
    }

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
            // One winner for each entity
            baton = virtualBaton({
                batonName: "io.highfidelity.cuckooClock:" + _this.entityID
            })

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