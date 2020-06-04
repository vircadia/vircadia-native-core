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
    var RECORDING_ICON_URL = "http://cdn.highfidelity.com/alan/production/icons/ICO_rec-active.svg";
    var NOT_RECORDING_ICON_URL = "http://cdn.highfidelity.com/alan/production/icons/ICO_rec-inactive.svg";
    var ICON_WIDTH = 60;
    var ICON_HEIGHT = 60;
    var overlay = null;


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
            if (!entityProperties.collisionless) {
                Entities.editEntity(_this.entityID, { collisionless: true });
            }

            Messages.messageReceived.connect(receivingMessage);
        },

        enterEntity: function (entityID) {
            print("entering in the recording area");
            Messages.subscribe(MASTER_TO_CLIENTS_CHANNEL);
            overlay = Overlays.addOverlay("image", {
                imageURL: NOT_RECORDING_ICON_URL,
                width: ICON_HEIGHT,
                height: ICON_WIDTH,
                x: 275,
                y: 0,
                visible: true
            });
        },

        leaveEntity: function (entityID) {
            print("leaving the recording area");
            _this.stopRecording();
            Messages.unsubscribe(MASTER_TO_CLIENTS_CHANNEL);
            Overlays.deleteOverlay(overlay);
            overlay = null;
        },

        startRecording: function () {
            if (!isAvatarRecording) {
                print("RECORDING STARTED");
                Messages.sendMessage(CLIENTS_TO_MASTER_CHANNEL, PARTICIPATING_MESSAGE);  //tell to master that I'm participating
                Recording.startRecording();
                isAvatarRecording = true;
                Overlays.editOverlay(overlay, {imageURL: RECORDING_ICON_URL});
            }
        },

        stopRecording: function () {
            if (isAvatarRecording) {
                print("RECORDING ENDED");
                Recording.stopRecording();
                isAvatarRecording = false;
                Recording.saveRecordingToAsset(getClipUrl);     //save the clip to the asset and link a callback to get its url
                Overlays.editOverlay(overlay, {imageURL: NOT_RECORDING_ICON_URL});
            }
        },

        unload: function (entityID) {
            print("RECORDING ENTITY UNLOAD");
            _this.stopRecording();
            Messages.unsubscribe(MASTER_TO_CLIENTS_CHANNEL);
            Messages.messageReceived.disconnect(receivingMessage);
            if (overlay !== null) {
                Overlays.deleteOverlay(overlay);
                overlay = null;
            }
        }
    }

    return new recordingEntity();
});
