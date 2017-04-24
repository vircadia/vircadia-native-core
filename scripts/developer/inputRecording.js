//
//  Created by Dante Ruiz 2017/04/17
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var onRecordingScreen = false;
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        text: "IRecord"
    });
    function onClick() {
        if (onRecordingScreen) {
            tablet.gotoHomeScreen();
        } else {
            tablet.loadQMLSource("InputRecorder.qml");
        }
    }

    button.clicked.connect(onClick);
    tablet.fromQml.connect(fromQml);
    function fromQml(message) {
        switch (message.method) {
        case "Start":
            startRecording();
            break;
        case "Stop":
            stopRecording();
            break;
        case "Save":
            saveRecording();
            break;
        case "Load":
            loadRecording(message.params.file);
            break;
        case "playback":
            startPlayback();
            break;
        }
        
    }

    function startRecording() {
        Controller.startInputRecording();
    }
    
    function stopRecording() {
        Controller.stopInputRecording();
    }
    
    function saveRecording() {
        Controller.saveInputRecording();
    }
    
    function loadRecording(file) {
        Controller.loadInputRecording(file);
    }

    function startPlayback() {
        Controller.startInputPlayback();
    }

    Script.scriptEnding.connect(function () {
        button.clicked.disconnect(onClick);
        if (tablet) {
            tablet.removeButton(button);
        }
    });

}());
