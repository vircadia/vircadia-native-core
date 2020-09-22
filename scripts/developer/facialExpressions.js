//
// facialExpressions.js
// A script to set different emotions using blend shapes
//
// Author: Elisa Lupin-Jimenez
// Copyright High Fidelity 2018
//
// Licensed under the Apache 2.0 License
// See accompanying license file or http://apache.org/
//
// All assets are under CC Attribution Non-Commerical
// http://creativecommons.org/licenses/
//

(function() {

    var TABLET_BUTTON_NAME = "EMOTIONS";
    // TODO: ADD HTML LANDING PAGE

    var TRANSITION_TIME_SECONDS = 0.25;

    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var icon = Script.getExternalPath(Script.ExternalPaths.HF_Content, "/elisalj/emoji_scripts/icons/emoji-i.svg");
    var activeIcon = Script.getExternalPath(Script.ExternalPaths.HF_Content, "/elisalj/emoji_scripts/icons/emoji-a.svg");
    var isActive = true;

    var controllerMappingName;
    var controllerMapping;

    var tabletButton = tablet.addButton({
        icon: icon,
        activeIcon: activeIcon,
        text: TABLET_BUTTON_NAME,
        isActive: true
    });

    var toggle = function() {
        isActive = !isActive;
        tabletButton.editProperties({isActive: isActive});
        if (isActive) {
            Controller.enableMapping(controllerMappingName);
        } else {
            setEmotion(DEFAULT);
            Controller.disableMapping(controllerMappingName);
        }
    };

    tabletButton.clicked.connect(toggle);

    var DEFAULT = {
        "EyeOpen_L": 0.00,
        "EyeOpen_R": 0.00,
        "EyeBlink_L": 0.00,
        "EyeBlink_R": 0.00,
        "EyeSquint_L": 0.00,
        "EyeSquint_R": 0.00,
        "BrowsD_L": 0.00,
        "BrowsD_R": 0.00,
        "BrowsU_L": 0.00,
        "BrowsU_C": 0.00,
        "JawOpen": 0.00,
        "JawFwd": 0.00,
        "MouthFrown_L": 0.00,
        "MouthFrown_R": 0.00,
        "MouthSmile_L": 0.00,
        "MouthSmile_R": 0.00,
        "MouthDimple_L": 0.00,
        "MouthDimple_R": 0.00,
        "LipsUpperClose": 0.00,
        "LipsLowerClose": 0.00,
        "LipsLowerOpen": 0.00,
        "ChinUpperRaise": 0.00,
        "Sneer": 0.00,
        "Puff": 0.00
    };

    var SMILE = {
        "EyeOpen_L": 0.00,
        "EyeOpen_R": 0.00,
        "EyeBlink_L": 0.30,
        "EyeBlink_R": 0.30,
        "EyeSquint_L": 0.90,
        "EyeSquint_R": 0.90,
        "BrowsD_L": 1.00,
        "BrowsD_R": 1.00,
        "BrowsU_L": 0.00,
        "BrowsU_C": 0.00,
        "JawOpen": 0.00,
        "JawFwd": 0.00,
        "MouthFrown_L": 0.00,
        "MouthFrown_R": 0.00,
        "MouthSmile_L": 1.00,
        "MouthSmile_R": 1.00,
        "MouthDimple_L": 1.00,
        "MouthDimple_R": 1.00,
        "LipsUpperClose": 0.40,
        "LipsLowerClose": 0.30,
        "LipsLowerOpen": 0.25,
        "ChinUpperRaise": 0.35,
        "Sneer": 0.00,
        "Puff": 0.00
    };

    var LAUGH = {
        "EyeOpen_L": 0.00,
        "EyeOpen_R": 0.00,
        "EyeBlink_L": 0.45,
        "EyeBlink_R": 0.45,
        "EyeSquint_L": 0.75,
        "EyeSquint_R": 0.75,
        "BrowsD_L": 0.00,
        "BrowsD_R": 0.00,
        "BrowsU_L": 0.00,
        "BrowsU_C": 0.50,
        "JawOpen": 0.50,
        "JawFwd": 0.00,
        "MouthFrown_L": 0.00,
        "MouthFrown_R": 0.00,
        "MouthSmile_L": 1.00,
        "MouthSmile_R": 1.00,
        "MouthDimple_L": 1.00,
        "MouthDimple_R": 1.00,
        "LipsUpperClose": 0.00,
        "LipsLowerClose": 0.00,
        "LipsLowerOpen": 0.00,
        "ChinUpperRaise": 0.30,
        "Sneer": 1.00,
        "Puff": 0.30
    };

    var FLIRT = {
        "EyeOpen_L": 0.00,
        "EyeOpen_R": 0.00,
        "EyeBlink_L": 0.50,
        "EyeBlink_R": 0.50,
        "EyeSquint_L": 0.25,
        "EyeSquint_R": 0.25,
        "BrowsD_L": 0.00,
        "BrowsD_R": 1.00,
        "BrowsU_L": 0.55,
        "BrowsU_C": 0.00,
        "JawOpen": 0.00,
        "JawFwd": 0.00,
        "MouthFrown_L": 0.00,
        "MouthFrown_R": 0.00,
        "MouthSmile_L": 0.50,
        "MouthSmile_R": 0.00,
        "MouthDimple_L": 1.00,
        "MouthDimple_R": 1.00,
        "LipsUpperClose": 0.00,
        "LipsLowerClose": 0.00,
        "LipsLowerOpen": 0.00,
        "ChinUpperRaise": 0.00,
        "Sneer": 0.00,
        "Puff": 0.00
    };

    var SAD = {
        "EyeOpen_L": 0.00,
        "EyeOpen_R": 0.00,
        "EyeBlink_L": 0.30,
        "EyeBlink_R": 0.30,
        "EyeSquint_L": 0.30,
        "EyeSquint_R": 0.30,
        "BrowsD_L": 0.00,
        "BrowsD_R": 0.00,
        "BrowsU_L": 0.00,
        "BrowsU_C": 0.50,
        "JawOpen": 0.00,
        "JawFwd": 0.80,
        "MouthFrown_L": 0.80,
        "MouthFrown_R": 0.80,
        "MouthSmile_L": 0.00,
        "MouthSmile_R": 0.00,
        "MouthDimple_L": 0.00,
        "MouthDimple_R": 0.00,
        "LipsUpperClose": 0.00,
        "LipsLowerClose": 0.50,
        "LipsLowerOpen": 0.00,
        "ChinUpperRaise": 0.00,
        "Sneer": 0.00,
        "Puff": 0.00
    };

    var ANGRY = {
        "EyeOpen_L": 1.00,
        "EyeOpen_R": 1.00,
        "EyeBlink_L": 0.00,
        "EyeBlink_R": 0.00,
        "EyeSquint_L": 1.00,
        "EyeSquint_R": 1.00,
        "BrowsD_L": 1.00,
        "BrowsD_R": 1.00,
        "BrowsU_L": 0.00,
        "BrowsU_C": 0.00,
        "JawOpen": 0.00,
        "JawFwd": 0.00,
        "MouthFrown_L": 0.50,
        "MouthFrown_R": 0.50,
        "MouthSmile_L": 0.00,
        "MouthSmile_R": 0.00,
        "MouthDimple_L": 0.00,
        "MouthDimple_R": 0.00,
        "LipsUpperClose": 0.50,
        "LipsLowerClose": 0.50,
        "LipsLowerOpen": 0.00,
        "ChinUpperRaise": 0.00,
        "Sneer": 0.50,
        "Puff": 0.00
    };

    var FEAR = {
        "EyeOpen_L": 1.00,
        "EyeOpen_R": 1.00,
        "EyeBlink_L": 0.00,
        "EyeBlink_R": 0.00,
        "EyeSquint_L": 0.00,
        "EyeSquint_R": 0.00,
        "BrowsD_L": 0.00,
        "BrowsD_R": 0.00,
        "BrowsU_L": 0.00,
        "BrowsU_C": 1.00,
        "JawOpen": 0.15,
        "JawFwd": 0.00,
        "MouthFrown_L": 0.30,
        "MouthFrown_R": 0.30,
        "MouthSmile_L": 0.00,
        "MouthSmile_R": 0.00,
        "MouthDimple_L": 0.00,
        "MouthDimple_R": 0.00,
        "LipsUpperClose": 0.00,
        "LipsLowerClose": 0.00,
        "LipsLowerOpen": 0.00,
        "ChinUpperRaise": 0.00,
        "Sneer": 0.00,
        "Puff": 0.00
    };

    var DISGUST = {
        "EyeOpen_L": 0.00,
        "EyeOpen_R": 0.00,
        "EyeBlink_L": 0.25,
        "EyeBlink_R": 0.25,
        "EyeSquint_L": 1.00,
        "EyeSquint_R": 1.00,
        "BrowsD_L": 1.00,
        "BrowsD_R": 1.00,
        "BrowsU_L": 0.00,
        "BrowsU_C": 0.00,
        "JawOpen": 0.00,
        "JawFwd": 0.00,
        "MouthFrown_L": 1.00,
        "MouthFrown_R": 1.00,
        "MouthSmile_L": 0.00,
        "MouthSmile_R": 0.00,
        "MouthDimple_L": 0.00,
        "MouthDimple_R": 0.00,
        "LipsUpperClose": 0.00,
        "LipsLowerClose": 0.75,
        "LipsLowerOpen": 0.00,
        "ChinUpperRaise": 0.75,
        "Sneer": 1.00,
        "Puff": 0.00
    };


    function mixValue(valueA, valueB, percentage) {
        return valueA + ((valueB - valueA) * percentage);
    }

    var lastEmotionUsed = DEFAULT;
    var emotion = DEFAULT;
    var isChangingEmotion = false;
    var changingEmotionPercentage = 0.0;

    Script.update.connect(function(deltaTime) {
        if (!isChangingEmotion) {
            return;
        }
        changingEmotionPercentage += deltaTime / TRANSITION_TIME_SECONDS;
        if (changingEmotionPercentage >= 1.0) {
            changingEmotionPercentage = 1.0;
            isChangingEmotion = false;
            if (emotion === DEFAULT) {
                MyAvatar.hasScriptedBlendshapes = false;
            }
        }
        for (var blendshape in emotion) {
            MyAvatar.setBlendshape(blendshape,
                mixValue(lastEmotionUsed[blendshape], emotion[blendshape], changingEmotionPercentage));
        }
    });

    function setEmotion(currentEmotion) {
        if (emotion !== lastEmotionUsed) {
            lastEmotionUsed = emotion;
        }
        if (currentEmotion !== lastEmotionUsed) {
            changingEmotionPercentage = 0.0;
            emotion = currentEmotion;
            isChangingEmotion = true;
            MyAvatar.hasScriptedBlendshapes = true;
        }
    }


    controllerMappingName = 'Hifi-FacialExpressions-Mapping';
    controllerMapping = Controller.newMapping(controllerMappingName);

    controllerMapping.from(Controller.Hardware.Keyboard.H).to(function(value) {
        if (value !== 0) {
            setEmotion(SMILE);
        }
    });

    controllerMapping.from(Controller.Hardware.Keyboard.J).to(function(value) {
        if (value !== 0) {
            setEmotion(LAUGH);
        }
    });

    controllerMapping.from(Controller.Hardware.Keyboard.K).to(function(value) {
        if (value !== 0) {
            setEmotion(FLIRT);
        }
    });

    controllerMapping.from(Controller.Hardware.Keyboard.L).to(function(value) {
        if (value !== 0) {
            setEmotion(SAD);
        }
    });

    controllerMapping.from(Controller.Hardware.Keyboard.V).to(function(value) {
        if (value !== 0) {
            setEmotion(ANGRY);
        }
    });

    controllerMapping.from(Controller.Hardware.Keyboard.B).to(function(value) {
        if (value !== 0) {
            setEmotion(FEAR);
        }
    });

    controllerMapping.from(Controller.Hardware.Keyboard.M).to(function(value) {
        if (value !== 0) {
            setEmotion(DISGUST);
        }
    });

    controllerMapping.from(Controller.Hardware.Keyboard.N).to(function(value) {
        if (value !== 0) {
            setEmotion(DEFAULT);
        }
    });

    Controller.enableMapping(controllerMappingName);

    Script.scriptEnding.connect(function() {
        tabletButton.clicked.disconnect(toggle);
        tablet.removeButton(tabletButton);
        Controller.disableMapping(controllerMappingName);

        if (emotion !== DEFAULT || isChangingEmotion) {
            isChangingEmotion = false;
            for (var blendshape in DEFAULT) {
                MyAvatar.setBlendshape(blendshape, DEFAULT[blendshape]);
            }
            MyAvatar.hasScriptedBlendshapes = false;
        }
    });

}());