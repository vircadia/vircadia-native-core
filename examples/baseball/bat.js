(function() {
    Script.include("file:///c:%5CUsers%5CRyan%5Cdev%5Chifi%5Cexamples%5Cbaseball%5Cpitching.js");
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
