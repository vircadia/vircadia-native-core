"use strict";

var origin = {x: 512, y: 512, z: 512};
var millisecondsToWaitBeforeStarting = 2 * 1000; // To give the various servers a chance to start.
var millisecondsToWaitBeforeEnding = 30 * 1000; 

Avatar.skeletonModelURL = "https://hifi-public.s3.amazonaws.com/marketplace/contents/dd03b8e3-52fb-4ab3-9ac9-3b17e00cd85d/98baa90b3b66803c5d7bd4537fca6993.fst"; //lovejoy
Avatar.displayName = "AC Avatar";
Agent.isAvatar = true;

Script.setTimeout(function () {
    Avatar.position = origin;
    Recording.loadRecording("d:/hifi.rec");
    Recording.setPlayerLoop(true);
    Recording.startPlaying();
}, millisecondsToWaitBeforeStarting);


Script.setTimeout(function () {
    print("Stopping script");
    Agent.isAvatar = false;
    Recording.stopPlaying();
    Script.stop();
}, millisecondsToWaitBeforeEnding);