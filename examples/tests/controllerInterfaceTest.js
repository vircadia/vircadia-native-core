ControllerTest = function() {
    var standard = Controller.Standard;
    var actions = Controller.Actions;
    var xbox = Controller.Hardware.GamePad;
    this.mappingEnabled = false;
    this.mapping = Controller.newMapping();
    this.mapping.from(standard.LX).when([standard.LB, standard.RB]).to(actions.Yaw);
    this.mapping.from(standard.RX).to(actions.StepYaw);
    this.mapping.from(standard.RY).invert().to(actions.Pitch);
    this.mapping.from(standard.RY).invert().to(actions.Pitch);


    var testMakeAxis = false;
    if (testMakeAxis) {
        this.mapping.makeAxis(standard.LB, standard.RB).pulse(0.25).scale(40.0).to(actions.StepYaw);
    } 
    
    var testStepYaw = false;
    if (!testMakeAxis && testStepYaw){
        this.mapping.from(standard.LB).pulse(0.10).invert().scale(40.0).to(actions.StepYaw);
        this.mapping.from(standard.RB).pulse(0.10).scale(15.0).to(actions.StepYaw);
    }
    
    var testFunctionSource = false;
    if (testFunctionSource) {
        this.mapping.from(function(){
            return Math.sin(Date.now() / 250); 
        }).to(actions.Yaw);
    }
    
    var testFunctionDest = true;
    if (testFunctionDest) {
        this.mapping.from(standard.DU).pulse(1.0).to(function(value){
            if (value != 0.0) {
                print(value);
            }
        });

    }

    this.mapping.enable();
    this.mappingEnabled = true;

    var dumpInputs = false;
    if (dumpInputs) {
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
    }

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