//
// automaticLookAt.js
// This script controls the avatar's look-at-target for the head and eyes, according to other avatar's actions
// It tries to simulate human interaction during group conversations
//
//  Created by Luis Cuenca on 11/11/19
//  Copyright 2019 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    
    //////////////////////////////////////////////
    // debugger.js ///////
    //////////////////////////////////////////////
    
    var TEXT_BOX_WIDTH = 350;
    var TEXT_BOX_MIN_HEIGHT = 40;
    var TEXT_BOX_TOP_MARGIN = 100;
    var TEXT_CAPTION_HEIGHT = 30;
    var TEXT_CAPTION_COLOR = { red: 0, green: 0, blue: 0 };
    var TEXT_CAPTION_SIZE = 18;
    var TEXT_CAPTION_MARGIN = 6;
    var CHECKBOX_MARK_MARGIN = 3;
    
    var DEGREE_TO_RADIAN = 0.0174533;
    var ENGAGED_AVATARS_DEBUG_COLOR = { red: 0, green: 255, blue: 255 };
    var ENGAGED_AVATARS_DEBUG_ALPHA = 0.3;
    var FOCUS_AVATAR_DEBUG_COLOR = { red: 255, green: 0, blue: 0 };
    var FOCUS_AVATAR_DEBUG_ALPHA = 1.0;
    var TALKER_AVATAR_DEBUG_COLOR = { red: 0, green: 0, blue: 0 };
    var TALKER_AVATAR_DEBUG_ALPHA = 0.8;
    var DEFAULT_OUTLINE_COLOR = { red: 155, green: 155, blue: 255 };
    var DEFAULT_OUTLINE_WIDTH = 2;
    
    var LookAtDebugger = function() {
        var self = this;
        var IMAGE_DIMENSIONS = {x: 0.2, y: 0.2, z:0.2};
        var TARGET_ICON_PATH = Script.getExternalPath(Script.ExternalPaths.HF_Content, "/luis/test_scripts/LookAtApp/eyeFocus.png");
        var INFINITY_ICON_PATH = Script.getExternalPath(Script.ExternalPaths.HF_Content, "/luis/test_scripts/LookAtApp/noFocus.png");
        this.items = {};
        this.active = false;
        
        // UI elements
        this.textBox;
        this.activeCheckBox;
        this.activeCheckBoxMark;
        this.activeCaption;
        this.textBoxHeight = 0.0;
        this.logs = [];

        this.getStyleProps = function(color, alpha) {
            return {
                fillUnoccludedColor: color,
                fillUnoccludedAlpha: alpha,                   
                fillOccludedColor: color,
                fillOccludedAlpha: alpha,
                outlineUnoccludedColor: DEFAULT_OUTLINE_COLOR,
                outlineUnoccludedAlpha: alpha,
                outlineOccludedColor: DEFAULT_OUTLINE_COLOR,
                outlineOccludedAlpha: alpha,
                outlineWidth: DEFAULT_OUTLINE_WIDTH,
                isOutlineSmooth: false
            }
        }
        this.Styles = {
            "engaged" : {
                "style": self.getStyleProps(ENGAGED_AVATARS_DEBUG_COLOR, ENGAGED_AVATARS_DEBUG_ALPHA),
                "name" : "engagedSel"
            },
            "focus" : {
                "style": self.getStyleProps(FOCUS_AVATAR_DEBUG_COLOR, FOCUS_AVATAR_DEBUG_ALPHA),
                "name" : "focusSel"
            },
            "talker" : {
                "style": self.getStyleProps(TALKER_AVATAR_DEBUG_COLOR, TALKER_AVATAR_DEBUG_ALPHA),
                "name" : "talkerSel"
            }
        };

        this.eyeTarget;
        this.eyesTargetProps = {
            name: "Eyes-Target-Image",
            position: { x: 0.0, y: 0.0, z: 0.0 },
            color: {red: 255, green: 0, blue: 255},
            url: TARGET_ICON_PATH,
            dimensions: IMAGE_DIMENSIONS,
            alpha: 1.0,
            visible: false,
            emissive: true,
            ignoreRayIntersection: true,
            drawInFront: true,
            grabbable: false,
            isFacingAvatar: true
        };
        this.headTarget;
        this.headTargetProps = {
            name: "Head-Target-Image",
            position: { x: 0.0, y: 0.0, z: 0.0 },
            color: {red: 0, green: 255, blue: 255},
            url: TARGET_ICON_PATH,
            dimensions: IMAGE_DIMENSIONS,
            alpha: 1.0,
            visible: false,
            emissive: true,
            ignoreRayIntersection: true,
            drawInFront: true,
            grabbable: false,
            isFacingAvatar: true
        };

        
        this.textBoxProps = {
            x: Window.innerWidth - TEXT_BOX_WIDTH,
            y: TEXT_BOX_TOP_MARGIN,
            width: TEXT_BOX_WIDTH,
            height: TEXT_BOX_MIN_HEIGHT,
            alpha: 0.7,
            color: { red: 255, green: 255, blue: 255 }
        };
        
        this.activeCheckBoxProps = {
            x: TEXT_CAPTION_MARGIN + self.textBoxProps.x,
            y: 2 * TEXT_CAPTION_MARGIN + self.textBoxProps.y,
            width: TEXT_CAPTION_SIZE,
            height: TEXT_CAPTION_SIZE,
            alpha: 0.0,
            borderWidth: 2,
            borderColor: TEXT_CAPTION_COLOR
        };
        
        this.activeCheckBoxMarkProps = {
            x: self.activeCheckBoxProps.x + CHECKBOX_MARK_MARGIN,
            y: self.activeCheckBoxProps.y + CHECKBOX_MARK_MARGIN,
            width: TEXT_CAPTION_SIZE - 2.0 * CHECKBOX_MARK_MARGIN,
            height: TEXT_CAPTION_SIZE - 2.0 * CHECKBOX_MARK_MARGIN,
            visible: false,
            alpha: 1.0,
            color: TEXT_CAPTION_COLOR
        }
        
        this.captionProps = {            
            x: 2 * TEXT_CAPTION_SIZE + self.textBoxProps.x,
            y: TEXT_CAPTION_MARGIN + self.textBoxProps.y,
            width: self.textBoxProps.width,
            height: 30,
            alpha: 1.0,
            backgroundAlpha: 0.0,
            visible: true,
            text: "Debug Auto Look At",
            font: {
                size: TEXT_CAPTION_SIZE
            },
            color: TEXT_CAPTION_COLOR,
            topMargin: 0.5 * TEXT_CAPTION_MARGIN
        };
        
        this.log = function(txt) {
            if (self.active) {
                self.logs.push(Overlays.addOverlay("text", self.captionProps));
                var y = self.textBoxProps.y + self.captionProps.height * (self.logs.length);
                Overlays.editOverlay(self.logs[self.logs.length - 1], {y: y, text: txt});
                var height = (TEXT_CAPTION_SIZE + 2.0 * TEXT_CAPTION_MARGIN)  * (self.logs.length + 2);
                if (this.textBoxHeight !== height) {
                    this.textBoxHeight = height
                    Overlays.editOverlay(self.textBox, {height: height});
                }
            }
        }
        
        this.clearLog = function() {
            for (var n = 0; n < self.logs.length; n++) {
                Overlays.deleteOverlay(self.logs[n]);
            }
            Overlays.editOverlay(self.textBox, self.textBoxProps);
            self.logs = [];
            self.log("____________________________");
        }
        
        this.setActive = function(isActive) {
            self.active = isActive;
            if (!self.active) {
                self.turnOff();
            } else {
                self.turnOn();
            }
        };
        
                
        this.onClick = function(event) {
            if (event.x > self.activeCheckBoxProps.x && event.x < (self.activeCheckBoxProps.x + self.activeCheckBoxProps.width) &&
                event.y > self.activeCheckBoxProps.y && event.y < (self.activeCheckBoxProps.y + self.activeCheckBoxProps.height)) {
                self.setActive(!self.active);
                Overlays.editOverlay(self.activeCheckBoxMark, {visible: self.active});
            }
        }
        
        this.turnOn = function() {
            for (var key in self.Styles) {
                var selStyle = self.Styles[key];
                Selection.enableListHighlight(selStyle.name, selStyle.style);
            }
            if (!self.eyeTarget) {
                self.eyeTarget = Overlays.addOverlay("image3d", self.eyesTargetProps);
            }
            if (!self.headTarget) {
                self.headTarget = Overlays.addOverlay("image3d", self.headTargetProps);
            }
        }
        
        this.init = function() {
            if (!self.textBox) {
                self.textBox = Overlays.addOverlay("rectangle", self.textBoxProps);
            }
            if (!self.activeCheckBox) {
                self.activeCheckBox = Overlays.addOverlay("rectangle", self.activeCheckBoxProps);
            }
            if (!self.activeCheckBoxMark) {
                self.activeCheckBoxMark = Overlays.addOverlay("rectangle", self.activeCheckBoxMarkProps);
            }
            if (!self.activeCaption) {
                self.activeCaption = Overlays.addOverlay("text", self.captionProps);
            }
        };
        
        this.highLightAvatars = function(engagedIDs, focusID, talkerID) {
            if (self.active) {
                self.clearSelection();
                engagedIDs = !engagedIDs ? [] : engagedIDs;
                var focusIDs = !focusID ? [] : [focusID];                
                var talkerIDs = !talkerID ? [] : [talkerID];
                self.highLightIDs(self.Styles.engaged.name, engagedIDs);
                self.highLightIDs(self.Styles.focus.name, focusIDs);
                self.highLightIDs(self.Styles.talker.name, talkerIDs);
                return true;
            } else {
                return false;
            }
        };
        
        this.getTargetProps = function(target) {
            var distance = Vec3.length(Vec3.subtract(target, Camera.getPosition()));
            RETARGET_MAX_DISTANCE = 100.0;
            DIMENSION_SCALE = 0.05;
            var isInfinite = false;
            if (distance > RETARGET_MAX_DISTANCE) {
                isInfinite = true;
                var eyesToTarget = Vec3.multiply(RETARGET_MAX_DISTANCE, Vec3.normalize(Vec3.subtract(target, MyAvatar.getDefaultEyePosition())));
                var newTarget = Vec3.sum(MyAvatar.getDefaultEyePosition(), eyesToTarget);
                var cameraToTarget = Vec3.normalize(Vec3.subtract(newTarget, Camera.getPosition()));
                target = Vec3.sum(Camera.getPosition(), cameraToTarget);
                distance = Vec3.length(cameraToTarget);
            }
            // Scale the target to appear always with the same size on screen
            var fov = DEGREE_TO_RADIAN * Camera.frustum.fieldOfView;
            var scale = (Camera.frustum.aspectRatio < 1.0 ? Camera.frustum.aspectRatio : 1.0) * DIMENSION_SCALE;
            var dimensionRatio = scale * (distance / (0.5 * Math.tan(0.5 * fov)));
            var dimensions = Vec3.multiply(dimensionRatio, IMAGE_DIMENSIONS);
            return {"dimensions": dimensions, "visible": true, "position": target, "url": isInfinite ? INFINITY_ICON_PATH : TARGET_ICON_PATH};
        }
        
        this.showTarget = function(headTarget, eyeTarget) {
            if (!self.active) {
                return;
            }
            var targetProps = self.getTargetProps(eyeTarget);
            Overlays.editOverlay(self.eyeTarget, targetProps); 
            var headTargetProps = self.getTargetProps(headTarget);
            Overlays.editOverlay(self.headTarget, headTargetProps);
        }
        
        this.hideEyeTarget = function() {
            Overlays.editOverlay(self.eyeTarget, {"visible": false}); 
        }
        
        this.highLightIDs = function(selectionName, ids) {
            self.items[selectionName] = ids;
            for (var idx in ids) {
                Selection.addToSelectedItemsList(selectionName, "avatar", ids[idx]); 
            }
        }
        this.clearSelection = function() {
            for (key in self.Styles) {
                var selStyle = self.Styles[key];
                Selection.clearSelectedItemsList(selStyle.name);
            }
        }
        
        this.turnOff = function() {
            self.clearLog();
            for (var key in self.Styles) {
                var selStyle = self.Styles[key];
                Selection.disableListHighlight(selStyle.name);
            }
            Overlays.deleteOverlay(self.headTarget);
            self.headTarget = undefined;
            Overlays.deleteOverlay(self.eyeTarget);
            self.eyeTarget = undefined;
        }
        
        this.finish = function() {
            if (self.active) {
                self.setActive(false);
            }
            Overlays.deleteOverlay(self.textBox);
            self.textBox = undefined;
            Overlays.deleteOverlay(self.activeCheckBox);
            self.activeCheckBox = undefined;
            Overlays.deleteOverlay(self.activeCaption);
            self.activeCaption = undefined;
            Overlays.deleteOverlay(self.activeCheckBoxMark);
            self.activeCheckBoxMark = undefined;
        }
        
        this.init();
    }
     
    //////////////////////////////////////////////
    // randomHelper.js //////////
    //////////////////////////////////////////////
    
    var RandomHelper = function() {
        var self = this;
        
        this.createRandomIndexes = function(count) {
            var indexes = [];
            for (var n = 0; n < count; n++) {
                indexes.push(n);
            }
            var randomIndexes = [];
            for (var n = 0; n < count; n++) {
                var indexesCount = indexes.length;
                var randomIndex = 0;
                if (indexesCount > 1) {
                    var randFactor = Math.random();
                    randomIndex = randFactor !== 1.0 ? Math.floor(randFactor * indexes.length) : indexes.length - 1;
                }
                randomIndexes.push(indexes[randomIndex]);
                indexes.splice(randomIndex, 1);
            }
            return randomIndexes;
        }
        
        this.getRandomKey = function(keypool) {
            // keypool can be an object. {key1: percentage1, key2: percentage2} or
            // keypool can be an array. [key1, key2] Equal percentage each component

            var equalChance = Array.isArray(keypool);
            var totalPercentage = 0.0;
            var percentages = {};
            var normalizedPercentages = {};
            var keys = equalChance ? keypool : Object.keys(keypool);
            for (var n = 0; n < keys.length; n++) {
                var key = keys[n];
                percentages[key] = equalChance ? 1.0 / keys.length : keypool[key].chance;
                totalPercentage += equalChance ? percentages[key] : keypool[key].chance;
            }
            var accumulatedVal = 0.0;
            for (var n = 0; n < keys.length; n++) {
                var key = keys[n];
                var val = accumulatedVal + (percentages[key] / totalPercentage);
                normalizedPercentages[key] = val;
                accumulatedVal = val;
            }
            var dice = Math.random();
            var floor = 0.0;
            var hit = normalizedPercentages[keys[0]];
            for (var n = 0; n < keys.length; n++) {
                var key = keys[n];
                if (dice > floor && dice < normalizedPercentages[key]) {
                    hit = key;
                    break;
                }
                floor = normalizedPercentages[key];
            }
            return { randomKey: hit, chance: percentages[hit]};
        }
    }
    
    
    //////////////////////////////////////////////
    // automaticMachine.js //////////
    //////////////////////////////////////////////
    var MIN_LOOKAT_HEAD_MIX_ALPHA = 0.04;
    var MAX_LOOKAT_HEAD_MIX_ALPHA = 0.08;
    var CAMERA_HEAD_MIX_ALPHA = 0.06;
    
    var TargetType = {
        "unknown" : 0,
        "avatar" : 1,
        "entity" : 2
    }
    
    var TargetOffsetMode = {
        "noOffset" : 0,
        "onlyHead" : 1,
        "onlyEyes" : 2,
        "headAndEyes" : 3,
        "print" : function(sta) {
            return ("OffsetMode: " + (Object.keys(TargetOffsetMode))[sta]);
        }
    }
    
    var TargetMode = {
        "noTarget" : 0,
        "leftEye" : 1,
        "rightEye" : 2,
        "mouth" : 3,
        "leftHand" : 4,
        "rightHand" : 5,
        "random" : 6,
        "print" : function(sta) {
            return ("TargetMode: " + (Object.keys(TargetMode))[sta]);
        }
    }
    
    var ACTION_CONFIGURATION = {
        "TargetMode.mouth": {    
            "joint": "Head", 
            "stareTimeRange" : {"min": 0.2, "max": 2.0},
            "headSpeedRange" : {"min": MAX_LOOKAT_HEAD_MIX_ALPHA, "max": MIN_LOOKAT_HEAD_MIX_ALPHA},
            "offsetChances" : {
                "TargetOffsetMode.noOffset": {"chance": 0.7},
                "TargetOffsetMode.onlyHead": {"chance":  0.3},
                "TargetOffsetMode.onlyEyes": {"chance":  0.0},
                "TargetOffsetMode.headAndEyes": {"chance": 0.0}
            },
            "offsetAngleRange" : {"min": 1.0, "max": 5.0},
            "chanceWhileListening" : 0.45,
            "chanceWhileTalking" : 0.25
        },
        "TargetMode.leftEye": {    
            "joint": "LeftEye", 
            "stareTimeRange" : {"min": 0.2, "max": 2.0},
            "headSpeedRange" : {"min": MAX_LOOKAT_HEAD_MIX_ALPHA, "max": MIN_LOOKAT_HEAD_MIX_ALPHA},
            "offsetChances" : {
                "TargetOffsetMode.noOffset": {"chance": 0.5},
                "TargetOffsetMode.onlyHead": {"chance": 0.3},
                "TargetOffsetMode.onlyEyes": {"chance": 0.1},
                "TargetOffsetMode.headAndEyes": {"chance": 0.1}
            },
            "offsetAngleRange" : {"min": 1.0, "max": 5.0},
            "chanceWhileListening" : 0.20,
            "chanceWhileTalking" : 0.30
        },
        "TargetMode.rightEye": {   
            "joint": "RightEye", 
            "stareTimeRange" : {"min": 0.2, "max": 2.0},
            "headSpeedRange" : {"min": MAX_LOOKAT_HEAD_MIX_ALPHA, "max": MIN_LOOKAT_HEAD_MIX_ALPHA},
            "offsetChances" : {
                "TargetOffsetMode.noOffset": {"chance": 0.5},
                "TargetOffsetMode.onlyHead": {"chance": 0.3},
                "TargetOffsetMode.onlyEyes": {"chance": 0.1},
                "TargetOffsetMode.headAndEyes": {"chance": 0.1}
            },
            "offsetAngleRange" : {"min": 1.0, "max": 5.0},
            "chanceWhileListening" : 0.20,
            "chanceWhileTalking" : 0.30
        },
        "TargetMode.leftHand": {    
            "joint": "LeftHand", 
            "stareTimeRange" : {"min": 0.2, "max": 2.0},
            "headSpeedRange" : {"min": MAX_LOOKAT_HEAD_MIX_ALPHA, "max": MIN_LOOKAT_HEAD_MIX_ALPHA},
            "offsetChances" : {
                "TargetOffsetMode.noOffset": {"chance": 0.9},
                "TargetOffsetMode.onlyHead": {"chance": 0.1},
                "TargetOffsetMode.onlyEyes": {"chance": 0.0},
                "TargetOffsetMode.headAndEyes": {"chance": 0.0}
            },
            "offsetAngleRange" : {"min": 1.0, "max": 10.0},
            "chanceWhileListening" : 0.05,
            "chanceWhileTalking" : 0.05  
        },
        "TargetMode.rightHand": {   
            "joint": "RightHand", 
            "stareTimeRange" : {"min": 0.2, "max": 2.0},
            "headSpeedRange" : {"min": MAX_LOOKAT_HEAD_MIX_ALPHA, "max": MIN_LOOKAT_HEAD_MIX_ALPHA},
            "offsetChances" : {
                "TargetOffsetMode.noOffset": {"chance": 0.9},
                "TargetOffsetMode.onlyHead": {"chance": 0.1},
                "TargetOffsetMode.onlyEyes": {"chance": 0.0},
                "TargetOffsetMode.headAndEyes": {"chance": 0.0}
            },
            "offsetAngleRange" : {"min": 1.0, "max": 10.0},
            "chanceWhileListening" : 0.05,
            "chanceWhileTalking" : 0.05        
        },
        "TargetMode.random": {   
            "joint": "Head", 
            "stareTimeRange" : {"min": 0.2, "max": 1.0},
            "headSpeedRange" : {"min": MAX_LOOKAT_HEAD_MIX_ALPHA, "max": MIN_LOOKAT_HEAD_MIX_ALPHA},
            "offsetChances" : {
                "TargetOffsetMode.noOffset": {"chance": 0.0},
                "TargetOffsetMode.onlyHead": {"chance": 0.0},
                "TargetOffsetMode.onlyEyes": {"chance": 0.4},
                "TargetOffsetMode.headAndEyes": {"chance": 0.6}
            },
            "offsetAngleRange" : {"min": 5.0, "max": 12.0},
            "chanceWhileListening" : 0.05,
            "chanceWhileTalking" : 0.05        
        },
        "TargetMode.noTarget": {   
            "joint": undefined, 
            "stareTimeRange" : {"min": 0.1, "max": 2.0},
            "headSpeedRange" : {"min": MAX_LOOKAT_HEAD_MIX_ALPHA, "max": MIN_LOOKAT_HEAD_MIX_ALPHA},
            "offsetChances" : {
                "TargetOffsetMode.noOffset": {"chance": 0.5},
                "TargetOffsetMode.onlyHead": {"chance": 0.0},
                "TargetOffsetMode.onlyEyes": {"chance": 0.5},
                "TargetOffsetMode.headAndEyes": {"chance": 0.0}
            },
            "offsetAngleRange" : {"min": 1.0, "max": 15.0},
            "chanceWhileListening" : 0.0,
            "chanceWhileTalking" : 0.0        
        }
    }
    
    var FOCUS_MODE_CHANCES = {
        "idle" : {
            "TargetMode.mouth": {"chance": 0.2}, 
            "TargetMode.rightEye": {"chance": 0.4}, 
            "TargetMode.leftEye": {"chance": 0.4}
        },
        "talking" : ["TargetMode.mouth", "TargetMode.rightEye", "TargetMode.leftEye"] // Equal chances (33.33% each)
    }
    
    var LookAction = function() {
        var self = this;
        this.targetType = TargetType.unknown;
        this.speed = MIN_LOOKAT_HEAD_MIX_ALPHA;
        this.id = "";
        this.focusName = "None";
        this.focusChance = 0.0;
        this.config = undefined;
        this.targetMode = undefined;
        this.lookAtJoint = undefined;
        this.targetPoint = undefined;
        this.elapseTime = 0.0;
        this.totalTime = 1.0;
        this.eyesHeadOffset = Vec3.ZERO;
        this.eyesForward = false;
        this.offsetEyes = false;
        this.offsetHead = false;
        this.offsetModeName = "";
        this.offsetChance = 0.0;
        this.confortAngle = 0.0;
        this.printChance = function(chance) {
            return "" + Math.floor(100 * chance) + "%";
        }
        this.print = function() {
            var lines = []; 
            lines.push(TargetMode.print(eval(self.targetMode)) + "  P: " + self.printChance(self.focusChance));
            lines.push(TargetOffsetMode.print(eval(self.offsetModeName)) + "  P: " + self.printChance(self.offsetChance));
            lines.push("Action time: " + self.totalTime.toFixed(2) + " seconds");
            return lines;
        }
    }
    
    var AudienceAvatar = function(id) {
        var self = this;
        this.id = id;
        this.name = "";
        this.engaged = true;
        this.moved = true;
        this.isTalking = false;
        this.isListening = false;
        this.position = Vec3.ZERO;
        this.headPosition = Vec3.ZERO;
        this.leftPalmPosition = Vec3.ZERO;
        this.rightPalmPosition = Vec3.ZERO;
        this.leftHandSpeed = 0.0;
        this.rightHandSpeed = 0.0;
        this.velocity = 0.0;
        this.reactionTime = 0.0;
        this.distance = 0.0;
        this.loudness = 0.0;
        this.talkingTime = 0.0;
    }
    
    
    var SmartLookMachine = function() {
        var self = this;
        this.myAvatarID = MyAvatar.sessionUUID;
        
        this.nearAvatarList = {};
        this.dice = new RandomHelper();

        var LOOK_FOR_AVATARS_MAX_DISTANCE = 15.0;
        var MIN_FOCUS_TO_LISTENER_TIME = 3.0;
        var MAX_FOCUS_TO_LISTENER_TIME = 5.0;
        var MIN_FOCUS_TO_TALKER_TIME = 0.5;
        var MAX_FOCUS_TO_TALKER_RANGE = 1.5;
        var TRIGGER_FOCUS_WHILE_IDLE_CHANCE = 0.1;
        

                
        this.currentAvatarFocusID = undefined;
        this.currentTalker;
        this.currentAction = new LookAction();
        
        this.eyesTargetPoint = Vec3.ZERO;
        this.headTargetPoint = Vec3.ZERO;
        

        this.timeScale = 1.0;
        this.lookAtDebugger = new LookAtDebugger();
        
        this.active = true;
        this.headTargetSpeed = 0.0;

        this.avatarFocusTotalTime = 0.0;
        this.avatarFocusMax = 0.0;
        this.lockedFocusID = undefined;
        
        this.visibilityCount = 0;
        
        this.shouldUpdateDebug = false;
        
        var TalkingState = {
            "noTalking" : 0,
            "meTalkingFirst" : 1,
            "meTalkingAgain" : 2,
            "otherTalkingFirst" : 3,
            "otherTalkingAgain" : 4,
            "othersTalking": 5,
            "print" : function(sta) {
                return ("TalkingState: " + (Object.keys(TalkingState))[sta]);
            }
        }
        this.talkingState = TalkingState.noTalking;
        var FocusState = {
            "onNobody" : 0,
            "onTalker" : 1,
            "onRandomAudience" : 2,
            "onLastTalker" : 3,
            "onRandomLastTalker" : 4,
            "onLastFocus" : 5,
            "onSelected" : 6,
            "onMovement" : 7,
            "print" : function(sta) {
                return ("FocusState: " + (Object.keys(FocusState))[sta]);
            }
        } 
        
        this.focusState = FocusState.onNobody;
        
        var LockFocusType = {
            "none" : 0,
            "click" : 1,
            "movement" : 2
        }
        self.lockFocusType = LockFocusType.none;
        
        this.wasMeTalking = false;
        this.nearAvatarIDs = [];
        
        this.updateAvatarVisibility = function() {
            if (self.nearAvatarIDs.length > 0) {
                if (self.nearAvatarList[self.myAvatarID] && self.nearAvatarList[self.myAvatarID].moved) {
                    for (id in self.nearAvatarList) {
                        self.nearAvatarList[id].moved = true;
                    }
                    self.nearAvatarList[self.myAvatarID].moved = false;
                }
                self.visibilityCount = ((self.visibilityCount + 1) >= self.nearAvatarIDs.length) ? 0 : self.visibilityCount + 1;
                var id = self.nearAvatarIDs[self.visibilityCount];
                var avatar = self.nearAvatarList[id];
                if (id !== self.myAvatarID && avatar !== undefined && avatar.moved) {
                    self.nearAvatarList[id].moved = false;
                    var eyePos = MyAvatar.getDefaultEyePosition();
                    var avatarSight = Vec3.subtract(avatar.headPosition, eyePos);
                    var intersection = Entities.findRayIntersection({origin: eyePos, direction: Vec3.normalize(avatarSight)}, true);
                    self.nearAvatarList[avatar.id].engaged = !intersection.intersects || intersection.distance > Vec3.length(avatarSight);
                }
            }
        }
        
        this.getEngagedAvatars = function() {
            var engagedAvatarIDs = [];
            for (var id in self.nearAvatarList) {
                if (self.nearAvatarList[id].engaged) {
                    engagedAvatarIDs.push(id);
                }
            }
            return engagedAvatarIDs;
        }
        
        this.updateAvatarList = function(deltaTime) {
            var TALKING_LOUDNESS_THRESHOLD = 50.0;
            var SILENCE_TALK_ATTENUATION = 0.5;
            var ATTENTION_HANDS_SPEED = 5.0;
            var ATTENTION_AVATAR_SPEED = 2.0;
            var MAX_TALKING_TIME = 5.0;
            var talkingAvatarID;
            var maxLoudness = 0.0;
            var count = 0;
            var previousTalkers = [];
            var fastHands = [];
            var fastMovers = [];
            var lookupCenter = Vec3.sum(MyAvatar.position, Vec3.multiply(0.8 * LOOK_FOR_AVATARS_MAX_DISTANCE, Quat.getFront(MyAvatar.orientation)));
            var nearbyAvatars = AvatarManager.getAvatarsInRange(lookupCenter, LOOK_FOR_AVATARS_MAX_DISTANCE);
            for (var n = 0; n < nearbyAvatars.length; n++) {
                var avatar = AvatarManager.getAvatar(nearbyAvatars[n]);
                var distance = Vec3.distance(MyAvatar.position, avatar.position);
                var loudness = avatar.audioLoudness;
                loudness = avatar.audioLoudness > 30.0 ? 100.0 : 0.0;
                var TALKING_TAU = 0.01;
                if (self.nearAvatarList[avatar.sessionUUID] === undefined) {
                    self.nearAvatarList[avatar.sessionUUID] = new AudienceAvatar(avatar.sessionUUID);
                    self.nearAvatarList[avatar.sessionUUID].name = avatar.displayName;
                } else {
                    if (Vec3.distance(self.nearAvatarList[avatar.sessionUUID].position, avatar.position) > 0.0) {
                        self.nearAvatarList[avatar.sessionUUID].moved = true;
                    } else {
                        self.nearAvatarList[avatar.sessionUUID].velocity = Vec3.length(avatar.velocity);
                        if (self.nearAvatarList[avatar.sessionUUID].velocity > 0.0) {
                            self.nearAvatarList[avatar.sessionUUID].moved = true;
                        }
                    }
                    self.nearAvatarList[avatar.sessionUUID].position = avatar.position;
                    self.nearAvatarList[avatar.sessionUUID].headPosition = avatar.getJointPosition("Head");
                    if (self.nearAvatarList[avatar.sessionUUID].engaged) {
                        self.nearAvatarList[avatar.sessionUUID].loudness = self.nearAvatarList[avatar.sessionUUID].loudness + TALKING_TAU * (loudness - self.nearAvatarList[avatar.sessionUUID].loudness);
                        self.nearAvatarList[avatar.sessionUUID].orientation = avatar.orientation;
                        var leftPalmPos = avatar.getJointPosition("LeftHand");
                        var rightPalmPos = avatar.getJointPosition("RightHand");

                        var distanceAttenuation = (distance > 1.0) ? (1.0 / distance) : 1.0;
                        var leftPalmSpeed = distanceAttenuation * Vec3.distance(self.nearAvatarList[avatar.sessionUUID].leftPalmPosition, leftPalmPos) / deltaTime;
                        var rightPalmSpeed = distanceAttenuation * Vec3.distance(self.nearAvatarList[avatar.sessionUUID].rightPalmPosition, rightPalmPos) / deltaTime;                 
                        self.nearAvatarList[avatar.sessionUUID].leftPalmSpeed = leftPalmSpeed;
                        self.nearAvatarList[avatar.sessionUUID].rightPalmSpeed = rightPalmSpeed;

                        self.nearAvatarList[avatar.sessionUUID].leftPalmPosition = leftPalmPos;
                        self.nearAvatarList[avatar.sessionUUID].rightPalmPosition = rightPalmPos;
                        if (self.nearAvatarList[avatar.sessionUUID] && self.nearAvatarList[avatar.sessionUUID].loudness) {
                            self.nearAvatarList[avatar.sessionUUID].loudness += 0.1;
                        }
                        self.nearAvatarList[avatar.sessionUUID].isTalking = false;
                        self.nearAvatarList[avatar.sessionUUID].isTalker = false;
                        if (self.nearAvatarList[avatar.sessionUUID].loudness > TALKING_LOUDNESS_THRESHOLD) {
                            if (self.nearAvatarList[avatar.sessionUUID].talkingTime < MAX_TALKING_TIME) {
                                self.nearAvatarList[avatar.sessionUUID].talkingTime += deltaTime;
                            }
                            self.nearAvatarList[avatar.sessionUUID].isTalking = true;
                            count++;
                            if (maxLoudness < self.nearAvatarList[avatar.sessionUUID].loudness) {
                                maxLoudness = self.nearAvatarList[avatar.sessionUUID].loudness;
                                talkingAvatarID = avatar.sessionUUID;
                            }
                        } else if (self.nearAvatarList[avatar.sessionUUID].talkingTime > 0.0){
                            self.nearAvatarList[avatar.sessionUUID].talkingTime -= SILENCE_TALK_ATTENUATION * deltaTime;
                            if (self.nearAvatarList[avatar.sessionUUID].talkingTime < 0.0) {
                                self.nearAvatarList[avatar.sessionUUID].talkingTime = 0.0;
                            }
                        }
                        if (!self.nearAvatarList[avatar.sessionUUID].isTalking && self.nearAvatarList[avatar.sessionUUID].talkingTime > 0.0){
                            previousTalkers.push(avatar.sessionUUID);
                        }
                        if ((leftPalmSpeed > ATTENTION_AVATAR_SPEED || rightPalmSpeed > ATTENTION_AVATAR_SPEED) && avatar.sessionUUID !== self.myAvatarID) {
                            if (!self.nearAvatarList[avatar.sessionUUID].isTalking) {
                                fastHands.push(avatar.sessionUUID);
                            } else if (self.nearAvatarList[avatar.sessionUUID].isTalking) {
                                var headPos = avatar.getJointPosition("Neck");
                                var raisedHand = Math.max(leftPalmPos.y, rightPalmPos.y) > headPos.y;
                                if (raisedHand) {
                                    // If the talker raise the hands it will trigger attention
                                    fastHands.push(avatar.sessionUUID);
                                }
                            }
                        }   
                    }
                }
            }
            
            for (var id in self.nearAvatarList) {
                if (nearbyAvatars.indexOf(id) == -1) {
                    delete self.nearAvatarList[id];
                }
            }
            self.nearAvatarIDs = Object.keys(self.nearAvatarList);
            if (self.nearAvatarList[self.myAvatarID] === undefined) {
                self.myAvatarID = MyAvatar.sessionUUID;
            }
            if (talkingAvatarID !== undefined) {
                self.nearAvatarList[talkingAvatarID].isTalker = true;
            }
            self.updateAvatarVisibility();
            return { talker: talkingAvatarID, talkingCount: count, previousTalkers: previousTalkers, fastHands: fastHands, fastMovers: fastMovers };
        }       
        
        this.getHeadConfortAngle = function(point) {
            var eyesToPoint = Vec3.subtract(point, MyAvatar.getDefaultEyePosition());
            var angle = Vec3.getAngle(eyesToPoint, Quat.getFront(MyAvatar.orientation)) / DEGREE_TO_RADIAN;
            angle = Math.min(angle, 90.0);
            var MAX_OFFSET_DEGREES = 20.0;
            var offsetMultiplier = MAX_OFFSET_DEGREES / 90.0;
            var facingRight = Vec3.dot(eyesToPoint, Quat.getRight(MyAvatar.orientation)) > 0.0;
            angle = (facingRight ? 1.0 : -1.0) * offsetMultiplier * angle;
            return angle;
        }
            
        this.updateCurrentAction = function(deltaTime) {
            if (self.currentAction.lookAtJoint !== undefined) {
                var avatar = AvatarList.getAvatar(self.currentAction.id);
                self.currentAction.targetPoint = avatar.getJointPosition(self.currentAction.lookAtJoint);
            }
            self.currentAction.elapseTime += deltaTime; 
        }
        
        this.requestNewAction = function(targetType, id) {
            
            var HAND_ATTENTION_TRIGGER_SPEED = 0.2;
            var action = new LookAction();
            action.targetType = targetType;
            action.id = id;
            var sortStare = false;
            action.targetMode = "TargetMode.noTarget";
            if (targetType == TargetType.avatar && id !== undefined && self.nearAvatarList[id] !== undefined) {
                var avatar = AvatarList.getAvatar(id);
                action.focusName = self.nearAvatarList[id].name;
                if (self.nearAvatarList[id].rightPalmSpeed > HAND_ATTENTION_TRIGGER_SPEED || self.nearAvatarList[id].leftPalmSpeed > HAND_ATTENTION_TRIGGER_SPEED) {
                    if (self.nearAvatarList[id].rightPalmSpeed < self.nearAvatarList[id].leftPalmSpeed) {
                        action.targetMode = "TargetMode.leftHand";
                    } else {
                        action.targetMode = "TargetMode.rightHand";
                    }
                    action.focusChance = 1.0;
                } else {
                    var faceChances = self.nearAvatarList[id].isTalking ? FOCUS_MODE_CHANCES.talking : FOCUS_MODE_CHANCES.idle;
                    var randomFaceTargetMode = self.dice.getRandomKey(faceChances);
                    action.targetMode = randomFaceTargetMode.randomKey;
                    action.focusChance = randomFaceTargetMode.chance;
                }
                
            } else if (targetType == TargetType.entity) {
                // TODO
                // Randomize around the entity size
            }
            var actionConfig = ACTION_CONFIGURATION[action.targetMode];
            action.config = actionConfig;
            action.lookAtJoint = actionConfig.joint;
            action.targetPoint = action.lookAtJoint !== undefined ? avatar.getJointPosition(action.lookAtJoint) : action.targetPoint = MyAvatar.getHeadLookAt();
            var randomKeyResult = self.dice.getRandomKey(actionConfig.offsetChances);
            action.offsetModeName = randomKeyResult.randomKey;
            action.offsetChance = randomKeyResult.chance;
            var offsetMode = eval(action.offsetModeName);
            if (offsetMode !== TargetOffsetMode.noOffset) {
                var headPosition = MyAvatar.getJointPosition("Head");
                var headToTarget = Vec3.subtract(action.targetPoint, headPosition);
                var randAngle = actionConfig.offsetAngleRange.min + Math.random() * (actionConfig.offsetAngleRange.max - actionConfig.offsetAngleRange.min);
                var randAngle = Math.random() < 0.5 ? -randAngle : randAngle;
                if (self.nearAvatarList[self.myAvatarID]) {
                    var randOffsetRotation = Quat.angleAxis(randAngle, Vec3.UNIT_Y);
                    action.eyesHeadOffset = Vec3.subtract(Vec3.sum(headPosition, Vec3.multiplyQbyV(randOffsetRotation, headToTarget)), action.targetPoint);
                }
                action.offsetEyes = offsetMode === TargetOffsetMode.onlyEyes || offsetMode === TargetOffsetMode.headAndEyes;
                action.offsetHead = offsetMode === TargetOffsetMode.onlyHead || offsetMode === TargetOffsetMode.headAndEyes;
            }
            action.totalTime = actionConfig.stareTimeRange.min + Math.random() * (actionConfig.stareTimeRange.max - actionConfig.stareTimeRange.min);
            action.confortAngle = self.getHeadConfortAngle(action.targetPoint);            
            action.speed = actionConfig.headSpeedRange.min + Math.random() * (actionConfig.headSpeedRange.max - actionConfig.headSpeedRange.min);
            
            return action;
        }

        this.findAudienceAvatar = function(avatarIDs) {
            // We look for avatars on the avatarIDs array if provided
            // If not avatarIDs becomes the array with all the engaged avatars nearAvatarList
            if (avatarIDs === undefined) {
                avatarIDs = self.nearAvatarIDs;
            }
            var randAvatarID;
            var MAX_AUDIENCE_DISTANCE = 8;
            var firstAnyOther = undefined;
            var firstNearOther = undefined;
            if (avatarIDs.length > 1) {
                var randomIndexes = self.dice.createRandomIndexes(avatarIDs.length);
                for (var n = 0; n < randomIndexes.length; n++) {
                    var avatarID = avatarIDs[randomIndexes[n]];
                    var avatar = self.nearAvatarList[avatarID];
                    if (avatarID != self.myAvatarID && avatar.engaged) {
                        firstAnyOther = !firstAnyOther ? avatarID : firstAnyOther;
                        
                        if (avatar.distance < MAX_AUDIENCE_DISTANCE) {
                            firstNearOther = !firstNearOther ? avatarID : firstNearOther;
                            var otherToMe = Vec3.normalize(Vec3.subtract(self.nearAvatarList[self.myAvatarID].position, avatar.position));
                            var myFront = Quat.getFront(self.nearAvatarList[self.myAvatarID].orientation);
                            var otherFront = Quat.getFront(avatar.orientation);
                            if (Vec3.dot(otherToMe, otherFront) > 0.0 && Vec3.dot(myFront, otherToMe) < 0.0) {
                                randAvatarID = avatarID;
                                break;
                            }
                        }
                        if (n === randomIndexes.length - 1) {
                            // We have not found a valid candidate facing us
                            // return the first id different from out avatar's id
                            randAvatarID = firstNearOther !== undefined ? firstNearOther : firstAnyOther;
                        }
                    }
                }
            } else if (avatarIDs.length > 0 && avatarIDs[0] != self.myAvatarID && self.nearAvatarList[avatarIDs[0]].engaged){
                // If the array provided only has one ID 
                randAvatarID = avatarIDs[0];
            }
            return randAvatarID;
        }
        
        this.applyHeadOffset = function(point, angle) {     
            var eyesToPoint = Vec3.subtract(point, MyAvatar.getDefaultEyePosition());
            var offsetRot = Quat.angleAxis(angle, Quat.getUp(MyAvatar.orientation));
            var offsetPoint = Vec3.sum(MyAvatar.getDefaultEyePosition(), Vec3.multiplyQbyV(offsetRot, eyesToPoint));
            return offsetPoint;
        }
        
        this.computeTalkingState = function(sceneData, myAvatarID, currentTalker, currentFocus) {
            var talkingState = TalkingState.noTalking;
            if (sceneData.talker === myAvatarID) {
                if (currentTalker != myAvatarID) {
                    talkingState = TalkingState.meTalkingFirst;
                } else {
                    talkingState = TalkingState.meTalkingAgain;
                }
            } else if (sceneData.talkingCount > 1) {
                talkingState = TalkingState.othersTalking;
            } else if (sceneData.talkingCount > 0) {
                if (sceneData.talker !== currentFocus) {
                    talkingState = TalkingState.otherTalkingFirst;
                } else {
                    talkingState = TalkingState.otherTalkingAgain;
                } 
            }
            return talkingState;
        }
        
        this.computeFocusState = function(sceneData, talkingState, currentFocus, lockedFocus, lockType) {
            var focusState = FocusState.onNobody;
            switch (talkingState) {
                case TalkingState.noTalking : {
                    if (sceneData.previousTalkers.length > 0) {
                        focusState = FocusState.onLastTalker;
                    } else if (Math.random() < TRIGGER_FOCUS_WHILE_IDLE_CHANCE) {
                        // There is chance of triggering a random focus when nobody is talking
                        focusState = FocusState.onRandomAudience;
                    } else {
                        focusState = FocusState.onNobody;
                    }
                    break;
                }
                case TalkingState.meTalkingFirst : {
                    if (currentFocus !== undefined) {
                        // Look at the last focused avatar
                        focusState = FocusState.onLastFocus;
                    } else if (sceneData.previousTalkers.length > 0) {
                        // Look at one of the previous talkers
                        focusState = FocusState.onRandomLastTalker;
                    } else {
                        focusState = FocusState.onRandomAudience;
                    }
                    break;
                }
                case TalkingState.meTalkingAgain : {
                    // Look at any random avatar
                    focusState = FocusState.onRandomAudience;
                    break;
                }
                case TalkingState.otherTalkingAgain : {
                    // If we were focused already on the talker we have a 15% chance to look at somebody else
                    // randomly giving preference to the previous talkers
                    if (Math.random() < 0.15) {
                        focusState = FocusState.onRandomLastTalker;
                    } else {
                        focusState = FocusState.onTalker;
                    }
                    break;
                }
                case TalkingState.otherTalkingFirst : {
                    // Focus on the new talker
                    focusState = FocusState.onTalker;
                    break;
                }
                case TalkingState.othersTalking : {
                    // When multiple people talk at the same time we have a 50% chance of not changing focus
                    if (Math.random() < 0.5) {
                        focusState = FocusState.onLastFocus;
                    } else {
                        focusState = FocusState.onTalker;
                    }                        
                    break;
                }
            }
            if (lockedFocus !== undefined) {
                if (lockType === LockFocusType.click) {
                    focusState = FocusState.onSelected;
                } else if (lockType === LockFocusType.movement) {
                    focusState = FocusState.onMovement;
                }
            }
            return focusState;
        }
        
        this.computeAvatarFocus = function(sceneData, focusState, currentFocus, lockedFocus) {
            var avatarFocusID = undefined;
            switch (focusState) {
                case FocusState.onTalker: {
                    avatarFocusID = sceneData.talker;
                    break;
                }
                case FocusState.onRandomAudience: {
                    avatarFocusID = self.findAudienceAvatar();
                    break;
                }
                case FocusState.onLastTalker:
                case FocusState.onRandomLastTalker: {
                    if (sceneData.previousTalkers.length > 0) {
                        avatarFocusID = self.findAudienceAvatar(sceneData.previousTalkers);
                    }
                    if (avatarFocusID === undefined) {
                        // Guarantee a 20% chance of looking at somebody 
                        if (focusState === FocusState.onRandomLastTalker || Math.random() < 0.2) {
                            avatarFocusID = self.findAudienceAvatar();
                        }
                    }
                    break;
                }
                case FocusState.onLastFocus: {
                    avatarFocusID = currentFocus;
                    break;
                }
                case FocusState.onMovement:
                case FocusState.onSelected: {
                    avatarFocusID = lockedFocus;
                    break;
                }
            }
            return avatarFocusID;
        }
        
        this.forceFocus = function(avatarID) {
            if (self.nearAvatarList[avatarID] !== undefined) {
                self.lockedFocusID = avatarID;
                self.lockFocusType = LockFocusType.click;
            }
        }
        
        this.logAction = function(action) {
            self.lookAtDebugger.clearLog();
            self.lookAtDebugger.log(TalkingState.print(self.talkingState));
            self.lookAtDebugger.log("________________________Focus");
            self.lookAtDebugger.log("On avatar: " + action.focusName);
            self.lookAtDebugger.log(FocusState.print(self.focusState));
            self.lookAtDebugger.log("Focus time: " + self.avatarFocusMax.toFixed(2) + " seconds");
            self.lookAtDebugger.log("________________________Action");           
            var extraLogs = action.print();
            for (var n = 0; n < extraLogs.length; n++) {
                self.lookAtDebugger.log(extraLogs[n]);
            }
        }
        
        this.update = function(deltaTime) {
            var FPS = 60.0;
            var CLICKED_AVATAR_MAX_FOCUS_TIME = 10.0;
            self.timeScale = deltaTime * FPS;
            
            var sceneData = self.updateAvatarList(deltaTime);
            if (self.nearAvatarIDs.length === 0) {
                return;
            }
            
            var abortAction = self.lockFocusType === LockFocusType.click;
            // Focus on any avatar moving their hands
            if (sceneData.fastHands.length > 0 && self.lockFocusType === LockFocusType.none) {
                var randomFastHands = self.findAudienceAvatar(sceneData.fastHands);
                if (self.nearAvatarList[randomFastHands] && (!self.nearAvatarList[randomFastHands].isTalking || self.nearAvatarList[randomFastHands].isTalker)) {
                    abortAction = Math.random() < 0.3;
                    self.lockedFocusID = randomFastHands;
                    self.lockFocusType = LockFocusType.movement;
                }
            } else if (self.avatarFocusTotalTime >= self.avatarFocusMax) {
                self.lockFocusType = LockFocusType.none;
            }
            
            // Set the talking status   
            self.talkingState = self.computeTalkingState(sceneData, self.myAvatarID, self.currentTalker, self.currentAvatarFocusID);
            
            // If the talker change, we have a 50% chance to focus on them once the next action is completed.
            var otherTalkerTriggerRefocus = self.talkingState === TalkingState.otherTalkingFirst && Math.random() < 0.5;
            if (self.lockedFocusID !== undefined || otherTalkerTriggerRefocus) {
                // Force a new focus
                self.avatarFocusTotalTime = self.avatarFocusMax;
                if (abortAction) {
                    // Force a new action
                    self.currentAction.elapseTime = self.currentAction.totalTime;
                }
            }
            var needsNewFocus = self.avatarFocusTotalTime >= self.avatarFocusMax;
            var needsNewAction = self.currentAction.elapseTime >= self.currentAction.totalTime;
            var newAvatarFocusID = self.currentAvatarFocusID;
            
            if (needsNewAction && needsNewFocus) {
                self.focusState = self.computeFocusState(sceneData, self.talkingState, self.currentAvatarFocusID, self.lockedFocusID, self.lockFocusType);
                newAvatarFocusID = self.computeAvatarFocus(sceneData, self.focusState, self.currentAvatarFocusID, self.lockedFocusID);
                self.lockedFocusID = undefined;
                if (self.lockFocusType !== LockFocusType.click) {
                    if (self.talkingState === TalkingState.meTalkingAgain) {
                        self.avatarFocusMax = MIN_FOCUS_TO_TALKER_TIME + Math.random() * MAX_FOCUS_TO_TALKER_RANGE;
                    } else {
                        self.avatarFocusMax = MIN_FOCUS_TO_LISTENER_TIME + Math.random() * MAX_FOCUS_TO_LISTENER_TIME;
                    }
                } else {
                    self.avatarFocusMax = CLICKED_AVATAR_MAX_FOCUS_TIME;
                }
                self.avatarFocusTotalTime = 0.0;
                self.currentTalker = sceneData.talker;
                self.shouldUpdateDebug = true;
            } else {
                self.avatarFocusTotalTime += deltaTime;
            }
            if (needsNewAction) {
                var currentFocus = newAvatarFocusID;
                if (otherTalkerTriggerRefocus) {
                    // Reset the last action on the previous focus to provide a random delay
                    currentFocus = self.currentAction.id;
                    self.currentAction.elapseTime = 0.0;
                } else {
                    // Create a new action
                    self.currentAction = self.requestNewAction(TargetType.avatar, newAvatarFocusID);
                }
                
                if (self.currentAvatarFocusID !== newAvatarFocusID) {
                    self.currentAvatarFocusID = newAvatarFocusID;
                }
                
                if (self.currentAvatarFocusID === undefined ||
                    self.talkingState === TalkingState.meTalkingAgain ||
                    self.talkingState === TalkingState.meTalkingFirst) {
                    // Minimize the head speed when we are talking or looking nowhere 
                    self.currentAction.speed = MIN_LOOKAT_HEAD_MIX_ALPHA;
                }
                
                self.logAction(self.currentAction);
            } else {
                self.updateCurrentAction(deltaTime);
            }    
            
            self.headTargetPoint = self.currentAction.targetPoint;
            self.eyesTargetPoint = self.currentAction.targetPoint;
            if (self.currentAction.offsetHead) {
                self.headTargetPoint = Vec3.sum(self.headTargetPoint, self.currentAction.eyesHeadOffset);
            }
            self.headTargetPoint = self.applyHeadOffset(self.headTargetPoint, self.currentAction.confortAngle); 
            if (self.currentAction.offsetEyes) {
                self.eyesTargetPoint = Vec3.sum(self.eyesTargetPoint, self.currentAction.eyesHeadOffset);
            }
            self.headTargetSpeed = Math.min(1.0, self.currentAction.speed * self.timeScale);
        }
        
        this.getResults = function() {
            return {
                "eyesTarget" : self.eyesTargetPoint,
                "headTarget" : self.headTargetPoint,
                "headSpeed" : self.headTargetSpeed
            }
        }

    }
    
    //////////////////////////////////////////////
    // autoLook.js ///////
    //////////////////////////////////////////////
    
    var LookAtController = function() {
        var CAMERA_HEAD_MIX_ALPHA = 0.06;
        var LookState = {
            "CameraLookActivating": 0,
            "CameraLookActive":1,
            "ClickToLookDeactivating":2,
            "ClickToLookActive":3,
            "AutomaticLook": 4        
        }

        var self = this;
        this.smartLookAt = new SmartLookMachine();
        this.currentState = LookState.AutomaticLook;
        this.lookAtTarget = undefined;
        this.lookingAtAvatarID = undefined;
        this.lookingAvatarJointIndex = undefined;

        this.interpolatedHeadLookAt = MyAvatar.getHeadLookAt();
        
        var CLICK_TO_LOOK_TOTAL_TIME = 5.0;
        this.clickToLookTimer = 0;
        
        var CAMERA_LOOK_TOTAL_TIME = 5.0;
        this.cameraLookTimer = 0;
    
        this.timeScale = 1.0;
        this.eyesTarget = Vec3.ZERO;
        this.headTarget = Vec3.ZERO;
        
        this.mousePressEvent = function(event) {
            if (event.isLeftButton) {
                self.smartLookAt.lookAtDebugger.onClick(event);
                if (self.currentState === LookState.AutomaticLook) {
                    var pickRay = Camera.computePickRay(event.x, event.y);
                    var intersection = AvatarManager.findRayIntersection({origin: pickRay.origin, direction: pickRay.direction}, [], [self.smartLookAt.myAvatarID], false);
                    self.lookingAtAvatarID = intersection.intersects ? intersection.avatarID : undefined;
                    if (self.lookingAtAvatarID) {
                        self.smartLookAt.forceFocus(self.lookingAtAvatarID);
                    }
                }
            }
        }
        
        this.mouseMoveEvent = function(event) {
            if (event.isRightButton && self.currentState === LookState.AutomaticLook) {
                self.currentState = LookState.CameraLookActivating;
                self.cameraLookTimer = 0.0;
            }
        }
        
        this.updateHeadLookAtTarget = function(target, interpolatedTarget, speed, noPitch) {
            var eyesPosition = MyAvatar.getDefaultEyePosition();
            var targetRot = Quat.lookAt(eyesPosition, target, Quat.getUp(MyAvatar.orientation));
            var interpolatedRot = Quat.lookAt(eyesPosition, interpolatedTarget, Quat.getUp(MyAvatar.orientation));
            var newInterpolatedRot = Quat.mix(interpolatedRot, targetRot, speed);
            var newInterpolatedTarget = Vec3.sum(eyesPosition, Vec3.multiply(Vec3.distance(eyesPosition, target), Quat.getFront(newInterpolatedRot)));
            // avoid pitch
            if (noPitch) {
                newInterpolatedTarget.y = eyesPosition.y;
            }
            MyAvatar.setHeadLookAt(newInterpolatedTarget);
            return newInterpolatedTarget;
        }
        
        this.retargetHeadTo = function(target, speed, noPitch, tolerance) {
            var eyePos = MyAvatar.getDefaultEyePosition();
            var localTarget = Vec3.normalize(Vec3.subtract(target, eyePos));
            self.interpolatedHeadLookAt = self.updateHeadLookAtTarget(target, self.interpolatedHeadLookAt, speed, noPitch);
            return (Vec3.dot(Vec3.normalize(Vec3.subtract(self.interpolatedHeadLookAt, eyePos)), Vec3.normalize(localTarget)) > tolerance);        
        }
        
        var MAX_INTERPOLING_STEPS = 100;
        this.interpolatingSteps = 0.0;
        this.automaticResults = {};
    
        this.update = function(deltaTime) {
            // Update timeScale
            var FPS = 60.0;
            self.timeScale = deltaTime * FPS;
            var stateTransitSpeed = Math.min(1.0, CAMERA_HEAD_MIX_ALPHA * self.timeScale);
            
            var CLICK_RETARGET_TOLERANCE = 0.98;
            var CAMERA_RETARGET_TOLERANCE = 0.98;
            
            var headTarget = MyAvatar.getHeadLookAt();
            var eyesTarget = MyAvatar.getEyesLookAt();
            
            var isTargetValid = (self.lookAtTarget || self.lookingAtAvatarID);
            if (isTargetValid && self.currentState === LookState.ClickToLookActive) {
                if (self.lookingAtAvatarID) {
                    self.lookAtTarget = AvatarList.getAvatar(self.lookingAtAvatarID).getJointPosition(self.lookingAvatarJointIndex);
                }
                self.interpolatedHeadLookAt = self.updateHeadLookAtTarget(self.lookAtTarget, self.interpolatedHeadLookAt, stateTransitSpeed);
                MyAvatar.setEyesLookAt(self.lookAtTarget);
                if (self.clickToLookTimer >  CLICK_TO_LOOK_TOTAL_TIME) {
                    self.currentState = LookState.ClickToLookDeactivating;
                }
                self.clickToLookTimer += deltaTime;
            } else if (self.currentState === LookState.ClickToLookDeactivating) {
                var breakInterpolation = self.interpolatingSteps > MAX_INTERPOLING_STEPS;
                if (breakInterpolation || self.retargetHeadTo(self.automaticResults.headTarget, stateTransitSpeed, false, CLICK_RETARGET_TOLERANCE)) {
                    self.currentState = LookState.AutomaticLook;
                    self.interpolatingSteps = 0.0;
                }            
                if (breakInterpolation) {
                    console.log("Breaking look at interpolation");
                } else if (self.interpolatingSteps++ === 0.0){
                    console.log("Interpolating click");
                }
            } else if (self.currentState === LookState.CameraLookActivating) {
                var cameraFront = Quat.getFront(Camera.getOrientation());
                if (Camera.mode === "selfie") {
                    cameraFront = Vec3.multiply(-1, cameraFront);
                }
                var breakInterpolation = self.interpolatingSteps > MAX_INTERPOLING_STEPS;
                if (breakInterpolation || self.retargetHeadTo(Vec3.sum(MyAvatar.getDefaultEyePosition(), cameraFront), stateTransitSpeed, true, CAMERA_RETARGET_TOLERANCE)) {
                    self.currentState = LookState.CameraLookActive;
                    self.interpolatingSteps = 0.0;
                    MyAvatar.releaseHeadLookAtControl();
                    MyAvatar.releaseEyesLookAtControl();
                }
                if (breakInterpolation) {
                    console.log("Breaking camera interpolation");
                } else if (self.interpolatingSteps++ === 0.0){
                    console.log("Interpolating camera");
                }
            } else if (self.currentState === LookState.CameraLookActive) {
                if (self.cameraLookTimer > CAMERA_LOOK_TOTAL_TIME) {
                    // Set as initial target the current head look at
                    self.interpolatedHeadLookAt = MyAvatar.getHeadLookAt();
                    self.currentState = LookState.AutomaticLook;
                }
                self.cameraLookTimer += deltaTime;
            } else if (self.currentState === LookState.AutomaticLook) {
                var updateLookat = Vec3.length(MyAvatar.velocity) < 1.0;
                self.smartLookAt.update(deltaTime);
                if (updateLookat) {
                    self.automaticResults = self.smartLookAt.getResults();
                    self.headTarget = self.automaticResults.headTarget;
                    self.eyesTarget = self.automaticResults.eyesTarget;
                    self.interpolatedHeadLookAt = self.updateHeadLookAtTarget(self.headTarget, self.interpolatedHeadLookAt, self.automaticResults.headSpeed, true);           
                    MyAvatar.setEyesLookAt(self.eyesTarget);
                    headTarget = self.interpolatedHeadLookAt;
                    eyesTarget = self.eyesTarget;
                } else {
                    // Too fast. Stand by automatic look
                    MyAvatar.setEyesLookAt(MyAvatar.getEyesLookAt());
                    MyAvatar.setHeadLookAt(MyAvatar.getHeadLookAt());
                }
            }
            if (self.smartLookAt.shouldUpdateDebug) {
                // Update engaged avatars debugging when a new action is created
                var engagedAvatars = self.smartLookAt.getEngagedAvatars();
                self.smartLookAt.lookAtDebugger.highLightAvatars(engagedAvatars, 
                                                     self.smartLookAt.currentAction.id, 
                                                     self.smartLookAt.currentTalker !== self.smartLookAt.currentAction.id ? self.smartLookAt.currentTalker : undefined);
            }
            self.smartLookAt.lookAtDebugger.showTarget(headTarget, eyesTarget);
        }
        
        this.finish = function() {
            self.smartLookAt.lookAtDebugger.finish();
        }
    }
    var lookAtController = new LookAtController();
    Controller.mousePressEvent.connect(lookAtController.mousePressEvent);
    Controller.mouseMoveEvent.connect(lookAtController.mouseMoveEvent);
    Script.update.connect(lookAtController.update);
    Script.scriptEnding.connect(lookAtController.finish);
})();