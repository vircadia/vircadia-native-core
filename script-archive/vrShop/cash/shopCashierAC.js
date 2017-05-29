// shopCashierAC.js
//

//  Created by Alessandro Signa and Edgar Pironti on 01/13/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

// Set the following variable to the values needed
var clip_url = "atp:6865d9c89472d58b18929aff0ac779026bc129190f7536f71d3835f7e2629c93.hfr"; // This url is working in VRshop


var PLAYBACK_CHANNEL = "playbackChannel";
var playFromCurrentLocation = false;
var useDisplayName = true;
var useAttachments = true;
var useAvatarModel = true;

var totalTime = 0;
var subscribed = false;
var WAIT_FOR_AUDIO_MIXER = 1;

var PLAY = "Play";

function getAction(channel, message, senderID) {
    if(subscribed) {
        print("I'm the agent and I received this: " + message);

        switch(message) {
            case PLAY:
                print("Play");
                if (!Recording.isPlaying()) {
                    Recording.setPlayerTime(0.0);
                    Recording.startPlaying();
                }
                break;

            default:
                print("Unknown action: " + action);
                break;
        }
    }
}


function update(deltaTime) {

    totalTime += deltaTime;

    if (totalTime > WAIT_FOR_AUDIO_MIXER) {
        if (!subscribed) {
            Messages.subscribe(PLAYBACK_CHANNEL);
            subscribed = true;
            Recording.loadRecording(clip_url, function(success) {
                if (success) {
                    Recording.setPlayFromCurrentLocation(playFromCurrentLocation);
                    Recording.setPlayerUseDisplayName(useDisplayName);
                    Recording.setPlayerUseAttachments(useAttachments);
                    Recording.setPlayerUseHeadModel(false);
                    Recording.setPlayerUseSkeletonModel(useAvatarModel);
                    Agent.isAvatar = true;
                } else {
                    print("Failed to load recording from " + clip_url);
                }
            });
        }
    }

}

Messages.messageReceived.connect(function (channel, message, senderID) {
    if (channel == PLAYBACK_CHANNEL) {
        getAction(channel, message, senderID);
    }
});

Script.update.connect(update);
