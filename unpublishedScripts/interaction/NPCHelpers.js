//
//  NPCHelpers.js
//  scripts/interaction
//
//  Created by Trevor Berninger on 3/20/17.
//  Copyright 2017 High Fidelity Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var audioInjector = false;
var blocked = false;
var playingResponseAnim = false;
var storyURL = "";
var _qid = "start";

print("TESTTEST");

function strContains(str, sub) {
    return str.search(sub) != -1;
}

function callbackOnCondition(conditionFunc, ms, callback, count) {
    var thisCount = 0;
    if (typeof count !== 'undefined') {
        thisCount = count;
    }
    if (conditionFunc()) {
        callback();
    } else if (thisCount < 10) {
        Script.setTimeout(function() {
            callbackOnCondition(conditionFunc, ms, callback, thisCount + 1);
        }, ms);
    } else {
        print("callbackOnCondition timeout");
    }
}

function playAnim(animURL, looping, onFinished) {
    print("got anim: " + animURL);
    print("looping: " + looping);
    // Start caching the animation if not already cached.
    AnimationCache.getAnimation(animURL);

    // Tell the avatar to animate so that we can tell if the animation is ready without crashing
    Avatar.startAnimation(animURL, 30, 1, false, false, 0, 1);

    // Continually check if the animation is ready
    callbackOnCondition(function(){
        var details = Avatar.getAnimationDetails();
        // if we are running the request animation and are past the first frame, the anim is loaded properly
        print("running: " + details.running);
        print("url and animURL: " + details.url.trim().replace(/ /g, "%20") + " | " + animURL.trim().replace(/ /g, "%20"));
        print("currentFrame: " + details.currentFrame);
        return details.running && details.url.trim().replace(/ /g, "%20") == animURL.trim().replace(/ /g, "%20") && details.currentFrame > 0;
    }, 250, function(){
        var timeOfAnim = ((AnimationCache.getAnimation(animURL).frames.length / 30) * 1000) + 100; // frames to miliseconds plus a small buffer
        print("animation loaded. length: " + timeOfAnim);
        // Start the animation again but this time with frame information
        Avatar.startAnimation(animURL, 30, 1, looping, true, 0, AnimationCache.getAnimation(animURL).frames.length);
        if (typeof onFinished !== 'undefined') {
            print("onFinished defined. setting the timeout with timeOfAnim");
            timers.push(Script.setTimeout(onFinished, timeOfAnim));
        }
    });
}

function playSound(soundURL, onFinished) {
    callbackOnCondition(function() {
        return SoundCache.getSound(soundURL).downloaded;
    }, 250, function() {
        if (audioInjector) {
            audioInjector.stop();
        }
        audioInjector = Audio.playSound(SoundCache.getSound(soundURL), {position: Avatar.position, volume: 1.0});
        if (typeof onFinished !== 'undefined') {
            audioInjector.finished.connect(onFinished);
        }
    });
}

function npcRespond(soundURL, animURL, onFinished) {
    if (typeof soundURL !== 'undefined' && soundURL != '') {
        print("npcRespond got soundURL!");
        playSound(soundURL, function(){
            print("sound finished");
            var animDetails = Avatar.getAnimationDetails();
            print("animDetails.lastFrame: " + animDetails.lastFrame);
            print("animDetails.currentFrame: " + animDetails.currentFrame);
            if (animDetails.lastFrame < animDetails.currentFrame + 1 || !playingResponseAnim) {
                onFinished();
            }
            audioInjector = false;
        });
    }
    if (typeof animURL !== 'undefined' && animURL != '') {
        print("npcRespond got animURL!");
        playingResponseAnim = true;
        playAnim(animURL, false, function() {
            print("anim finished");
            playingResponseAnim = false;
            print("injector: " + audioInjector);
            if (!audioInjector || !audioInjector.isPlaying()) {
                print("resetting Timer");
                print("about to call onFinished");
                onFinished();
            }
        });
    }
}

function npcRespondBlocking(soundURL, animURL, onFinished) {
    print("blocking response requested");
    if (!blocked) {
        print("not already blocked");
        blocked = true;
        npcRespond(soundURL, animURL, function(){
            if (onFinished){
                onFinished();
            }blocked = false; 
        });
    }
}

function npcContinueStory(soundURL, animURL, nextID, onFinished) {
    if (!nextID) {
        nextID = _qid;
    }
    npcRespondBlocking(soundURL, animURL, function(){
        if (onFinished){
            onFinished();
        }setQid(nextID);
    });
}

function setQid(newQid) {
    print("setting quid");
    print("_qid: " + _qid);
    _qid = newQid;
    print("_qid: " + _qid);
    doActionFromServer("init");
}

function runOnClient(code) {
    Messages.sendMessage("interactionComs", "clientexec:" + code);
}

function doActionFromServer(action, data, useServerCache) {
    if (action == "start") {
        ignoreCount = 0;
        _qid = "start";
    }
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "http://gserv_devel.studiolimitless.com/story", true);
    xhr.onreadystatechange = function(){
        if (xhr.readyState == 4){
            if (xhr.status == 200) {
                print("200!");
                print("evaluating: " + xhr.responseText);
                Script.evaluate(xhr.responseText, "");
            } else if (xhr.status == 444) {
                print("Limitless Serv 444: API error: " + xhr.responseText);
            } else {
                print("HTTP Code: " + xhr.status + ": " + xhr.responseText);
            }
        }
    };
    xhr.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
    var postData = "url=" + storyURL + "&action=" + action + "&qid=" + _qid;
    if (typeof data !== 'undefined' && data != '') {
        postData += "&data=" + data;
    }
    if (typeof useServerCache !== 'undefined' && !useServerCache) {
        postData += "&nocache=true";
    }
    print("Sending: " + postData);
    xhr.send(postData);
}
