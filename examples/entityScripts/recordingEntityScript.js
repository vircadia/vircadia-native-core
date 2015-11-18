//
//  recordingEntityScript.js
//  examples/entityScripts
//
//  Created by Alessandro Signa on 11/12/15.
//  Copyright 2015 High Fidelity, Inc.
//

//  All the avatars in the area when the master presses the button will start/stop recording.
//  

//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


(function () {

    HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";
    Script.include(HIFI_PUBLIC_BUCKET + "scripts/libraries/utils.js");

    var insideRecorderArea = false;
    var enteredInTime = false;
    var isAvatarRecording = false;
    var _this;

    function recordingEntity() {
        _this = this;
        return;
    }

    function update() {
        var isRecordingStarted = getEntityCustomData("recordingKey", _this.entityID, { isRecordingStarted: false }).isRecordingStarted;
        if (isRecordingStarted && !isAvatarRecording) {
            _this.startRecording();
        } else if ((!isRecordingStarted && isAvatarRecording) || (isAvatarRecording && !insideRecorderArea)) {
            _this.stopRecording();
        } else if (!isRecordingStarted && insideRecorderArea && !enteredInTime) {
            //if an avatar enters the zone while a recording is started he will be able to participate to the next group recording
            enteredInTime = true;
        }
    };

    recordingEntity.prototype = {

        preload: function (entityID) {
            print("RECORDING ENTITY PRELOAD");
            this.entityID = entityID;

            var entityProperties = Entities.getEntityProperties(_this.entityID);
            if (!entityProperties.ignoreForCollisions) {
                Entities.editEntity(_this.entityID, { ignoreForCollisions: true });
            }

            //print(JSON.stringify(entityProperties));
            var recordingKey = getEntityCustomData("recordingKey", _this.entityID, undefined);
            if (recordingKey === undefined) {
                setEntityCustomData("recordingKey", _this.entityID, { isRecordingStarted: false });
            }

            Script.update.connect(update);
        },
        enterEntity: function (entityID) {
            print("entering in the recording area");
            insideRecorderArea = true;
            var isRecordingStarted = getEntityCustomData("recordingKey", _this.entityID, { isRecordingStarted: false }).isRecordingStarted;
            if (!isRecordingStarted) {
                //i'm in the recording area in time (before the event starts)
                enteredInTime = true;
            }
        },
        leaveEntity: function (entityID) {
            print("leaving the recording area");
            insideRecorderArea = false;
            enteredInTime = false;
        },

        startRecording: function (entityID) {
            if (enteredInTime && !isAvatarRecording) {
                print("RECORDING STARTED");
                Recording.startRecording();
                isAvatarRecording = true;
            }
        },

        stopRecording: function (entityID) {
            if (isAvatarRecording) {
                print("RECORDING ENDED");
                Recording.stopRecording();
                Recording.loadLastRecording();
                isAvatarRecording = false;
                recordingFile = Window.save("Save recording to file", "./groupRecording", "Recordings (*.hfr)");
                if (!(recordingFile === "null" || recordingFile === null || recordingFile === "")) {
                    Recording.saveRecording(recordingFile);
                }
            }
        },
        unload: function (entityID) {
            print("RECORDING ENTITY UNLOAD");
            Script.update.disconnect(update);
        }
    }



    return new recordingEntity();
});