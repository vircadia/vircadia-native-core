//
//  Created by Dante Ruiz 2017/04/17
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var recording = false;
    var onRecordingScreen = false;
    var passedSaveDirectory = false;
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        text: "IRecord"
    });
    function onClick() {
        if (onRecordingScreen) {
            tablet.gotoHomeScreen();
            onRecordingScreen = false;
        } else {
            tablet.loadQMLSource("InputRecorder.qml");
            onRecordingScreen = true;
        }
    }

    function onScreenChanged(type, url) {
        onRecordingScreen = false;
        passedSaveDirectory = false;
    }

    button.clicked.connect(onClick);
    tablet.fromQml.connect(fromQml);
    tablet.screenChanged.connect(onScreenChanged);
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
        recording = true;
    }
    
    function stopRecording() {
        Controller.stopInputRecording();
        recording = false;
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

    function sendToQml(message) {
        tablet.sendToQml(message);
    }

    function update() {

        if (!passedSaveDirectory) {
            var directory = Controller.getInputRecorderSaveDirectory();
            sendToQml({method: "path", params: directory});
            passedSaveDirectory = true;
        }
        sendToQml({method: "update", params: recording});
    }

    Script.setInterval(update, 60);

    Script.scriptEnding.connect(function () {
        button.clicked.disconnect(onClick);
        if (tablet) {
            tablet.removeButton(button);
        }

        Controller.stopInputRecording();
    });

}());
