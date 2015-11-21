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

    var _this;
    var isAvatarRecording = false;
    var channel = "groupRecordingChannel";
    var startMessage = "RECONDING STARTED";
    var stopMessage = "RECONDING ENDED";

    function recordingEntity() {
        _this = this;
        return;
    }

    function receivingMessage(channel, message, senderID) {
        print("message received on channel:" + channel + ", message:" + message + ", senderID:" + senderID);
        if(message === startMessage) {
            _this.startRecording();
        } else if(message === stopMessage) {
            _this.stopRecording();
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

            Messages.messageReceived.connect(receivingMessage);
        },

        enterEntity: function (entityID) {
            print("entering in the recording area");
            Messages.subscribe(channel);
            
        },

        leaveEntity: function (entityID) {
            print("leaving the recording area");
            _this.stopRecording();
            Messages.unsubscribe(channel);
        },

        startRecording: function (entityID) {
            if (!isAvatarRecording) {
                print("RECORDING STARTED");
                Recording.startRecording();
                isAvatarRecording = true;
            }
        },

        stopRecording: function (entityID) {
            if (isAvatarRecording) {
                print("RECORDING ENDED");
                Recording.stopRecording();
                isAvatarRecording = false;
                recordingFile = Window.save("Save recording to file", "./groupRecording", "Recordings (*.hfr)");
                if (!(recordingFile === "null" || recordingFile === null || recordingFile === "")) {
                    Recording.saveRecording(recordingFile);
                }
            }
        },

        unload: function (entityID) {
            print("RECORDING ENTITY UNLOAD");
            _this.stopRecording();
            Messages.unsubscribe(channel);
            Messages.messageReceived.disconnect(receivingMessage);
        }
    }

    return new recordingEntity();
});