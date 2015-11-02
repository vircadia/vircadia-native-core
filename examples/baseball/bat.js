(function() {
    Script.include("pitching.js");
    var pitchingMachine = getPitchingMachine();
    Script.update.connect(function(dt) { pitchingMachine.update(dt); });
    this.startNearGrab = function() {
        print("Started near grab!");
        pitchingMachine.start();
    };
    this.releaseGrab = function() {
        print("Stopped near grab!");
        if (pitchingMachine) {
            pitchingMachine.stop();
        }
    };
});
