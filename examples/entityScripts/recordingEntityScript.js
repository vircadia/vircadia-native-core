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
    var MASTER_TO_CLIENTS_CHANNEL = "startStopChannel";
    var CLIENTS_TO_MASTER_CHANNEL = "resultsChannel";
    var START_MESSAGE = "recordingStarted";
    var STOP_MESSAGE = "recordingEnded";
    var PARTICIPATING_MESSAGE = "participatingToRecording";

    function recordingEntity() {
        _this = this;
        return;
    };

    function receivingMessage(channel, message, senderID) {
        if (channel === MASTER_TO_CLIENTS_CHANNEL) {
            print("CLIENT received message:" + message);
            if (message === START_MESSAGE) {
                _this.startRecording();
            } else if (message === STOP_MESSAGE) {
                _this.stopRecording();
            }
        }
    };

    function getClipUrl(url) {
        Messages.sendMessage(CLIENTS_TO_MASTER_CHANNEL, url);    //send back the url to the master
        print("clip uploaded and url sent to master");
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
            Messages.subscribe(MASTER_TO_CLIENTS_CHANNEL);
        },

        leaveEntity: function (entityID) {
            print("leaving the recording area");
            _this.stopRecording();
            Messages.unsubscribe(MASTER_TO_CLIENTS_CHANNEL);
        },

        startRecording: function (entityID) {
            if (!isAvatarRecording) {
                print("RECORDING STARTED");
                Messages.sendMessage(CLIENTS_TO_MASTER_CHANNEL, PARTICIPATING_MESSAGE);  //tell to master that I'm participating
                Recording.startRecording();
                isAvatarRecording = true;
            }
        },

        stopRecording: function (entityID) {
            if (isAvatarRecording) {
                print("RECORDING ENDED");
                Recording.stopRecording();
                isAvatarRecording = false;

                var recordingFile = Window.save("Save recording to file", "./groupRecording", "Recordings (*.hfr)");
                if (!(recordingFile === "null" || recordingFile === null || recordingFile === "")) {
                    Recording.saveRecording(recordingFile);     //save the clip locally
                }
                Recording.saveRecordingToAsset(getClipUrl);     //save the clip to the asset and link a callback to get its url
            }
        },

        unload: function (entityID) {
            print("RECORDING ENTITY UNLOAD");
            _this.stopRecording();
            Messages.unsubscribe(MASTER_TO_CLIENTS_CHANNEL);
            Messages.messageReceived.disconnect(receivingMessage);
        }
    }

    return new recordingEntity();
});