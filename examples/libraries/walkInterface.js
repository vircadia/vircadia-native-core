//
//  walkInterface.js
//
//  version 2.0
//
//  Created by David Wooldridge, Autumn 2014
//
//  Presents the UI for the walk.js script v1.12
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

walkInterface = (function() {

    // references to walk.js objects
    var _motion = null;
    var _walkAssets = null;

    // controller UI element positions and dimensions
    var _backgroundWidth = 350;
    var _backgroundHeight = 700;
    var _backgroundX = Window.innerWidth - _backgroundWidth - 58;
    var _backgroundY = Window.innerHeight / 2 - _backgroundHeight / 2;
    var _bigButtonsY = 348;

    // Load up the overlays
    var _buttonOverlays = [];

    // ui minimised tab
    var _controlsMinimisedTab = Overlays.addOverlay("image", {
        x: Window.innerWidth - 58,
        y: Window.innerHeight - 145,
        width: 50, height: 50,
        imageURL: pathToAssets + 'overlay-images/minimised-tab.png',
        visible: true, alpha: 0.9
    });

    // ui background
    var _controlsBackground = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX,
            y: _backgroundY,
            width: _backgroundWidth,
            height: _backgroundHeight
        },
        imageURL: pathToAssets + "overlay-images/background.png",
        alpha: 1, visible: false
    });

    // button overlays
    var _controlsMinimiseButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + _backgroundWidth - 62,
            y: _backgroundY + 40,
            width: 25, height: 25
        },
        imageURL: pathToAssets + "overlay-images/minimise-button.png",
        alpha: 1, visible: false
    });
    _buttonOverlays.push(_controlsMinimiseButton);

    var _onButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + _backgroundWidth / 2 - 115,
            y: _backgroundY + _bigButtonsY,
            width: 230, height: 36
        },
        imageURL: pathToAssets + "overlay-images/power-button-selected.png",
        alpha: 1, visible: false
    });
    _buttonOverlays.push(_onButton);

    var _offButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + _backgroundWidth / 2 - 115,
            y: _backgroundY + _bigButtonsY,
            width: 230, height: 36
        },
        imageURL: pathToAssets + "overlay-images/power-button.png",
        alpha: 1, visible: false
    });
    _buttonOverlays.push(_offButton);

    var _femaleButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + _backgroundWidth / 2 - 115,
            y: _backgroundY + _bigButtonsY + 60,
            width: 230, height: 36
        },
        imageURL: pathToAssets + "overlay-images/female-button.png",
        alpha: 1, visible: false
    });
    _buttonOverlays.push(_femaleButton);

    var _femaleButtonSelected = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + _backgroundWidth / 2 - 115,
            y: _backgroundY + _bigButtonsY + 60,
            width: 230, height: 36
        },
        imageURL: pathToAssets + "overlay-images/female-button-selected.png",
        alpha: 1, visible: false
    });
    _buttonOverlays.push(_femaleButtonSelected);

    var _maleButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + _backgroundWidth / 2 - 115,
            y: _backgroundY + _bigButtonsY + 120,
            width: 230, height: 36
        },
        imageURL: pathToAssets + "overlay-images/male-button.png",
        alpha: 1, visible: false
    });
    _buttonOverlays.push(_maleButton);

    var _maleButtonSelected = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + _backgroundWidth / 2 - 115,
            y: _backgroundY + _bigButtonsY + 120,
            width: 230, height: 36
        },
        imageURL: pathToAssets + "overlay-images/male-button-selected.png",
        alpha: 1, visible: false
    });
    _buttonOverlays.push(_maleButtonSelected);

    var _armsFreeButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + _backgroundWidth / 2 - 115,
            y: _backgroundY + _bigButtonsY + 180,
            width: 230, height: 36
        },
        imageURL: pathToAssets + "overlay-images/arms-free-button.png",
        alpha: 1, visible: false
    });
    _buttonOverlays.push(_armsFreeButton);

    var _armsFreeButtonSelected = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + _backgroundWidth / 2 - 115,
            y: _backgroundY + _bigButtonsY + 180,
            width: 230, height: 36
        },
        imageURL: pathToAssets + "overlay-images/arms-free-button-selected.png",
        alpha: 1, visible: false
    });
    _buttonOverlays.push(_armsFreeButtonSelected);

    var _footstepsButton = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + _backgroundWidth / 2 - 115,
            y: _backgroundY + _bigButtonsY + 240,
            width: 230, height: 36
        },
        imageURL: pathToAssets + "overlay-images/footstep-sounds-button.png",
        alpha: 1, visible: false
    });
    _buttonOverlays.push(_footstepsButton);

    var _footstepsButtonSelected = Overlays.addOverlay("image", {
        bounds: {
            x: _backgroundX + _backgroundWidth / 2 - 115,
            y: _backgroundY + _bigButtonsY + 240,
            width: 230, height: 36
        },
        imageURL: pathToAssets + "overlay-images/footstep-sounds-button-selected.png",
        alpha: 1, visible: false
    });
    _buttonOverlays.push(_footstepsButtonSelected);


    function minimiseDialog(minimise) {

        Overlays.editOverlay(_controlsBackground, {visible: !minimise});
        Overlays.editOverlay(_controlsMinimisedTab, {visible: minimise});
        Overlays.editOverlay(_controlsMinimiseButton, {visible: !minimise});

        if(_state.powerOn) {

            Overlays.editOverlay(_onButton, {visible: !minimise});
            Overlays.editOverlay(_offButton, {visible: false});

        } else {

            Overlays.editOverlay(_onButton, {visible: false});
            Overlays.editOverlay(_offButton, {visible: !minimise});

        }
        if (_motion.avatarGender === FEMALE) {

            Overlays.editOverlay(_femaleButtonSelected, {visible: !minimise});
            Overlays.editOverlay(_femaleButton, {visible: false});
            Overlays.editOverlay(_maleButtonSelected, {visible: false});
            Overlays.editOverlay(_maleButton, {visible: !minimise});

        } else {

            Overlays.editOverlay(_femaleButtonSelected, {visible: false});
            Overlays.editOverlay(_femaleButton, {visible: !minimise});
            Overlays.editOverlay(_maleButtonSelected, {visible: !minimise});
            Overlays.editOverlay(_maleButton, {visible: false});
        }
        if (_motion.armsFree) {

            Overlays.editOverlay(_armsFreeButtonSelected, {visible: !minimise});
            Overlays.editOverlay(_armsFreeButton, {visible: false});

        } else {

            Overlays.editOverlay(_armsFreeButtonSelected, {visible: false});
            Overlays.editOverlay(_armsFreeButton, {visible: !minimise});
        }
        if (_motion.makesFootStepSounds) {

            Overlays.editOverlay(_footstepsButtonSelected, {visible: !minimise});
            Overlays.editOverlay(_footstepsButton, {visible: false});

        } else {

            Overlays.editOverlay(_footstepsButtonSelected, {visible: false});
            Overlays.editOverlay(_footstepsButton, {visible: !minimise});
        }
    };

    // mouse event handler
    function mousePressEvent(event) {

        var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});

        switch (clickedOverlay) {

            case _controlsMinimiseButton:

                minimiseDialog(true);
                _state.setInternalState(_state.STANDING);
                return;

            case _controlsMinimisedTab:

                minimiseDialog(false);
                _state.setInternalState(_state.STANDING);
                return;

            case _onButton:

                _state.powerOn = false;
                Overlays.editOverlay(_offButton, {visible: true});
                Overlays.editOverlay(_onButton, {visible: false});
                _state.setInternalState(state.STANDING);
                return;

            case _offButton:

                _state.powerOn = true;
                Overlays.editOverlay(_offButton, {visible: false});
                Overlays.editOverlay(_onButton, {visible: true});
                _state.setInternalState(state.STANDING);
                return;


            case _footstepsButton:

                _motion.makesFootStepSounds = true;
                Overlays.editOverlay(_footstepsButtonSelected, {visible: true});
                Overlays.editOverlay(_footstepsButton, {visible: false});
                return;

            case _footstepsButtonSelected:

                _motion.makesFootStepSounds = false;
                Overlays.editOverlay(_footstepsButton, {visible: true});
                Overlays.editOverlay(_footstepsButtonSelected, {visible: false});
                return;

            case _femaleButton:
            case _maleButtonSelected:

                _motion.setGender(FEMALE);
                Overlays.editOverlay(_femaleButtonSelected, {visible: true});
                Overlays.editOverlay(_femaleButton, {visible: false});
                Overlays.editOverlay(_maleButton, {visible: true});
                Overlays.editOverlay(_maleButtonSelected, {visible: false});
                return;

            case _maleButton:
            case _femaleButtonSelected:

                _motion.setGender(MALE);
                Overlays.editOverlay(_femaleButton, {visible: true});
                Overlays.editOverlay(_femaleButtonSelected, {visible: false});
                Overlays.editOverlay(_maleButtonSelected, {visible: true});
                Overlays.editOverlay(_maleButton, {visible: false});
                return;

            case _armsFreeButton:

                _motion.armsFree = true;
                Overlays.editOverlay(_armsFreeButtonSelected, {visible: true});
                Overlays.editOverlay(_armsFreeButton, {visible: false});
                return;

            case _armsFreeButtonSelected:

                _motion.armsFree = false;
                _motion.poseFingers();
                Overlays.editOverlay(_armsFreeButtonSelected, {visible: false});
                Overlays.editOverlay(_armsFreeButton, {visible: true});
                return;
        }
    };

    Controller.mousePressEvent.connect(mousePressEvent);

    // delete overlays on script ending
    Script.scriptEnding.connect(function() {

        // delete overlays
        Overlays.deleteOverlay(_controlsBackground);
        Overlays.deleteOverlay(_controlsMinimisedTab);
        for (var i in _buttonOverlays) {
            Overlays.deleteOverlay(_buttonOverlays[i]);
        }
    });

    // public method
    return {

        // gather references to objects from the walk.js script
        initialise: function(state, motion, walkAssets) {

            _state = state;
            _motion = motion;
            _walkAssets = walkAssets;
        }

    }; // end public methods (return)

})();