/*

    Simplified Emote
    simplifiedEmote.js
    Created by Milad Nazeri on 2019-08-06
    Copyright 2019 High Fidelity, Inc.

    Distributed under the Apache License, Version 2.0.
    See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


*/


// *************************************
// START EMOTE
// *************************************
// #region EMOTE


// *************************************
// START EMOTE_UTILITY
// *************************************
// #region EMOTE_UTILITY


// See the note below about needing to end reactions before another one starts
// This just takes care of all 3 of them at the same time
function maybeEndOtherReactions() {
    if (raiseHandPlaying) {
        MyAvatar.endReaction("raiseHand");
        raiseHandPlaying = false;
    }
    if (applaudPlaying) {
        MyAvatar.endReaction("applaudPlaying");
        applaudPlaying = false;
    }
    if (pointPlaying) {
        MyAvatar.endReaction("pointPlaying");
        pointPlaying = false;
    }
}



function updateEmoteAppBarPosition() {
    if (!emoteAppBarWindow) {
        return;
    }

    emoteAppBarWindow.position = {
        x: Window.x + EMOTE_APP_BAR_LEFT_MARGIN,
        y: Window.y + Window.innerHeight - EMOTE_APP_BAR_BOTTOM_MARGIN
    };
}



// #endregion
// *************************************
// END EMOTE_UTILITY
// *************************************
// *************************************
// START EMOTE_HANDLERS
// *************************************
// #region EMOTE_HANDLERS

/*
    MILAD NOTE:
    Right now you can do more than one reaction at a time so it has to be handled here.  
    I might see about adding a small flag to the engine so that if a reaction is currently playing it will end it.
    This will cut down on a lot of boiler code below checking to see if another reaction is playing.  Also because it doesn't
    make sense for more than one reaction to be playing.
*/
var EMOTE_APP_BAR_MESSAGE_SOURCE = "EmoteAppBar.qml";
var raiseHandPlaying = false;
var applaudPlaying = false;
var pointPlaying = false;
function onMessageFromEmoteAppBar(message) {
    console.log("MESSAGE From emote app bar: ", message);
    if (message.source !== EMOTE_APP_BAR_MESSAGE_SOURCE) {
        return;
    }
    switch (message.method) {
        case "happyPressed":
            MyAvatar.triggerReaction("positive");
            break;
        case "sadPressed": 
            MyAvatar.triggerReaction("negative");
            break;
        case "raiseHandPressed":
            maybeEndOtherReactions();
            MyAvatar.beginReaction("raiseHand");
            raiseHandPlaying = true;
            break;
        case "applaudPressed":
            maybeEndOtherReactions();
            MyAvatar.beginReaction("applaud");
            applaudPlaying = true;
            break;
        case "pointPressed":
            maybeEndOtherReactions();
            MyAvatar.beginReaction("point");
            applaudPlaying = true;
            break;
        case "toggleEmojiApp": 
            toggleEmojiApp();
            break;
        default:
            console.log("Unrecognized message from " + EMOTE_APP_BAR_MESSAGE_SOURCE + ": " + JSON.stringify(message));
            break;
    }
}


function onGeometryChanged(rect) {
    updateEmoteAppBarPosition();
}


function onEmoteAppBarClosed() {
    if (emoteAppBarWindow) {
        emoteAppBarWindow.fromQml.disconnect(onMessageFromEmoteAppBar);
    }
    emoteAppBarWindow = false;
}


var POSITIVE_KEY = "z";
var NEGATIVE_KEY = "c";
var RAISE_HAND_KEY = "v";
var APPLAUD_KEY = "b";
var POINT_KEY = "n";
var EMOTE_WINDOW = "f";
function keyPressHandler(event) {
    if (!event.isAutoRepeat && ! event.isMeta && ! event.isControl && ! event.isAlt) {
        if (event.text === POSITIVE_KEY) {
            MyAvatar.triggerReaction("positive");
        } else if (event.text === NEGATIVE_KEY) {
            MyAvatar.triggerReaction("negative");
        } else if (event.text === RAISE_HAND_KEY) {
            MyAvatar.beginReaction("raiseHand");
        } else if (event.text === APPLAUD_KEY) {
            MyAvatar.beginReaction("applaud");
        } else if (event.text === POINT_KEY) {
            MyAvatar.beginReaction("point");
        } else if (event.text === EMOTE_WINDOW) {
            toggleEmojiApp();
        }
    }
}


function keyReleaseHandler(event) {
    if (!event.isAutoRepeat) {
        if (event.text === RAISE_HAND_KEY) {
            MyAvatar.endReaction("raiseHand");
        } else if (event.text === APPLAUD_KEY) {
            MyAvatar.endReaction("applaud");
        } else if (event.text === POINT_KEY) {
            MyAvatar.endReaction("point");
        }
    }
}


// #endregion
// *************************************
// END EMOTE_HANDLERS
// *************************************

// *************************************
// START EMOTE_MAIN
// *************************************
// #region EMOTE_MAIN

var EMOTE_APP_BAR_QML_PATH = Script.resolvePath("./ui/qml/SimplifiedEmoteIndicator.qml");
var EMOTE_APP_BAR_WINDOW_TITLE = "Emote Reaction Bar";
var EMOTE_APP_BAR_PRESENTATION_MODE = Desktop.PresentationMode.NATIVE;
var EMOTE_APP_BAR_WIDTH_PX = 48;
var EMOTE_APP_BAR_HEIGHT_PX = 48;
var EMOTE_APP_BAR_LEFT_MARGIN = 48;
var EMOTE_APP_BAR_BOTTOM_MARGIN = 48;
var EMOTE_APP_BAR_WINDOW_FLAGS = 0x00000001 | // Qt::Window
    0x00000800 | // Qt::FramelessWindowHint
    0x40000000 | // Qt::NoDropShadowWindowHint
    0x00200000; // Qt::WindowDoesNotAcceptFocus
var emoteAppBarWindow = false;
var emoteIndicatorVisible = true;
function showEmoteAppBar() {
    emoteAppBarWindow = Desktop.createWindow(EMOTE_APP_BAR_QML_PATH, {
        title: EMOTE_APP_BAR_WINDOW_TITLE,
        presentationMode: EMOTE_APP_BAR_PRESENTATION_MODE,
        size: {
            x: EMOTE_APP_BAR_WIDTH_PX,
            y: EMOTE_APP_BAR_HEIGHT_PX
        },
        position: {
            x: Window.x + EMOTE_APP_BAR_LEFT_MARGIN,
            y: Window.y + Window.innerHeight - EMOTE_APP_BAR_BOTTOM_MARGIN
        },
        overrideFlags: EMOTE_APP_BAR_WINDOW_FLAGS
    });

    emoteAppBarWindow.fromQml.connect(onMessageFromEmoteAppBar);
    emoteAppBarWindow.closed.connect(onEmoteAppBarClosed);
}


var EmojiAPI = Script.require("./emojiApp/simplifiedEmoji.js?" + Date.now());
var emojiAPI = new EmojiAPI();
var geometryChangedSignalConnected = false;
var keyPressSignalsConnected = false;
function init() {
    Window.geometryChanged.connect(onGeometryChanged);
    geometryChangedSignalConnected = true;
    emojiAPI.startup();
    showEmoteAppBar();
    
    Controller.keyPressEvent.connect(keyPressHandler);
    Controller.keyReleaseEvent.connect(keyReleaseHandler);
    keyPressSignalsConnected = true;

    Script.scriptEnding.connect(shutdown);
}


function shutdown() {
    if (emoteAppBarWindow) {
        emoteAppBarWindow.close();
    }

    if (emojiAppWindow) {
        emojiAppWindow.close();
    }

    emojiAPI.unload();

    if (geometryChangedSignalConnected) {
        Window.geometryChanged.disconnect(onGeometryChanged);
        geometryChangedSignalConnected = false;
    }

    if (keyPressSignalsConnected) {
        Controller.keyPressEvent.disconnect(keyPressHandler);
        Controller.keyReleaseEvent.disconnect(keyReleaseHandler);
        keyPressSignalsConnected = false;
    }
}


// #endregion
// *************************************
// END EMOTE_MAIN
// *************************************


// #endregion
// *************************************
// END EMOTE
// *************************************

// *************************************
// START EMOJI
// *************************************
// #region EMOJI

// *************************************
// START EMOJI_UTILITY
// *************************************
// #region EMOJI_UTILITY


function selectedEmoji(code) {
    emojiAPI.addEmoji(code);
}


// #endregion
// *************************************
// END EMOJI_UTILITY
// *************************************

// *************************************
// START EMOJI_HANDLERS
// *************************************
// #region EMOJI_HANDLERS


function onEmojiAppClosed() {
    if (emojiAppWindow) {
        emojiAppWindow.fromQml.disconnect(onMessageFromEmojiApp);
        emojiAppWindow.closed.disconnect(onEmojiAppClosed);
    }
    emojiAppWindow = false;
}


var EMOJI_APP_MESSAGE_SOURCE = "SimplifiedEmoji.qml";
function onMessageFromEmojiApp(message) {
    if (message.source !== EMOJI_APP_MESSAGE_SOURCE) {
        return;
    }

    switch (message.method) {
        case "selectedEmoji":
            selectedEmoji(message.code);
            break;
        default:
            console.log("Unrecognized message from " + EMOJI_APP_MESSAGE_SOURCE + ": " + JSON.stringify(message));
            break;
    }
}


// #endregion
// *************************************
// END EMOJI_HANDLERS
// *************************************

// *************************************
// START EMOJI_MAIN
// *************************************
// #region EMOJI_MAIN


var EMOJI_APP_QML_PATH = Script.resolvePath("./emojiApp/ui/qml/SimplifiedEmoji.qml");
var EMOJI_APP_WINDOW_TITLE = "Emoji";
var EMOJI_APP_PRESENTATION_MODE = Desktop.PresentationMode.NATIVE;
var EMOJI_APP_WIDTH_PX = 480;
var EMOJI_APP_HEIGHT_PX = 615;
var EMOJI_APP_WINDOW_FLAGS = 0x00000001 | // Qt::Window
    0x00001000 | // Qt::WindowTitleHint
    0x00002000 | // Qt::WindowSystemMenuHint
    0x08000000 | // Qt::WindowCloseButtonHint
    0x00008000 | // Qt::WindowMaximizeButtonHint
    0x00004000; // Qt::WindowMinimizeButtonHint
var emojiAppWindow = false;
var POPOUT_SAFE_MARGIN_X = 30;
var POPOUT_SAFE_MARGIN_Y = 30;
function toggleEmojiApp() {
    if (emojiAppWindow) {
        emojiAppWindow.close();
        // This really shouldn't be necessary.
        // This signal really should automatically be called by the signal handler set up below.
        // But fixing that requires an engine change, so this workaround will do.
        onEmojiAppClosed();
        return;
    }

    emojiAppWindow = Desktop.createWindow(EMOJI_APP_QML_PATH, {
        title: EMOJI_APP_WINDOW_TITLE,
        presentationMode: EMOJI_APP_PRESENTATION_MODE,
        size: {
            x: EMOJI_APP_WIDTH_PX,
            y: EMOJI_APP_HEIGHT_PX
        },
        position: {
            x: Math.max(Window.x + POPOUT_SAFE_MARGIN_X, Window.x + Window.innerWidth / 2 - EMOJI_APP_WIDTH_PX / 2),
            y: Math.max(Window.y + POPOUT_SAFE_MARGIN_Y, Window.y + Window.innerHeight / 2 - EMOJI_APP_HEIGHT_PX / 2)
        },
        overrideFlags: EMOJI_APP_WINDOW_FLAGS
    });

    emojiAppWindow.fromQml.connect(onMessageFromEmojiApp);
    emojiAppWindow.closed.connect(onEmojiAppClosed);
    // The actual emoji module needs this qml window object so it can send messages
    // to update the Selected emoji UI
    emojiAPI.registerAvimojiQMLWindow(emojiAppWindow);
}

// #endregion
// *************************************
// END EMOJI_MAIN
// *************************************

// #endregion
// *************************************
// END EMOJI
// *************************************

// *************************************
// START API
// *************************************
// #region API


function startup() {
    init();
}


function unload() {
    shutdown();
}

function handleEmoteIndicatorVisible(emoteIndicatorVisible) {
    console.log("in handle emote indicator");
    if (!emoteAppBarWindow) {
        return;
    } 

    if (emoteIndicatorVisible) {
        showEmoteAppBar();
    } else {
        emoteAppBarWindow.close();
    }
}

var _this;
function EmoteBar() {
    _this = this;
}


EmoteBar.prototype = {
    startup: startup,
    handleEmoteIndicatorVisible: handleEmoteIndicatorVisible,
    unload: unload
};

module.exports = EmoteBar;


// #endregion
// *************************************
// END API
// *************************************

