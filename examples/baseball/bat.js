(function() {
    Script.include("pitching.js");
    var pitchingMachine = null;
    this.startNearGrab = function() {
        if (!pitchingMachine) {
            getPitchingMachine();
            Script.update.connect(function(dt) { pitchingMachine.update(dt); });
        }
        print("Started near grab!");
        pitchingMachine.start();
        MyAvatar.shouldRenderLocally = false;
    };
    this.continueNearGrab = function() {
        MyAvatar.shouldRenderLocally = false;
    }
    this.releaseGrab = function() {
        print("Stopped near grab!");
        if (pitchingMachine) {
            pitchingMachine.stop();
        }
        MyAvatar.shouldRenderLocally = true;
    };
});
