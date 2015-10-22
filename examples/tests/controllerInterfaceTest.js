ControllerTest = function() {
    var standard = Controller.Standard;
    var actions = Controller.Actions;
    this.mappingEnabled = false;
    this.mapping = Controller.newMapping();
    this.mapping.from(standard.RX).to(actions.StepYaw);
    this.mapping.enable();
    this.mappingEnabled = true;


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
    if (this.mappingEnabled) {
        this.mapping.disable();
    }
}


new ControllerTest();