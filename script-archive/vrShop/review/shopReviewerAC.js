// shopReviewerAC.js
//

//  Created by Alessandro Signa and Edgar Pironti on 01/13/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


var command = null;
var clip_url = null;

var REVIEW_CHANNEL = "reviewChannel";
var playFromCurrentLocation = true;
var useDisplayName = true;
var useAttachments = true;
var useAvatarModel = true;

var totalTime = 0;
var subscribed = false;
var WAIT_FOR_AUDIO_MIXER = 1;

var PLAY = "Play";
var SHOW = "Show";
var HIDE = "Hide";

function getAction(channel, message, senderID) {
    if(subscribed) {
        print("I'm the agent and I received this: " + message);

        if (Recording.isPlaying()) {
            Recording.stopPlaying();
        }

        m = JSON.parse(message);

        command = m.command;
        clip_url = m.clip_url;

        switch(command) {
            case PLAY:
                print("Play");
                if (!Recording.isPlaying()) {
                    Recording.setPlayerTime(0.0);
                    Recording.startPlaying();
                }
                break;

            case SHOW:
                print("Show");
                Recording.loadRecording(clip_url, function(success) {
                    if (success) {
                        Agent.isAvatar = true;
                        Recording.setPlayerTime(0.0);
                        Recording.startPlaying();
                        Recording.stopPlaying();
                    }
                });

                break;

            case HIDE:
                print("Hide");
                Agent.isAvatar = false;
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
            Messages.subscribe(REVIEW_CHANNEL);
            subscribed = true;
            Recording.setPlayFromCurrentLocation(playFromCurrentLocation);
            Recording.setPlayerUseDisplayName(useDisplayName);
            Recording.setPlayerUseAttachments(useAttachments);
            Recording.setPlayerUseHeadModel(false);
            Recording.setPlayerUseSkeletonModel(useAvatarModel);
        }
    }

}

Messages.messageReceived.connect(function (channel, message, senderID) {
    if (channel == REVIEW_CHANNEL) {
        getAction(channel, message, senderID);
    }
});

Script.update.connect(update);
