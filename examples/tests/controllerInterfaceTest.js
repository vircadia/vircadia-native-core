ControllerTest = function() {

    print("Actions");
    for (var prop in Controller.Actions) {
        print("\t" + prop);
    }
    print("Standard");
    for (var prop in Controller.Standard) {
        print("\t" + prop);
    }
    print("Hardware");
    for (var prop in Controller.Hardware) {
        print("\t" + prop);
        for (var prop2 in Controller.Hardware[prop]) {
            print("\t\t" + prop2);
        }
    }
    print("Done");

    var that = this;
    Script.scriptEnding.connect(function() {
        that.onCleanup();
    });
}

ControllerTest.prototype.onCleanup = function() {
}


new ControllerTest();