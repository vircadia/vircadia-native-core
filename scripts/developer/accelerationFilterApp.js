var LEFT_HAND_INDEX = 0;
var RIGHT_HAND_INDEX = 1;
var LEFT_FOOT_INDEX = 2;
var RIGHT_FOOT_INDEX = 3;
var HIPS_INDEX = 4;
var SPINE2_INDEX = 5;

var mappingJson = {
    name: "com.highfidelity.testing.accelerationTest",
    channels: [
        {
            from: "Vive.LeftHand",
            to: "Standard.LeftHand",
            filters: [
                {
                    type: "accelerationLimiter",
                    rotationAccelerationLimit: 2000.0,
                    rotationDecelerationLimit: 4000.0,
                    translationAccelerationLimit: 100.0,
                    translationDecelerationLimit: 200.0
                }
            ]
        },
        {
            from: "Vive.RightHand",
            to: "Standard.RightHand",
            filters: [
                {
                    type: "accelerationLimiter",
                    rotationAccelerationLimit: 2000.0,
                    rotationDecelerationLimit: 4000.0,
                    translationAccelerationLimit: 100.0,
                    translationDecelerationLimit: 200.0
                }
            ]
        },
        {
            from: "Vive.LeftFoot",
            to: "Standard.LeftFoot",
            filters: [
                {
                    type: "exponentialSmoothing",
                    rotation: 0.15,
                    translation: 0.3
                }
            ]
        },
        {
            from: "Vive.RightFoot",
            to: "Standard.RightFoot",
            filters: [
                {
                    type: "exponentialSmoothing",
                    rotation: 0.15,
                    translation: 0.3
                }
            ]
        },
        {
            from: "Vive.Hips",
            to: "Standard.Hips",
            filters: [
                {
                    type: "exponentialSmoothing",
                    rotation: 0.15,
                    translation: 0.3
                }
            ]
        },
        {
            from: "Vive.Spine2",
            to: "Standard.Spine2",
            filters: [
                {
                    type: "exponentialSmoothing",
                    rotation: 0.15,
                    translation: 0.3
                }
            ]
        }
    ]
};

//
// tablet app boiler plate
//

var TABLET_BUTTON_NAME = "ACCFILT";
var HTML_URL = "https://s3.amazonaws.com/hifi-public/tony/html/accelerationFilterApp.html";

var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
var tabletButton = tablet.addButton({
    text: TABLET_BUTTON_NAME,
    icon: "https://s3.amazonaws.com/hifi-public/tony/icons/tpose-i.svg",
    activeIcon: "https://s3.amazonaws.com/hifi-public/tony/icons/tpose-a.svg"
});

tabletButton.clicked.connect(function () {
    if (shown) {
        tablet.gotoHomeScreen();
    } else {
        tablet.gotoWebScreen(HTML_URL);
    }
});

var shown = false;

function onScreenChanged(type, url) {
    if (type === "Web" && url === HTML_URL) {
        tabletButton.editProperties({isActive: true});
        if (!shown) {
            // hook up to event bridge
            tablet.webEventReceived.connect(onWebEventReceived);
            shownChanged(true);
        }
        shown = true;
    } else {
        tabletButton.editProperties({isActive: false});
        if (shown) {
            // disconnect from event bridge
            tablet.webEventReceived.disconnect(onWebEventReceived);
            shownChanged(false);
        }
        shown = false;
    }
}

function getTranslationAccelerationLimit(i) {
    return mappingJson.channels[i].filters[0].translationAccelerationLimit;
}
function setTranslationAccelerationLimit(i, value) {
    mappingJson.channels[i].filters[0].translationAccelerationLimit = value;
    mappingChanged();
}
function getTranslationDecelerationLimit(i) {
    return mappingJson.channels[i].filters[0].translationDecelerationLimit;
}
function setTranslationDecelerationLimit(i, value) {
    mappingJson.channels[i].filters[0].translationDecelerationLimit = value; mappingChanged();
}
function getRotationAccelerationLimit(i) {
    return mappingJson.channels[i].filters[0].rotationAccelerationLimit;
}
function setRotationAccelerationLimit(i, value) {
    mappingJson.channels[i].filters[0].rotationAccelerationLimit = value; mappingChanged();
}
function getRotationDecelerationLimit(i) {
    return mappingJson.channels[i].filters[0].rotationDecelerationLimit;
}
function setRotationDecelerationLimit(i, value) {
    mappingJson.channels[i].filters[0].rotationDecelerationLimit = value; mappingChanged();
}

function onWebEventReceived(msg) {
    if (msg.name === "init-complete") {
        var values = [
            {name: "left-hand-translation-acceleration-limit", val: getTranslationAccelerationLimit(LEFT_HAND_INDEX), checked: false},
            {name: "left-hand-translation-deceleration-limit", val: getTranslationDecelerationLimit(LEFT_HAND_INDEX), checked: false},
            {name: "left-hand-rotation-acceleration-limit", val: getRotationAccelerationLimit(LEFT_HAND_INDEX), checked: false},
            {name: "left-hand-rotation-deceleration-limit", val: getRotationDecelerationLimit(LEFT_HAND_INDEX), checked: false}
        ];
        tablet.emitScriptEvent(JSON.stringify(values));
    } else if (msg.name === "left-hand-translation-acceleration-limit") {
        setTranslationAccelerationLimit(LEFT_HAND_INDEX, parseInt(msg.val, 10));
    } else if (msg.name === "left-hand-translation-deceleration-limit") {
        setTranslationDecelerationLimit(LEFT_HAND_INDEX, parseInt(msg.val, 10));
    } else if (msg.name === "left-hand-rotation-acceleration-limit") {
        setRotationAccelerationLimit(LEFT_HAND_INDEX, parseInt(msg.val, 10));
    } else if (msg.name === "left-hand-rotation-deceleration-limit") {
        setRotationDecelerationLimit(LEFT_HAND_INDEX, parseInt(msg.val, 10));
    }
}

tablet.screenChanged.connect(onScreenChanged);

function shutdownTabletApp() {
    tablet.removeButton(tabletButton);
    if (shown) {
        tablet.webEventReceived.disconnect(onWebEventReceived);
        tablet.gotoHomeScreen();
    }
    tablet.screenChanged.disconnect(onScreenChanged);
}

//
// end tablet app boiler plate
//

var mapping;
function mappingChanged() {
    if (mapping) {
        mapping.disable();
    }
    mapping = Controller.parseMapping(JSON.stringify(mappingJson));
    mapping.enable();
}

function shownChanged(newShown) {
    if (newShown) {
        mappingChanged();
    } else {
        mapping.disable();
    }
}

mappingChanged();

Script.scriptEnding.connect(function() {
    if (mapping) {
        mapping.disable();
    }
    tablet.removeButton(tabletButton);
});

