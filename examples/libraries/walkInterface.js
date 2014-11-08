//
//  walkInterface.js
//
//  version 1.000
//
//  Created by David Wooldridge, Autumn 2014
//
//  Presents the UI for the walk.js script v1.1
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

walkInterface = (function() {

    //  controller element positions and dimensions
    var _backgroundWidth = 350;
    var _backgroundHeight = 700;
    var _backgroundX = Window.innerWidth - _backgroundWidth - 58;
    var _backgroundY = Window.innerHeight / 2 - _backgroundHeight / 2;
    var _minSliderX = _backgroundX + 30;
    var _sliderRangeX = 295 - 30;
    var _jointsControlWidth = 200;
    var _jointsControlHeight = 300;
    var _jointsControlX = _backgroundX + _backgroundWidth / 2 - _jointsControlWidth / 2;
    var _jointsControlY = _backgroundY + 242 - _jointsControlHeight / 2;
    var _buttonsY = 20; // distance from top of panel to menu buttons
    var _bigButtonsY = 408; //  distance from top of panel to top of first big button

    // arrays of overlay names
    var _sliderThumbOverlays = [];
    var _backgroundOverlays = [];
    var _buttonOverlays = [];
    var _jointsControlOverlays = [];
    var _bigbuttonOverlays = [];

    // reference to the internal state
    var _state = {
        editingTranslation: false
    };

    // reference to the Motion object
    var _motion = null;

    var _walkAssets = null;

    // constants
    var MAX_WALK_SPEED = 1257;

    // look and feel
    var momentaryButtonTimer = null;

    // all slider controls have a range (with the exception of phase controls that are always +-180)
    var _sliderRanges = {
        "joints": [{
            "name": "hips",
            "pitchRange": 12,
            "yawRange": 18,
            "rollRange": 12,
            "pitchOffsetRange": 25,
            "yawOffsetRange": 25,
            "rollOffsetRange": 25,
            "swayRange": 0.12,
            "bobRange": 0.05,
            "thrustRange": 0.05,
            "swayOffsetRange": 0.25,
            "bobOffsetRange": 0.25,
            "thrustOffsetRange": 0.25
        }, {
            "name": "upperLegs",
            "pitchRange": 30,
            "yawRange": 35,
            "rollRange": 35,
            "pitchOffsetRange": 20,
            "yawOffsetRange": 20,
            "rollOffsetRange": 20
        }, {
            "name": "lowerLegs",
            "pitchRange": 10,
            "yawRange": 20,
            "rollRange": 20,
            "pitchOffsetRange": 180,
            "yawOffsetRange": 20,
            "rollOffsetRange": 20
        }, {
            "name": "feet",
            "pitchRange": 10,
            "yawRange": 20,
            "rollRange": 20,
            "pitchOffsetRange": 180,
            "yawOffsetRange": 50,
            "rollOffsetRange": 50
        }, {
            "name": "toes",
            "pitchRange": 90,
            "yawRange": 20,
            "rollRange": 20,
            "pitchOffsetRange": 90,
            "yawOffsetRange": 20,
            "rollOffsetRange": 20
        }, {
            "name": "spine",
            "pitchRange": 40,
            "yawRange": 40,
            "rollRange": 40,
            "pitchOffsetRange": 90,
            "yawOffsetRange": 50,
            "rollOffsetRange": 50
        }, {
            "name": "spine1",
            "pitchRange": 20,
            "yawRange": 40,
            "rollRange": 20,
            "pitchOffsetRange": 90,
            "yawOffsetRange": 50,
            "rollOffsetRange": 50
        }, {
            "name": "spine2",
            "pitchRange": 20,
            "yawRange": 40,
            "rollRange": 20,
            "pitchOffsetRange": 90,
            "yawOffsetRange": 50,
            "rollOffsetRange": 50
        }, {
            "name": "shoulders",
            "pitchRange": 35,
            "yawRange": 40,
            "rollRange": 20,
            "pitchOffsetRange": 180,
            "yawOffsetRange": 180,
            "rollOffsetRange": 180
        }, {
            "name": "upperArms",
            "pitchRange": 90,
            "yawRange": 90,
            "rollRange": 90,
            "pitchOffsetRange": 180,
            "yawOffsetRange": 180,
            "rollOffsetRange": 180
        }, {
            "name": "lowerArms",
            "pitchRange": 90,
            "yawRange": 90,
            "rollRange": 120,
            "pitchOffsetRange": 180,
            "yawOffsetRange": 180,
            "rollOffsetRange": 180
        }, {
            "name": "hands",
            "pitchRange": 90,
            "yawRange": 180,
            "rollRange": 90,
            "pitchOffsetRange": 180,
            "yawOffsetRange": 180,
            "rollOffsetRange": 180
        }, {
            "name": "head",
            "pitchRange": 20,
            "yawRange": 20,
            "rollRange": 20,
            "pitchOffsetRange": 90,
            "yawOffsetRange": 90,
            "rollOffsetRange": 90
        }]
    };

    // load overlay images
    var _controlsMinimisedTab = Overlays.addOverlay("image", {
        x: Window.innerWidth - 58,
        y: Window.innerHeight - 145,
        width: 50,
        height: 50,
        imageURL: pathToAssets + 'overlay-images/ddao-minimise-tab.png',
        visible: true,
        alpha: 0.9
    });

    var _controlsBackground = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX,
            y: _backgroundY,
            width: _backgroundWidth,
            height: _backgroundHeight
        },
        imageURL: pathToAssets + "overlay-images/ddao-background.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _backgroundOverlays.push(_controlsBackground);

    var _controlsBackgroundWalkEditStyles = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX,
            y: _backgroundY,
            width: _backgroundWidth,
            height: _backgroundHeight
        },
        imageURL: pathToAssets + "overlay-images/ddao-background-edit-styles.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _backgroundOverlays.push(_controlsBackgroundWalkEditStyles);

    var _controlsBackgroundWalkEditTweaks = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX,
            y: _backgroundY,
            width: _backgroundWidth,
            height: _backgroundHeight
        },
        imageURL: pathToAssets + "overlay-images/ddao-background-edit-tweaks.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _backgroundOverlays.push(_controlsBackgroundWalkEditTweaks);

    var _controlsBackgroundWalkEditHipTrans = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX,
            y: _backgroundY,
            width: _backgroundWidth,
            height: _backgroundHeight
        },
        imageURL: pathToAssets + "overlay-images/ddao-background-edit-translation.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _backgroundOverlays.push(_controlsBackgroundWalkEditHipTrans);

    var _controlsBackgroundWalkEditJoints = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX,
            y: _backgroundY,
            width: _backgroundWidth,
            height: _backgroundHeight
        },
        imageURL: pathToAssets + "overlay-images/ddao-background-edit-joints.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _backgroundOverlays.push(_controlsBackgroundWalkEditJoints);

    // load character joint selection control images
    var _hipsJointsTranslation = Overlays.addOverlay("image", {
        bounds: {
            x: _jointsControlX,
            y: _jointsControlY,
            width: 200,
            height: 300
        },
        imageURL: pathToAssets + "overlay-images/ddao-background-edit-hips-translation.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _jointsControlOverlays.push(_hipsJointsTranslation);

    var _hipsJointControl = Overlays.addOverlay("image", {
        bounds: {
            x: _jointsControlX,
            y: _jointsControlY,
            width: 200,
            height: 300
        },
        imageURL: pathToAssets + "overlay-images/ddao-background-edit-hips.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _jointsControlOverlays.push(_hipsJointControl);

    var _upperLegsJointControl = Overlays.addOverlay("image", {
        bounds: {
            x: _jointsControlX,
            y: _jointsControlY,
            width: 200,
            height: 300
        },
        imageURL: pathToAssets + "overlay-images/ddao-background-edit-upper-legs.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _jointsControlOverlays.push(_upperLegsJointControl);

    var _lowerLegsJointControl = Overlays.addOverlay("image", {
        bounds: {
            x: _jointsControlX,
            y: _jointsControlY,
            width: 200,
            height: 300
        },
        imageURL: pathToAssets + "overlay-images/ddao-background-edit-lower-legs.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _jointsControlOverlays.push(_lowerLegsJointControl);

    var _feetJointControl = Overlays.addOverlay("image", {
        bounds: {
            x: _jointsControlX,
            y: _jointsControlY,
            width: 200,
            height: 300
        },
        imageURL: pathToAssets + "overlay-images/ddao-background-edit-feet.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _jointsControlOverlays.push(_feetJointControl);

    var _toesJointControl = Overlays.addOverlay("image", {
        bounds: {
            x: _jointsControlX,
            y: _jointsControlY,
            width: 200,
            height: 300
        },
        imageURL: pathToAssets + "overlay-images/ddao-background-edit-toes.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _jointsControlOverlays.push(_toesJointControl);

    var _spineJointControl = Overlays.addOverlay("image", {
        bounds: {
            x: _jointsControlX,
            y: _jointsControlY,
            width: 200,
            height: 300
        },
        imageURL: pathToAssets + "overlay-images/ddao-background-edit-spine.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _jointsControlOverlays.push(_spineJointControl);

    var _spine1JointControl = Overlays.addOverlay("image", {
        bounds: {
            x: _jointsControlX,
            y: _jointsControlY,
            width: 200,
            height: 300
        },
        imageURL: pathToAssets + "overlay-images/ddao-background-edit-spine1.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _jointsControlOverlays.push(_spine1JointControl);

    var _spine2JointControl = Overlays.addOverlay("image", {
        bounds: {
            x: _jointsControlX,
            y: _jointsControlY,
            width: 200,
            height: 300
        },
        imageURL: pathToAssets + "overlay-images/ddao-background-edit-spine2.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _jointsControlOverlays.push(_spine2JointControl);

    var _shouldersJointControl = Overlays.addOverlay("image", {
        bounds: {
            x: _jointsControlX,
            y: _jointsControlY,
            width: 200,
            height: 300
        },
        imageURL: pathToAssets + "overlay-images/ddao-background-edit-shoulders.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _jointsControlOverlays.push(_shouldersJointControl);

    var _upperArmsJointControl = Overlays.addOverlay("image", {
        bounds: {
            x: _jointsControlX,
            y: _jointsControlY,
            width: 200,
            height: 300
        },
        imageURL: pathToAssets + "overlay-images/ddao-background-edit-upper-arms.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _jointsControlOverlays.push(_upperArmsJointControl);

    var _forearmsJointControl = Overlays.addOverlay("image", {
        bounds: {
            x: _jointsControlX,
            y: _jointsControlY,
            width: 200,
            height: 300
        },
        imageURL: pathToAssets + "overlay-images/ddao-background-edit-forearms.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _jointsControlOverlays.push(_forearmsJointControl);

    var _handsJointControl = Overlays.addOverlay("image", {
        bounds: {
            x: _jointsControlX,
            y: _jointsControlY,
            width: 200,
            height: 300
        },
        imageURL: pathToAssets + "overlay-images/ddao-background-edit-hands.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _jointsControlOverlays.push(_handsJointControl);

    var _headJointControl = Overlays.addOverlay("image", {
        bounds: {
            x: _jointsControlX,
            y: _jointsControlY,
            width: 200,
            height: 300
        },
        imageURL: pathToAssets + "overlay-images/ddao-background-edit-head.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _jointsControlOverlays.push(_headJointControl);


    // slider thumb overlays
    var _sliderOne = Overlays.addOverlay("image", {
        bounds: {
            x: 0,
            y: 0,
            width: 25,
            height: 25
        },
        imageURL: pathToAssets + "overlay-images/ddao-slider-handle.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _sliderThumbOverlays.push(_sliderOne);

    var _sliderTwo = Overlays.addOverlay("image", {
        bounds: {
            x: 0,
            y: 0,
            width: 25,
            height: 25
        },
        imageURL: pathToAssets + "overlay-images/ddao-slider-handle.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _sliderThumbOverlays.push(_sliderTwo);

    var _sliderThree = Overlays.addOverlay("image", {
        bounds: {
            x: 0,
            y: 0,
            width: 25,
            height: 25
        },
        imageURL: pathToAssets + "overlay-images/ddao-slider-handle.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _sliderThumbOverlays.push(_sliderThree);

    var _sliderFour = Overlays.addOverlay("image", {
        bounds: {
            x: 0,
            y: 0,
            width: 25,
            height: 25
        },
        imageURL: pathToAssets + "overlay-images/ddao-slider-handle.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _sliderThumbOverlays.push(_sliderFour);

    var _sliderFive = Overlays.addOverlay("image", {
        bounds: {
            x: 0,
            y: 0,
            width: 25,
            height: 25
        },
        imageURL: pathToAssets + "overlay-images/ddao-slider-handle.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _sliderThumbOverlays.push(_sliderFive);

    var _sliderSix = Overlays.addOverlay("image", {
        bounds: {
            x: 0,
            y: 0,
            width: 25,
            height: 25
        },
        imageURL: pathToAssets + "overlay-images/ddao-slider-handle.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _sliderThumbOverlays.push(_sliderSix);

    var _sliderSeven = Overlays.addOverlay("image", {
        bounds: {
            x: 0,
            y: 0,
            width: 25,
            height: 25
        },
        imageURL: pathToAssets + "overlay-images/ddao-slider-handle.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _sliderThumbOverlays.push(_sliderSeven);

    var _sliderEight = Overlays.addOverlay("image", {
        bounds: {
            x: 0,
            y: 0,
            width: 25,
            height: 25
        },
        imageURL: pathToAssets + "overlay-images/ddao-slider-handle.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _sliderThumbOverlays.push(_sliderEight);

    var _sliderNine = Overlays.addOverlay("image", {
        bounds: {
            x: 0,
            y: 0,
            width: 25,
            height: 25
        },
        imageURL: pathToAssets + "overlay-images/ddao-slider-handle.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _sliderThumbOverlays.push(_sliderNine);


    // button overlays
    var _onButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 20,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-on-button.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_onButton);

    var _offButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 20,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-off-button.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_offButton);

    var _configWalkButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 83,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-edit-walk-button.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_configWalkButton);

    var _configWalkButtonSelected = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 83,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-edit-walk-button-selected.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_configWalkButtonSelected);

    var _configStandButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 146,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-edit-stand-button.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_configStandButton);

    var _configStandButtonSelected = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 146,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-edit-stand-button-selected.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_configStandButtonSelected);

    var _configFlyingButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 209,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-edit-fly-button.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_configFlyingButton);

    var _configFlyingButtonSelected = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 209,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-edit-fly-button-selected.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_configFlyingButtonSelected);

    var _configFlyingUpButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 83,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-edit-fly-up-button.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_configFlyingUpButton);

    var _configFlyingUpButtonSelected = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 83,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-edit-fly-up-button-selected.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_configFlyingUpButtonSelected);

    var _configFlyingDownButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 146,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-edit-fly-down-button.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_configFlyingDownButton);

    var _configFlyingDownButtonSelected = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 146,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-edit-fly-down-button-selected.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_configFlyingDownButtonSelected);

    var _hideButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 272,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-hide-button.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_hideButton);

    var _hideButtonSelected = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 272,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-hide-button-selected.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_hideButtonSelected);

    var _configWalkStylesButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 83,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-walk-styles-button.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_configWalkStylesButton);

    var _configWalkStylesButtonSelected = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 83,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-walk-styles-button-selected.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_configWalkStylesButtonSelected);

    var _configWalkTweaksButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 146,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-walk-tweaks-button.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_configWalkTweaksButton);

    var _configWalkTweaksButtonSelected = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 146,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-walk-tweaks-button-selected.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_configWalkTweaksButtonSelected);

    var _configSideStepLeftButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 83,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-edit-sidestep-left-button.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_configSideStepLeftButton);

    var _configSideStepLeftButtonSelected = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 83,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-edit-sidestep-left-button-selected.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_configSideStepLeftButtonSelected);

    var _configSideStepRightButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 209,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-edit-sidestep-right-button.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_configSideStepRightButton);

    var _configSideStepRightButtonSelected = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 209,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-edit-sidestep-right-button-selected.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_configSideStepRightButtonSelected);

    var _configWalkJointsButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 209,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-joints-button.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_configWalkJointsButton);

    var _configWalkJointsButtonSelected = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 209,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-joints-button-selected.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_configWalkJointsButtonSelected);

    var _backButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 272,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-back-button.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_backButton);

    var _backButtonSelected = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + 272,
            y: _backgroundY + _buttonsY,
            width: 60,
            height: 47
        },
        imageURL: pathToAssets + "overlay-images/ddao-back-button-selected.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _buttonOverlays.push(_backButtonSelected);

    // big button overlays - front panel
    var _femaleBigButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + _backgroundWidth / 2 - 115,
            y: _backgroundY + _bigButtonsY,
            width: 230,
            height: 36
        },
        imageURL: pathToAssets + "overlay-images/ddao-female-big-button.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _bigbuttonOverlays.push(_femaleBigButton);

    var _femaleBigButtonSelected = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + _backgroundWidth / 2 - 115,
            y: _backgroundY + _bigButtonsY,
            width: 230,
            height: 36
        },
        imageURL: pathToAssets + "overlay-images/ddao-female-big-button-selected.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _bigbuttonOverlays.push(_femaleBigButtonSelected);

    var _maleBigButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + _backgroundWidth / 2 - 115,
            y: _backgroundY + _bigButtonsY + 60,
            width: 230,
            height: 36
        },
        imageURL: pathToAssets + "overlay-images/ddao-male-big-button.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _bigbuttonOverlays.push(_maleBigButton);

    var _maleBigButtonSelected = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + _backgroundWidth / 2 - 115,
            y: _backgroundY + _bigButtonsY + 60,
            width: 230,
            height: 36
        },
        imageURL: pathToAssets + "overlay-images/ddao-male-big-button-selected.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _bigbuttonOverlays.push(_maleBigButtonSelected);

    var _armsFreeBigButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + _backgroundWidth / 2 - 115,
            y: _backgroundY + _bigButtonsY + 120,
            width: 230,
            height: 36
        },
        imageURL: pathToAssets + "overlay-images/ddao-arms-free-button.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _bigbuttonOverlays.push(_armsFreeBigButton);

    var _armsFreeBigButtonSelected = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + _backgroundWidth / 2 - 115,
            y: _backgroundY + _bigButtonsY + 120,
            width: 230,
            height: 36
        },
        imageURL: pathToAssets + "overlay-images/ddao-arms-free-button-selected.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _bigbuttonOverlays.push(_armsFreeBigButtonSelected);

    var _footstepsBigButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + _backgroundWidth / 2 - 115,
            y: _backgroundY + _bigButtonsY + 180,
            width: 230,
            height: 36
        },
        imageURL: pathToAssets + "overlay-images/ddao-footsteps-big-button.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _bigbuttonOverlays.push(_footstepsBigButton);

    var _footstepsBigButtonSelected = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + _backgroundWidth / 2 - 115,
            y: _backgroundY + _bigButtonsY + 180,
            width: 230,
            height: 36
        },
        imageURL: pathToAssets + "overlay-images/ddao-footsteps-big-button-selected.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _bigbuttonOverlays.push(_footstepsBigButtonSelected);


    // walk styles
    _bigButtonsY = 121;
    var _standardWalkBigButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + _backgroundWidth / 2 - 115,
            y: _backgroundY + _bigButtonsY,
            width: 230,
            height: 36
        },
        imageURL: pathToAssets + "overlay-images/ddao-walk-select-button-standard.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _bigbuttonOverlays.push(_standardWalkBigButton);

    var _standardWalkBigButtonSelected = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + _backgroundWidth / 2 - 115,
            y: _backgroundY + _bigButtonsY,
            width: 230,
            height: 36
        },
        imageURL: pathToAssets + "overlay-images/ddao-walk-select-button-standard-selected.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: false
    });
    _bigbuttonOverlays.push(_standardWalkBigButtonSelected);

    // various show / hide GUI element functions
    function minimiseDialog(minimise) {

        if (momentaryButtonTimer) {
            Script.clearInterval(momentaryButtonTimer);
            momentaryButtonTimer = null;
        }

        if (minimise) {
            setBackground();
            hideMenuButtons();
            setSliderThumbsVisible(false);
            hideJointSelectors();
            initialiseFrontPanel(false);
            Overlays.editOverlay(_controlsMinimisedTab, {
                visible: true
            });
        } else {
            Overlays.editOverlay(_controlsMinimisedTab, {
                visible: false
            });
        }
    };

    function setBackground(backgroundID) {
        for (var i in _backgroundOverlays) {
            if (_backgroundOverlays[i] === backgroundID) {
                Overlays.editOverlay(_backgroundOverlays[i], {
                    visible: true
                });
			} else {
				Overlays.editOverlay(_backgroundOverlays[i], { visible: false });
			}
        }
    };

    // top row menu type buttons (on | walk | stand | fly | hide)
    function hideMenuButtons() {
        for (var i in _buttonOverlays) {
            Overlays.editOverlay(_buttonOverlays[i], { visible: false });
        }
    };

    function hideJointSelectors() {
        for (var i in _jointsControlOverlays) {
            Overlays.editOverlay(_jointsControlOverlays[i], {
                visible: false
            });
        }
    };

    function setSliderThumbsVisible(thumbsVisible) {
        for (var i = 0; i < _sliderThumbOverlays.length; i++) {
            Overlays.editOverlay(_sliderThumbOverlays[i], {
                visible: thumbsVisible
            });
        }
    };

    function setButtonOverlayVisible(buttonOverlayName) {
        for (var i in _buttonOverlays) {
            if (_buttonOverlays[i] === buttonOverlayName) {
                Overlays.editOverlay(buttonOverlayName, { visible: true });
            }
        }
    };

    function initialiseFrontPanel(showButtons) {

        if (_motion.avatarGender === FEMALE) {
            Overlays.editOverlay(_femaleBigButtonSelected, {
                visible: showButtons
            });
            Overlays.editOverlay(_femaleBigButton, {
                visible: false
            });
            Overlays.editOverlay(_maleBigButtonSelected, {
                visible: false
            });
            Overlays.editOverlay(_maleBigButton, {
                visible: showButtons
            });

        } else {

            Overlays.editOverlay(_femaleBigButtonSelected, {
                visible: false
            });
            Overlays.editOverlay(_femaleBigButton, {
                visible: showButtons
            });
            Overlays.editOverlay(_maleBigButtonSelected, {
                visible: showButtons
            });
            Overlays.editOverlay(_maleBigButton, {
                visible: false
            });
        }
        if (_motion.armsFree) {
            Overlays.editOverlay(_armsFreeBigButtonSelected, {
                visible: showButtons
            });
            Overlays.editOverlay(_armsFreeBigButton, {
                visible: false
            });

        } else {

            Overlays.editOverlay(_armsFreeBigButtonSelected, {
                visible: false
            });
            Overlays.editOverlay(_armsFreeBigButton, {
                visible: showButtons
            });
        }
        if (_motion.makesFootStepSounds) {
            Overlays.editOverlay(_footstepsBigButtonSelected, {
                visible: showButtons
            });
            Overlays.editOverlay(_footstepsBigButton, {
                visible: false
            });

        } else {

            Overlays.editOverlay(_footstepsBigButtonSelected, {
                visible: false
            });
            Overlays.editOverlay(_footstepsBigButton, {
                visible: showButtons
            });
        }
    };

    function initialiseWalkStylesPanel(showButtons) {

        // set all big buttons to hidden, but skip the first 8, as are used by the front panel
        for (var i = 8; i < _bigbuttonOverlays.length; i++) {
            Overlays.editOverlay(_bigbuttonOverlays[i], {
                visible: false
            });
        }

        if (!showButtons) {
			return;
		}

        // set all the non-selected ones to showing
        for (var i = 8; i < _bigbuttonOverlays.length; i += 2) {
            Overlays.editOverlay(_bigbuttonOverlays[i], { visible: true });
        }

        // set the currently selected one
        if (_motion.selWalk === _walkAssets.femaleStandardWalk ||
            _motion.selWalk === _walkAssets.maleStandardWalk) {

            Overlays.editOverlay(_standardWalkBigButtonSelected, {
                visible: true
            });
            Overlays.editOverlay(_standardWalkBigButton, {
                visible: false
            });
        }
    };

    function initialiseWalkTweaksPanel() {

        // sliders for commonly required walk adjustments
        var i = 0;
        var yLocation = _backgroundY + 71;

        // walk speed
        var sliderXPos = _motion.curAnim.calibration.frequency / MAX_WALK_SPEED * _sliderRangeX;
        Overlays.editOverlay(_sliderThumbOverlays[i], {
            x: _minSliderX + sliderXPos,
            y: yLocation += 60,
            visible: true
        });

        // lean (hips pitch offset)
        sliderXPos = (((_sliderRanges.joints[0].pitchOffsetRange + _motion.curAnim.joints[0].pitchOffset) / 2) /
            _sliderRanges.joints[0].pitchOffsetRange) * _sliderRangeX;
        Overlays.editOverlay(_sliderThumbOverlays[++i], {
            x: _minSliderX + sliderXPos,
            y: yLocation += 60,
            visible: true
        });

        // stride (upper legs pitch)
        sliderXPos = _motion.curAnim.joints[1].pitch / _sliderRanges.joints[1].pitchRange * _sliderRangeX;
        Overlays.editOverlay(_sliderThumbOverlays[++i], {
            x: _minSliderX + sliderXPos,
            y: yLocation += 60,
            visible: true
        });

        // Legs separation (upper legs roll offset)
        sliderXPos = (((_sliderRanges.joints[1].rollOffsetRange + _motion.curAnim.joints[1].rollOffset) / 2) /
            _sliderRanges.joints[1].rollOffsetRange) * _sliderRangeX;
        Overlays.editOverlay(_sliderThumbOverlays[++i], {
            x: _minSliderX + sliderXPos,
            y: yLocation += 60,
            visible: true
        });

        // Legs forward (upper legs pitch offset)
        sliderXPos = (((_sliderRanges.joints[1].pitchOffsetRange + _motion.curAnim.joints[1].pitchOffset) / 2) /
            _sliderRanges.joints[1].pitchOffsetRange) * _sliderRangeX;
        Overlays.editOverlay(_sliderThumbOverlays[++i], {
            x: _minSliderX + sliderXPos,
            y: yLocation += 60,
            visible: true
        });

        // Lower legs splay (lower legs roll offset)
        sliderXPos = (((_sliderRanges.joints[2].rollOffsetRange + _motion.curAnim.joints[2].rollOffset) / 2) /
            _sliderRanges.joints[2].rollOffsetRange) * _sliderRangeX;
        Overlays.editOverlay(_sliderThumbOverlays[++i], {
            x: _minSliderX + sliderXPos,
            y: yLocation += 60,
            visible: true
        });

        // Arms forward (upper arms yaw offset)
        sliderXPos = (((_sliderRanges.joints[9].yawOffsetRange + _motion.curAnim.joints[9].yawOffset) / 2) /
            _sliderRanges.joints[9].yawOffsetRange) * _sliderRangeX;
        Overlays.editOverlay(_sliderThumbOverlays[++i], {
            x: _minSliderX + sliderXPos,
            y: yLocation += 60,
            visible: true
        });

        // Arms out (upper arm pitch offset)
        sliderXPos = (((_sliderRanges.joints[9].pitchOffsetRange - _motion.curAnim.joints[9].pitchOffset) / 2) /
            _sliderRanges.joints[9].pitchOffsetRange) * _sliderRangeX;
        Overlays.editOverlay(_sliderThumbOverlays[++i], {
            x: _minSliderX + sliderXPos,
            y: yLocation += 60,
            visible: true
        });

        // Lower arms splay (lower arm pitch offset)
        sliderXPos = (((_sliderRanges.joints[10].pitchOffsetRange - _motion.curAnim.joints[10].pitchOffset) / 2) /
            _sliderRanges.joints[10].pitchOffsetRange) * _sliderRangeX;
        Overlays.editOverlay(_sliderThumbOverlays[++i], {
            x: _minSliderX + sliderXPos,
            y: yLocation += 60,
            visible: true
        });
    };

    function initialiseJointsEditingPanel() {

        var i = 0;
        var yLocation = _backgroundY + 359;
        hideJointSelectors();

        if (_state.editingTranslation) {

            // display the joint control selector for hips translations
            Overlays.editOverlay(_hipsJointsTranslation, {visible: true});

            // Hips sway
            var sliderXPos = _motion.curAnim.joints[0].sway / _sliderRanges.joints[0].swayRange * _sliderRangeX;
            Overlays.editOverlay(_sliderThumbOverlays[i], {
                x: _minSliderX + sliderXPos,
                y: yLocation += 30,
                visible: true
            });

            // Hips bob
            sliderXPos = _motion.curAnim.joints[0].bob / _sliderRanges.joints[0].bobRange * _sliderRangeX;
            Overlays.editOverlay(_sliderThumbOverlays[++i], {
                x: _minSliderX + sliderXPos,
                y: yLocation += 30,
                visible: true
            });

            // Hips thrust
            sliderXPos = _motion.curAnim.joints[0].thrust / _sliderRanges.joints[0].thrustRange * _sliderRangeX;
            Overlays.editOverlay(_sliderThumbOverlays[++i], {
                x: _minSliderX + sliderXPos,
                y: yLocation += 30,
                visible: true
            });

            // Sway Phase
            sliderXPos = (90 + _motion.curAnim.joints[0].swayPhase / 2) / 180 * _sliderRangeX;
            Overlays.editOverlay(_sliderThumbOverlays[++i], {
                x: _minSliderX + sliderXPos,
                y: yLocation += 30,
                visible: true
            });

            // Bob Phase
            sliderXPos = (90 + _motion.curAnim.joints[0].bobPhase / 2) / 180 * _sliderRangeX;
            Overlays.editOverlay(_sliderThumbOverlays[++i], {
                x: _minSliderX + sliderXPos,
                y: yLocation += 30,
                visible: true
            });

            // Thrust Phase
            sliderXPos = (90 + _motion.curAnim.joints[0].thrustPhase / 2) / 180 * _sliderRangeX;
            Overlays.editOverlay(_sliderThumbOverlays[++i], {
                x: _minSliderX + sliderXPos,
                y: yLocation += 30,
                visible: true
            });

            // offset ranges are also -ve thr' zero to +ve, so we centre them
            sliderXPos = (((_sliderRanges.joints[0].swayOffsetRange + _motion.curAnim.joints[0]
                .swayOffset) / 2) / _sliderRanges.joints[0].swayOffsetRange) * _sliderRangeX;
            Overlays.editOverlay(_sliderThumbOverlays[++i], {
                x: _minSliderX + sliderXPos,
                y: yLocation += 30,
                visible: true
            });

            sliderXPos = (((_sliderRanges.joints[0].bobOffsetRange + _motion.curAnim.joints[0]
                .bobOffset) / 2) / _sliderRanges.joints[0].bobOffsetRange) * _sliderRangeX;
            Overlays.editOverlay(_sliderThumbOverlays[++i], {
                x: _minSliderX + sliderXPos,
                y: yLocation += 30,
                visible: true
            });

            sliderXPos = (((_sliderRanges.joints[0].thrustOffsetRange + _motion.curAnim.joints[0]
                .thrustOffset) / 2) / _sliderRanges.joints[0].thrustOffsetRange) * _sliderRangeX;
            Overlays.editOverlay(_sliderThumbOverlays[++i], {
                x: _minSliderX + sliderXPos,
                y: yLocation += 30,
                visible: true
            });

        } else {

            switch (_motion.curJointIndex) {

                case 0:
                    Overlays.editOverlay(_hipsJointControl, {
                        visible: true
                    });
                    break;
                case 1:
                    Overlays.editOverlay(_upperLegsJointControl, {
                        visible: true
                    });
                    break;
                case 2:
                    Overlays.editOverlay(_lowerLegsJointControl, {
                        visible: true
                    });
                    break;
                case 3:
                    Overlays.editOverlay(_feetJointControl, {
                        visible: true
                    });
                    break;
                case 4:
                    Overlays.editOverlay(_toesJointControl, {
                        visible: true
                    });
                    break;
                case 5:
                    Overlays.editOverlay(_spineJointControl, {
                        visible: true
                    });
                    break;
                case 6:
                    Overlays.editOverlay(_spine1JointControl, {
                        visible: true
                    });
                    break;
                case 7:
                    Overlays.editOverlay(_spine2JointControl, {
                        visible: true
                    });
                    break;
                case 8:
                    Overlays.editOverlay(_shouldersJointControl, {
                        visible: true
                    });
                    break;
                case 9:
                    Overlays.editOverlay(_upperArmsJointControl, {
                        visible: true
                    });
                    break;
                case 10:
                    Overlays.editOverlay(_forearmsJointControl, {
                        visible: true
                    });
                    break;
                case 11:
                    Overlays.editOverlay(_handsJointControl, {
                        visible: true
                    });
                    break;
                case 12:
                    Overlays.editOverlay(_headJointControl, {
                        visible: true
                    });
                    break;
            }

            var sliderXPos = _motion.curAnim.joints[_motion.curJointIndex].pitch /
                _sliderRanges.joints[_motion.curJointIndex].pitchRange * _sliderRangeX;
            Overlays.editOverlay(_sliderThumbOverlays[i], {
                x: _minSliderX + sliderXPos,
                y: yLocation += 30,
                visible: true
            });

            sliderXPos = _motion.curAnim.joints[_motion.curJointIndex].yaw /
                _sliderRanges.joints[_motion.curJointIndex].yawRange * _sliderRangeX;
            Overlays.editOverlay(_sliderThumbOverlays[++i], {
                x: _minSliderX + sliderXPos,
                y: yLocation += 30,
                visible: true
            });

            sliderXPos = _motion.curAnim.joints[_motion.curJointIndex].roll /
                _sliderRanges.joints[_motion.curJointIndex].rollRange * _sliderRangeX;
            Overlays.editOverlay(_sliderThumbOverlays[++i], {
                x: _minSliderX + sliderXPos,
                y: yLocation += 30,
                visible: true
            });

            // set phases (full range, -180 to 180)
            sliderXPos = (90 + _motion.curAnim.joints[_motion.curJointIndex].pitchPhase / 2) / 180 * _sliderRangeX;
            Overlays.editOverlay(_sliderThumbOverlays[++i], {
                x: _minSliderX + sliderXPos,
                y: yLocation += 30,
                visible: true
            });

            sliderXPos = (90 + _motion.curAnim.joints[_motion.curJointIndex].yawPhase / 2) / 180 * _sliderRangeX;
            Overlays.editOverlay(_sliderThumbOverlays[++i], {
                x: _minSliderX + sliderXPos,
                y: yLocation += 30,
                visible: true
            });

            sliderXPos = (90 + _motion.curAnim.joints[_motion.curJointIndex].rollPhase / 2) / 180 * _sliderRangeX;
            Overlays.editOverlay(_sliderThumbOverlays[++i], {
                x: _minSliderX + sliderXPos,
                y: yLocation += 30,
                visible: true
            });

            // offset ranges are also -ve thr' zero to +ve, so we offset
            sliderXPos = (((_sliderRanges.joints[_motion.curJointIndex].pitchOffsetRange +
                _motion.curAnim.joints[_motion.curJointIndex].pitchOffset) / 2) /
                _sliderRanges.joints[_motion.curJointIndex].pitchOffsetRange) * _sliderRangeX;
            Overlays.editOverlay(_sliderThumbOverlays[++i], {
                x: _minSliderX + sliderXPos,
                y: yLocation += 30,
                visible: true
            });

            sliderXPos = (((_sliderRanges.joints[_motion.curJointIndex].yawOffsetRange +
                    _motion.curAnim.joints[_motion.curJointIndex].yawOffset) / 2) /
                    _sliderRanges.joints[_motion.curJointIndex].yawOffsetRange) * _sliderRangeX;
            Overlays.editOverlay(_sliderThumbOverlays[++i], {
                x: _minSliderX + sliderXPos,
                y: yLocation += 30,
                visible: true
            });

            sliderXPos = (((_sliderRanges.joints[_motion.curJointIndex].rollOffsetRange +
                    _motion.curAnim.joints[_motion.curJointIndex].rollOffset) / 2) /
                    _sliderRanges.joints[_motion.curJointIndex].rollOffsetRange) * _sliderRangeX;
            Overlays.editOverlay(_sliderThumbOverlays[++i], {
                x: _minSliderX + sliderXPos,
                y: yLocation += 30,
                visible: true
            });
        }
    };

    // mouse event handlers
    var _movingSliderOne = false;
    var _movingSliderTwo = false;
    var _movingSliderThree = false;
    var _movingSliderFour = false;
    var _movingSliderFive = false;
    var _movingSliderSix = false;
    var _movingSliderSeven = false;
    var _movingSliderEight = false;
    var _movingSliderNine = false;

    function mousePressEvent(event) {

        var clickedOverlay = Overlays.getOverlayAtPoint({
            x: event.x,
            y: event.y
        });

        if (_state.currentState === _state.EDIT_WALK_JOINTS ||
            _state.currentState === _state.EDIT_STANDING ||
            _state.currentState === _state.EDIT_FLYING ||
            _state.currentState === _state.EDIT_FLYING_UP ||
            _state.currentState === _state.EDIT_FLYING_DOWN ||
            _state.currentState === _state.EDIT_SIDESTEP_LEFT ||
            _state.currentState === _state.EDIT_SIDESTEP_RIGHT) {

            // check for new joint selection and update display accordingly
            var clickX = event.x - _backgroundX - 75;
            var clickY = event.y - _backgroundY - 92;

            if (clickX > 60 && clickX < 120 && clickY > 123 && clickY < 155) {
                _motion.curJointIndex = 0;
                initialiseJointsEditingPanel();
                return;

            } else if (clickX > 63 && clickX < 132 && clickY > 156 && clickY < 202) {
                _motion.curJointIndex = 1;
                initialiseJointsEditingPanel();
                return;

            } else if (clickX > 58 && clickX < 137 && clickY > 203 && clickY < 250) {
                _motion.curJointIndex = 2;
                initialiseJointsEditingPanel();
                return;

            } else if (clickX > 58 && clickX < 137 && clickY > 250 && clickY < 265) {
                _motion.curJointIndex = 3;
                initialiseJointsEditingPanel();
                return;

            } else if (clickX > 58 && clickX < 137 && clickY > 265 && clickY < 280) {
                _motion.curJointIndex = 4;
                initialiseJointsEditingPanel();
                return;

            } else if (clickX > 78 && clickX < 121 && clickY > 111 && clickY < 128) {
                _motion.curJointIndex = 5;
                initialiseJointsEditingPanel();
                return;

            } else if (clickX > 78 && clickX < 128 && clickY > 89 && clickY < 111) {
                _motion.curJointIndex = 6;
                initialiseJointsEditingPanel();
                return;

            } else if (clickX > 85 && clickX < 118 && clickY > 77 && clickY < 94) {
                _motion.curJointIndex = 7;
                initialiseJointsEditingPanel();
                return;

            } else if (clickX > 64 && clickX < 125 && clickY > 55 && clickY < 77) {
                _motion.curJointIndex = 8;
                initialiseJointsEditingPanel();
                return;

            } else if ((clickX > 44 && clickX < 73 && clickY > 71 && clickY < 94) ||
                       (clickX > 125 && clickX < 144 && clickY > 71 && clickY < 94)) {
                _motion.curJointIndex = 9;
                initialiseJointsEditingPanel();
                return;

            } else if ((clickX > 28 && clickX < 57 && clickY > 94 && clickY < 119) ||
                       (clickX > 137 && clickX < 170 && clickY > 97 && clickY < 114)) {
                _motion.curJointIndex = 10;
                initialiseJointsEditingPanel();
                return;

            } else if ((clickX > 18 && clickX < 37 && clickY > 115 && clickY < 136) ||
                       (clickX > 157 && clickX < 182 && clickY > 115 && clickY < 136)) {
                _motion.curJointIndex = 11;
                initialiseJointsEditingPanel();
                return;

            } else if (clickX > 81 && clickX < 116 && clickY > 12 && clickY < 53) {
                _motion.curJointIndex = 12;
                initialiseJointsEditingPanel();
                return;

            } else if (clickX > 188 && clickX < 233 && clickY > 6 && clickY < 34) {

                // translation editing radio selection
                if (_state.editingTranslation) {

                    hideJointSelectors();
                    setBackground(_controlsBackgroundWalkEditJoints);
                    _state.editingTranslation = false;

                } else {

                    hideJointSelectors();
                    setBackground(_controlsBackgroundWalkEditHipTrans);
                    _state.editingTranslation = true;
                }
                initialiseJointsEditingPanel();
                return;
            }
        }

        switch (clickedOverlay) {

            case _offButton:

                _state.powerOn = true;
                Overlays.editOverlay(_offButton, {
                    visible: false
                });
                Overlays.editOverlay(_onButton, {
                    visible: true
                });
                _state.setInternalState(state.STANDING);
                return;

            case _controlsMinimisedTab:

                _state.minimised = false;
                minimiseDialog(_state.minimised);
                _state.setInternalState(_state.STANDING);
                return;

            case _hideButton:
            case _hideButtonSelected:

                    Overlays.editOverlay(_hideButton, {
                        visible: false
                    });
                Overlays.editOverlay(_hideButtonSelected, {
                    visible: true
                });
                _state.minimised = true;
                momentaryButtonTimer = Script.setInterval(function() {
                    minimiseDialog(_state.minimised);
                }, 80);
                return;

            case _backButton:

                Overlays.editOverlay(_backButton, {
                    visible: false
                });
                Overlays.editOverlay(_backButtonSelected, {
                    visible: true
                });
                momentaryButtonTimer = Script.setInterval(function() {

                    _state.setInternalState(_state.STANDING);
                    Overlays.editOverlay(_backButton, {
                        visible: false
                    });
                    Overlays.editOverlay(_backButtonSelected, {
                        visible: false
                    });
                    Script.clearInterval(momentaryButtonTimer);
                    momentaryButtonTimer = null;
                }, 80);
                return;

            case _footstepsBigButton:

                _motion.makesFootStepSounds = true;
                Overlays.editOverlay(_footstepsBigButtonSelected, {
                    visible: true
                });
                Overlays.editOverlay(_footstepsBigButton, {
                    visible: false
                });
                return;

            case _footstepsBigButtonSelected:

                _motion.makesFootStepSounds = false;
                Overlays.editOverlay(_footstepsBigButton, {
                    visible: true
                });
                Overlays.editOverlay(_footstepsBigButtonSelected, {
                    visible: false
                });
                return;

            case _femaleBigButton:
            case _maleBigButtonSelected:

                _motion.setGender(FEMALE);

                Overlays.editOverlay(_femaleBigButtonSelected, {
                    visible: true
                });
                Overlays.editOverlay(_femaleBigButton, {
                    visible: false
                });
                Overlays.editOverlay(_maleBigButton, {
                    visible: true
                });
                Overlays.editOverlay(_maleBigButtonSelected, {
                    visible: false
                });
                return;

            case _maleBigButton:
            case _femaleBigButtonSelected:

                _motion.setGender(MALE);

                Overlays.editOverlay(_femaleBigButton, {
                    visible: true
                });
                Overlays.editOverlay(_femaleBigButtonSelected, {
                    visible: false
                });
                Overlays.editOverlay(_maleBigButtonSelected, {
                    visible: true
                });
                Overlays.editOverlay(_maleBigButton, {
                    visible: false
                });
                return;

            case _armsFreeBigButton:

                _motion.armsFree = true;

                Overlays.editOverlay(_armsFreeBigButtonSelected, {
                    visible: true
                });
                Overlays.editOverlay(_armsFreeBigButton, {
                    visible: false
                });
                return;

            case _armsFreeBigButtonSelected:

                _motion.armsFree = false;

                Overlays.editOverlay(_armsFreeBigButtonSelected, {
                    visible: false
                });
                Overlays.editOverlay(_armsFreeBigButton, {
                    visible: true
                });
                return;

            case _standardWalkBigButton:

                if (_motion.avatarGender === FEMALE) {
					_motion.selWalk = _motion.femaleStandardWalk;
				} else {
					_motion.selWalk = _motion.maleStandardWalk;
				}
                _motion.curAnim = _motion.selWalk;
                initialiseWalkStylesPanel(true);
                return;

            case _standardWalkBigButtonSelected:

                // toggle forwards / backwards walk display
                if (_motion.direction === FORWARDS) {
                    _motion.direction = BACKWARDS;
                } else {
					_motion.direction = FORWARDS;
				}
                return;

            case _sliderOne:

                _movingSliderOne = true;
                return;

            case _sliderTwo:

                _movingSliderTwo = true;
                return;

            case _sliderThree:

                _movingSliderThree = true;
                return;

            case _sliderFour:

                _movingSliderFour = true;
                return;

            case _sliderFive:

                _movingSliderFive = true;
                return;

            case _sliderSix:

                _movingSliderSix = true;
                return;

            case _sliderSeven:

                _movingSliderSeven = true;
                return;

            case _sliderEight:

                _movingSliderEight = true;
                return;

            case _sliderNine:

                _movingSliderNine = true;
                return;

            case _configWalkButtonSelected:
            case _configStandButtonSelected:
            case _configSideStepLeftButtonSelected:
            case _configSideStepRightButtonSelected:
            case _configFlyingButtonSelected:
            case _configFlyingUpButtonSelected:
            case _configFlyingDownButtonSelected:
            case _configWalkStylesButtonSelected:
            case _configWalkTweaksButtonSelected:
            case _configWalkJointsButtonSelected:

                // exit edit modes
                _motion.curAnim = _motion.selStand;
                _state.setInternalState(_state.STANDING);
                break;

            case _onButton:

                _state.powerOn = false;
                _state.setInternalState(state.STANDING);
                Overlays.editOverlay(_offButton, {
                    visible: true
                });
                Overlays.editOverlay(_onButton, {
                    visible: false
                });
                //resetJoints();
                break;

            case _backButton:
            case _backButtonSelected:

                Overlays.editOverlay(_backButton, {
                    visible: false
                });
                Overlays.editOverlay(_backButtonSelected, {
                    visible: false
                });
                _state.setInternalState(_state.STANDING);
                break;

            case _configWalkStylesButton:

                _state.setInternalState(_state.EDIT_WALK_STYLES);
                break;

            case _configWalkTweaksButton:

                _state.setInternalState(_state.EDIT_WALK_TWEAKS);
                break;

            case _configWalkJointsButton:

                _state.setInternalState(_state.EDIT_WALK_JOINTS);
                break;

            case _configWalkButton:

                _state.setInternalState(_state.EDIT_WALK_STYLES);
                break;

            case _configStandButton:

                _state.setInternalState(_state.EDIT_STANDING);
                break;

            case _configSideStepLeftButton:
                _state.setInternalState(_state.EDIT_SIDESTEP_LEFT);

                break;

            case _configSideStepRightButton:

                _state.setInternalState(_state.EDIT_SIDESTEP_RIGHT);
                break;

            case _configFlyingButton:

                _state.setInternalState(_state.EDIT_FLYING);
                break;

            case _configFlyingUpButton:

                _state.setInternalState(_state.EDIT_FLYING_UP);
                break;

            case _configFlyingDownButton:

                _state.setInternalState(_state.EDIT_FLYING_DOWN);
                break;
        }
    };

    function mouseMoveEvent(event) {

        // workaround for bug (https://worklist.net/20160)
        if ((event.x > 310 && event.x < 318 && event.y > 1350 && event.y < 1355) ||
           (event.x > 423 && event.x < 428 && event.y > 1505 && event.y < 1508 )) {
			   return;
		}

        if (_state.currentState === _state.EDIT_WALK_JOINTS ||
            _state.currentState === _state.EDIT_STANDING ||
            _state.currentState === _state.EDIT_FLYING ||
            _state.currentState === _state.EDIT_FLYING_UP ||
            _state.currentState === _state.EDIT_FLYING_DOWN ||
            _state.currentState === _state.EDIT_SIDESTEP_LEFT ||
            _state.currentState === _state.EDIT_SIDESTEP_RIGHT) {

            var thumbClickOffsetX = event.x - _minSliderX;
            var thumbPositionNormalised = thumbClickOffsetX / _sliderRangeX;
            if (thumbPositionNormalised < 0) {
				thumbPositionNormalised = 0;
			} else if (thumbPositionNormalised > 1) {
				thumbPositionNormalised = 1;
			}
            var sliderX = thumbPositionNormalised * _sliderRangeX; // sets range

            if (_movingSliderOne) {

                // currently selected joint pitch or sway
                Overlays.editOverlay(_sliderOne, {
                    x: sliderX + _minSliderX
                });
                if (_state.editingTranslation) {
                    _motion.curAnim.joints[0].sway =
                        thumbPositionNormalised * _sliderRanges.joints[0].swayRange;
                } else {
                    _motion.curAnim.joints[_motion.curJointIndex].pitch =
                        thumbPositionNormalised * _sliderRanges.joints[_motion.curJointIndex].pitchRange;
                }

            } else if (_movingSliderTwo) {

                // currently selected joint yaw or bob
                Overlays.editOverlay(_sliderTwo, {
                    x: sliderX + _minSliderX
                });
                if (_state.editingTranslation) {
                    _motion.curAnim.joints[0].bob =
                        thumbPositionNormalised * _sliderRanges.joints[0].bobRange;
                } else {
                    _motion.curAnim.joints[_motion.curJointIndex].yaw =
                        thumbPositionNormalised * _sliderRanges.joints[_motion.curJointIndex].yawRange;
                }

            } else if (_movingSliderThree) {

                // currently selected joint roll or thrust
                Overlays.editOverlay(_sliderThree, {
                    x: sliderX + _minSliderX
                });
                if (_state.editingTranslation) {
                    _motion.curAnim.joints[0].thrust =
                        thumbPositionNormalised * _sliderRanges.joints[0].thrustRange;
                } else {
                    _motion.curAnim.joints[_motion.curJointIndex].roll =
                        thumbPositionNormalised * _sliderRanges.joints[_motion.curJointIndex].rollRange;
                }

            } else if (_movingSliderFour) {

                // currently selected joint pitch phase
                Overlays.editOverlay(_sliderFour, {
                    x: sliderX + _minSliderX
                });

                var newPhase = 360 * thumbPositionNormalised - 180;

                if (_state.editingTranslation) {
                    _motion.curAnim.joints[0].swayPhase = newPhase;
                } else {
                    _motion.curAnim.joints[_motion.curJointIndex].pitchPhase = newPhase;
                }

            } else if (_movingSliderFive) {

                // currently selected joint yaw phase;
                Overlays.editOverlay(_sliderFive, {
                    x: sliderX + _minSliderX
                });

                var newPhase = 360 * thumbPositionNormalised - 180;

                if (_state.editingTranslation) {
                    _motion.curAnim.joints[0].bobPhase = newPhase;
                } else {
                    _motion.curAnim.joints[_motion.curJointIndex].yawPhase = newPhase;
                }

            } else if (_movingSliderSix) {

                // currently selected joint roll phase
                Overlays.editOverlay(_sliderSix, {
                    x: sliderX + _minSliderX
                });

                var newPhase = 360 * thumbPositionNormalised - 180;

                if (_state.editingTranslation) {
                    _motion.curAnim.joints[0].thrustPhase = newPhase;
                } else {
                    _motion.curAnim.joints[_motion.curJointIndex].rollPhase = newPhase;
                }

            } else if (_movingSliderSeven) {

                // currently selected joint pitch offset
                Overlays.editOverlay(_sliderSeven, {
                    x: sliderX + _minSliderX
                });
                if (_state.editingTranslation) {
                    var newOffset = (thumbPositionNormalised - 0.5) *
                                     2 * _sliderRanges.joints[0].swayOffsetRange;
                    _motion.curAnim.joints[0].swayOffset = newOffset;
                } else {
                    var newOffset = (thumbPositionNormalised - 0.5) *
                                     2 * _sliderRanges.joints[_motion.curJointIndex].pitchOffsetRange;
                    _motion.curAnim.joints[_motion.curJointIndex].pitchOffset = newOffset;
                }

            } else if (_movingSliderEight) {

                // currently selected joint yaw offset
                Overlays.editOverlay(_sliderEight, {
                    x: sliderX + _minSliderX
                });
                if (_state.editingTranslation) {
                    var newOffset = (thumbPositionNormalised - 0.5) *
                                     2 *_sliderRanges.joints[0].bobOffsetRange;
                    _motion.curAnim.joints[0].bobOffset = newOffset;
                } else {
                    var newOffset = (thumbPositionNormalised - 0.5) *
                                     2 * _sliderRanges.joints[_motion.curJointIndex].yawOffsetRange;
                    _motion.curAnim.joints[_motion.curJointIndex].yawOffset = newOffset;
                }

            } else if (_movingSliderNine) {

                // currently selected joint roll offset
                Overlays.editOverlay(_sliderNine, {
                    x: sliderX + _minSliderX
                });
                if (_state.editingTranslation) {
                    var newOffset = (thumbPositionNormalised - 0.5) *
                                     2 * _sliderRanges.joints[0].thrustOffsetRange;
                    _motion.curAnim.joints[0].thrustOffset = newOffset;
                } else {
                    var newOffset = (thumbPositionNormalised - 0.5) *
                                     2 * _sliderRanges.joints[_motion.curJointIndex].rollOffsetRange;
                    _motion.curAnim.joints[_motion.curJointIndex].rollOffset = newOffset;
                }
            }

		// end if editing joints

        } else if (_state.currentState === _state.EDIT_WALK_TWEAKS) {

            // sliders for commonly required walk adjustments
            var thumbClickOffsetX = event.x - _minSliderX;
            var thumbPositionNormalised = thumbClickOffsetX / _sliderRangeX;
            if (thumbPositionNormalised < 0) thumbPositionNormalised = 0;
            if (thumbPositionNormalised > 1) thumbPositionNormalised = 1;
            var sliderX = thumbPositionNormalised * _sliderRangeX; // sets range

            if (_movingSliderOne) {
                // walk speed
                Overlays.editOverlay(_sliderOne, {
                    x: sliderX + _minSliderX
                });
                _motion.curAnim.calibration.frequency = thumbPositionNormalised * MAX_WALK_SPEED;
            } else if (_movingSliderTwo) {
                // lean (hips pitch offset)
                Overlays.editOverlay(_sliderTwo, {
                    x: sliderX + _minSliderX
                });
                var newOffset = (thumbPositionNormalised - 0.5) * 2 * _sliderRanges.joints[0].pitchOffsetRange;
                _motion.curAnim.joints[0].pitchOffset = newOffset;
            } else if (_movingSliderThree) {
                // stride (upper legs pitch)
                Overlays.editOverlay(_sliderThree, {
                    x: sliderX + _minSliderX
                });
                _motion.curAnim.joints[1].pitch = thumbPositionNormalised * _sliderRanges.joints[1].pitchRange;
            } else if (_movingSliderFour) {
                // legs separation (upper legs roll)
                Overlays.editOverlay(_sliderFour, {
                    x: sliderX + _minSliderX
                });
                _motion.curAnim.joints[1].rollOffset = (thumbPositionNormalised - 0.5) *
                    2 * _sliderRanges.joints[1].rollOffsetRange;
            } else if (_movingSliderFive) {
                // legs forward (lower legs pitch offset)
                Overlays.editOverlay(_sliderFive, {
                    x: sliderX + _minSliderX
                });
                _motion.curAnim.joints[1].pitchOffset = (thumbPositionNormalised - 0.5) *
                    2 * _sliderRanges.joints[1].pitchOffsetRange;
            } else if (_movingSliderSix) {
                // lower legs splay (lower legs roll offset)
                Overlays.editOverlay(_sliderSix, {
                    x: sliderX + _minSliderX
                });
                _motion.curAnim.joints[2].rollOffset = (thumbPositionNormalised - 0.5) *
                    2 * _sliderRanges.joints[2].rollOffsetRange;

            } else if (_movingSliderSeven) {
                // arms forward (upper arms yaw offset)
                Overlays.editOverlay(_sliderSeven, {
                    x: sliderX + _minSliderX
                });
                _motion.curAnim.joints[9].yawOffset = (thumbPositionNormalised - 0.5) *
                    2 * _sliderRanges.joints[9].yawOffsetRange;
            } else if (_movingSliderEight) {
                // arms out (upper arm pitch offset)
                Overlays.editOverlay(_sliderEight, {
                    x: sliderX + _minSliderX
                });
                _motion.curAnim.joints[9].pitchOffset = (thumbPositionNormalised - 0.5) *
                    -2 * _sliderRanges.joints[9].pitchOffsetRange;
            } else if (_movingSliderNine) {
                // lower arms splay (lower arm pitch offset)
                Overlays.editOverlay(_sliderNine, {
                    x: sliderX + _minSliderX
                });
                _motion.curAnim.joints[10].pitchOffset = (thumbPositionNormalised - 0.5) *
                    -2 * _sliderRanges.joints[10].pitchOffsetRange;
            }
        } // if tweaking
    };

    function mouseReleaseEvent(event) {

        if (_movingSliderOne) {
			_movingSliderOne = false;
		} else if (_movingSliderTwo) {
			_movingSliderTwo = false;
		} else if (_movingSliderThree) {
			_movingSliderThree = false;
		} else if (_movingSliderFour) {
			_movingSliderFour = false;
		} else if (_movingSliderFive) {
			_movingSliderFive = false;
		} else if (_movingSliderSix) {
			_movingSliderSix = false;
		} else if (_movingSliderSeven) {
			_movingSliderSeven = false;
		} else if (_movingSliderEight) {
			_movingSliderEight = false;
		} else if (_movingSliderNine) {
			_movingSliderNine = false;
		}
    };

    Controller.mousePressEvent.connect(mousePressEvent);
    Controller.mouseMoveEvent.connect(mouseMoveEvent);
    Controller.mouseReleaseEvent.connect(mouseReleaseEvent);

    // Script ending
    Script.scriptEnding.connect(function() {

        // delete the background overlays
        for (var i in _backgroundOverlays) {
            Overlays.deleteOverlay(_backgroundOverlays[i]);
        }
        // delete the button overlays
        for (var i in _buttonOverlays) {
            Overlays.deleteOverlay(_buttonOverlays[i]);
        }
        // delete the slider thumb overlays
        for (var i in _sliderThumbOverlays) {
            Overlays.deleteOverlay(_sliderThumbOverlays[i]);
        }
        // delete the character joint control overlays
        for (var i in _jointsControlOverlays) {
            Overlays.deleteOverlay(_jointsControlOverlays[i]);
        }
        // delete the big button overlays
        for (var i in _bigbuttonOverlays) {
            Overlays.deleteOverlay(_bigbuttonOverlays[i]);
        }
        // delete the mimimised tab
        Overlays.deleteOverlay(_controlsMinimisedTab);
    });

    // public methods
    return {

        // gather references to objects from the walk.js script
        initialise: function(state, motion, walkAssets) {

            _state = state;
            _motion = motion;
            _walkAssets = walkAssets;
        },

        updateMenu: function() {

            if (!_state.minimised) {

                switch (_state.currentState) {

                    case _state.EDIT_WALK_STYLES:
                    case _state.EDIT_WALK_TWEAKS:
                    case _state.EDIT_WALK_JOINTS: {

                        hideMenuButtons();
                        initialiseFrontPanel(false);
                        hideJointSelectors();

                        if (_state.currentState === _state.EDIT_WALK_STYLES) {

                            setBackground(_controlsBackgroundWalkEditStyles);
                            initialiseWalkStylesPanel(true);
                            setSliderThumbsVisible(false);
                            hideJointSelectors();
                            setButtonOverlayVisible(_configWalkStylesButtonSelected);
                            setButtonOverlayVisible(_configWalkTweaksButton);
                            setButtonOverlayVisible(_configWalkJointsButton);

                        } else if (_state.currentState === _state.EDIT_WALK_TWEAKS) {

                            setBackground(_controlsBackgroundWalkEditTweaks);
                            initialiseWalkStylesPanel(false);
                            setSliderThumbsVisible(true);
                            hideJointSelectors();
                            initialiseWalkTweaksPanel();
                            setButtonOverlayVisible(_configWalkStylesButton);
                            setButtonOverlayVisible(_configWalkTweaksButtonSelected);
                            setButtonOverlayVisible(_configWalkJointsButton);

                        } else if (_state.currentState === _state.EDIT_WALK_JOINTS) {

                            if (_state.editingTranslation) {
                                setBackground(_controlsBackgroundWalkEditHipTrans);
							} else {
                                setBackground(_controlsBackgroundWalkEditJoints);
							}

                            initialiseWalkStylesPanel(false);
                            setSliderThumbsVisible(true);
                            setButtonOverlayVisible(_configWalkStylesButton);
                            setButtonOverlayVisible(_configWalkTweaksButton);
                            setButtonOverlayVisible(_configWalkJointsButtonSelected);
                            initialiseJointsEditingPanel();
                        }
                        setButtonOverlayVisible(_onButton);
                        setButtonOverlayVisible(_backButton);
                        return;
					}

                    case _state.EDIT_STANDING:
                    case _state.EDIT_SIDESTEP_LEFT:
                    case _state.EDIT_SIDESTEP_RIGHT: {

                        if (_state.editingTranslation) {
                            setBackground(_controlsBackgroundWalkEditHipTrans);
						} else {
                            setBackground(_controlsBackgroundWalkEditJoints);
						}
                        hideMenuButtons();
                        initialiseWalkStylesPanel(false);
                        initialiseFrontPanel(false);

                        if (_state.currentState === _state.EDIT_SIDESTEP_LEFT) {

                            setButtonOverlayVisible(_configSideStepRightButton);
                            setButtonOverlayVisible(_configSideStepLeftButtonSelected);
                            setButtonOverlayVisible(_configStandButton);

                        } else if (_state.currentState === _state.EDIT_SIDESTEP_RIGHT) {

                            setButtonOverlayVisible(_configSideStepRightButtonSelected);
                            setButtonOverlayVisible(_configSideStepLeftButton);
                            setButtonOverlayVisible(_configStandButton);

                        } else if (_state.currentState === _state.EDIT_STANDING) {

                            setButtonOverlayVisible(_configSideStepRightButton);
                            setButtonOverlayVisible(_configSideStepLeftButton);
                            setButtonOverlayVisible(_configStandButtonSelected);
                        }
                        initialiseJointsEditingPanel();
                        setButtonOverlayVisible(_onButton);
                        setButtonOverlayVisible(_backButton);
                        return;
					}

                    case _state.EDIT_FLYING:
                    case _state.EDIT_FLYING_UP:
                    case _state.EDIT_FLYING_DOWN: {

                        setBackground(_controlsBackgroundWalkEditJoints);
                        hideMenuButtons();
                        initialiseWalkStylesPanel(false);
                        initialiseFrontPanel(false);
                        if (_state.currentState === _state.EDIT_FLYING) {

                            setButtonOverlayVisible(_configFlyingUpButton);
                            setButtonOverlayVisible(_configFlyingDownButton);
                            setButtonOverlayVisible(_configFlyingButtonSelected);

                        } else if (_state.currentState === _state.EDIT_FLYING_UP) {

                            setButtonOverlayVisible(_configFlyingUpButtonSelected);
                            setButtonOverlayVisible(_configFlyingDownButton);
                            setButtonOverlayVisible(_configFlyingButton);

                        } else if (_state.currentState === _state.EDIT_FLYING_DOWN) {

                            setButtonOverlayVisible(_configFlyingUpButton);
                            setButtonOverlayVisible(_configFlyingDownButtonSelected);
                            setButtonOverlayVisible(_configFlyingButton);
                        }
                        initialiseJointsEditingPanel();
                        setButtonOverlayVisible(_onButton);
                        setButtonOverlayVisible(_backButton);
                        return;
					}

                    case _state.STANDING:
                    case _state.WALKING:
                    case _state.FLYING:
                    case _state.SIDE_STEP:
                    default: {

                        hideMenuButtons();
                        hideJointSelectors();
                        setBackground(_controlsBackground);
                        if (_state.powerOn) {
							setButtonOverlayVisible(_onButton);
						} else {
							setButtonOverlayVisible(_offButton);
						}
                        setButtonOverlayVisible(_configWalkButton);
                        setButtonOverlayVisible(_configStandButton);
                        setButtonOverlayVisible(_configFlyingButton);
                        setButtonOverlayVisible(_hideButton);
                        setSliderThumbsVisible(false);
                        initialiseFrontPanel(true);
                        initialiseWalkStylesPanel(false);
                        return;
					}
                }
            }
        }
    }; // end public methods (return)
})();