//  halfDuplex.js
//
//  Created by Philip Rosedale on April 21, 2017
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  This client script reduces echo by muting the microphone when others nearby are speaking, 
//  allowing the use of live microphones and speakers.
//  NOTE:  This will only work where there are no injectors playing sound effects, 
//  since those will not be muted and will cause feedback.  
// 
//  IMPORTANT:  If you use multiple source microphones, you must give the microphone avatars 
//  the display name 'Microphone' (or change the string below), to prevent feedback.  This script
//  will mute the other so-named avatars, so that you can have speakers connected to those same 
//  avatars and in the same room.  
//

var averageLoudness = 0.0; 
var AVERAGING_TIME = 0.9;
var LOUDNESS_THRESHOLD = 100;
var HYSTERESIS_GAP = 1.41;          //  3dB gap
var MICROPHONE_DISPLAY_NAME = "Microphone";

var debug = false; 
var isMuted = false;  

Script.update.connect(function () {
    //  
    //  Check for other people's audio levels, mute if anyone is talking.  
    //
    var othersLoudness = 0;
    var avatars = AvatarList.getAvatarIdentifiers();
    avatars.forEach(function (id) {
        var avatar = AvatarList.getAvatar(id);
        if (MyAvatar.sessionUUID !== avatar.sessionUUID) {
            if (avatar.displayName.indexOf(MICROPHONE_DISPLAY_NAME) !== 0) {
                othersLoudness += Math.round(avatar.audioLoudness); 
            }
            //  Mute other microphone avatars to not feedback with muti-source environment
            if (avatar.displayName.indexOf(MICROPHONE_DISPLAY_NAME) === 0) {
                if (!Users.getPersonalMuteStatus(avatar.sessionUUID)) {
                    Users.personalMute(avatar.sessionUUID, true);
                }
            }
        }
    });

    averageLoudness = AVERAGING_TIME * averageLoudness + (1.0 - AVERAGING_TIME) * othersLoudness;

    if (!isMuted && (averageLoudness > LOUDNESS_THRESHOLD * HYSTERESIS_GAP)) {
        if (debug) { 
            print("Muted!"); 
        }
        isMuted = true;
        Audio.muted = true;
    } else if (isMuted && (averageLoudness < LOUDNESS_THRESHOLD)) {
        if (debug) { 
            print("UnMuted!"); 
        }
        isMuted = false;
        Audio.muted = false;
    }
});

