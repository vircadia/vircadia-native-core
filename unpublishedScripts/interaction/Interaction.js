//
//  Interaction.js
//  scripts/interaction
//
//  Created by Trevor Berninger on 3/20/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function(){
    print("loading interaction script");

    var Avatar = false;
    var NPC = false;
    var previousNPC = false;
    var hasCenteredOnNPC = false;
    var distance = 10;
    var r = 8;
    var player = false;

    var baselineX = 0;
    var baselineY = 0;
    var nodRange = 20;
    var shakeRange = 20;

    var ticker = false;
    var heartbeatTimer = false;

    function callOnNPC(message) {
        if(NPC)
            Messages.sendMessage("interactionComs", NPC + ":" + message);
        else
            Messages.sendMessage("interactionComs", previousNPC + ":" + message);
    }

    LimitlessSpeechRecognition.onFinishedSpeaking.connect(function(speech) {
        print("Got: " + speech);
        callOnNPC("voiceData:" + speech);
    });

    LimitlessSpeechRecognition.onReceivedTranscription.connect(function(speech) {
        callOnNPC("speaking");
    });

    function setBaselineRotations(rot) {
        baselineX = rot.x;
        baselineY = rot.y;
    }

    function findLookedAtNPC() {
        var intersection = AvatarList.findRayIntersection({origin: MyAvatar.position, direction: Quat.getFront(Camera.getOrientation())}, true);
        if (intersection.intersects && intersection.distance <= distance){
            var npcAvatar = AvatarList.getAvatar(intersection.avatarID);
            if (npcAvatar.displayName.search("NPC") != -1) {
                setBaselineRotations(Quat.safeEulerAngles(Camera.getOrientation()));
                return intersection.avatarID;
            }
        }
        return false;
    }

    function isStillFocusedNPC() {
        var avatar = AvatarList.getAvatar(NPC);
        if (avatar) {
            var avatarPosition = avatar.position;
            return Vec3.distance(MyAvatar.position, avatarPosition) <= distance && Math.abs(Quat.dot(Camera.getOrientation(), Quat.lookAtSimple(MyAvatar.position, avatarPosition))) > 0.6;
        }
        return false; // NPC reference died. Maybe it crashed or we teleported to a new world?
    }

    function onWeLostFocus() {
        print("lost NPC: " + NPC);
        callOnNPC("onLostFocused");
        var baselineX = 0;
        var baselineY = 0;
    }

    function onWeGainedFocus() {
        print("found NPC: " + NPC);
        callOnNPC("onFocused");
        var rotation = Quat.safeEulerAngles(Camera.getOrientation());
        baselineX = rotation.x;
        baselineY = rotation.y;
        LimitlessSpeechRecognition.setListeningToVoice(true);
    }

    function checkFocus() {
        var newNPC = findLookedAtNPC();

        if (NPC && newNPC != NPC && !isStillFocusedNPC()) {
            onWeLostFocus();
            previousNPC = NPC;
            NPC = false;
        }
        if (!NPC && newNPC != false) {
            NPC = newNPC;
            onWeGainedFocus();
        }
    }

    function checkGesture() {
        var rotation = Quat.safeEulerAngles(Camera.getOrientation());

        var deltaX = Math.abs(rotation.x - baselineX);
        if (deltaX > 180) {
            deltaX -= 180;
        }
        var deltaY = Math.abs(rotation.y - baselineY);
        if (deltaY > 180) {
            deltaY -= 180;
        }

        if (deltaX >= nodRange && deltaY <= shakeRange) {
            callOnNPC("onNodReceived");
        } else if (deltaY >= shakeRange && deltaX <= nodRange) {
            callOnNPC("onShakeReceived");
        }
    }

    function tick() {
        checkFocus();
        if (NPC) {
            checkGesture();
        }
    }

    function heartbeat() {
        callOnNPC("beat");
    }

    Messages.subscribe("interactionComs");

    Messages.messageReceived.connect(function (channel, message, sender) {
        if(channel === "interactionComs" && player) {
            var codeIndex = message.search('clientexec');
            if(codeIndex != -1) {
                var code = message.substr(codeIndex+11);
                Script.evaluate(code, '');
            }
        }
    });

    this.enterEntity = function(id) {
        player = true;
        print("Something entered me: " + id);
        LimitlessSpeechRecognition.setAuthKey("testKey");
        if (!ticker) {
            ticker = Script.setInterval(tick, 333);
        }
        if(!heartbeatTimer) {
            heartbeatTimer = Script.setInterval(heartbeat, 1000);
        }
    };
    this.leaveEntity = function(id) {
        LimitlessSpeechRecognition.setListeningToVoice(false);
        player = false;
        print("Something left me: " + id);
        if (previousNPC)
            Messages.sendMessage("interactionComs", previousNPC + ":leftArea");
        if (ticker) {
            ticker.stop();
            ticker = false;
        }
        if (heartbeatTimer) {
            heartbeatTimer.stop();
            heartbeatTimer = false;
        }
    };
    this.unload = function() {
        print("Okay. I'm Unloading!");
        if (ticker) {
            ticker.stop();
            ticker = false;
        }
    };
    print("finished loading interaction script");
});
