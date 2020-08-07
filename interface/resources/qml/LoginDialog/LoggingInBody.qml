//
//  LoggingInBody.qml
//
//  Created by Wayne Chen on 10/18/18
//  Copyright 2018 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4 as OriginalStyles

import controlsUit 1.0 as HifiControlsUit
import stylesUit 1.0 as HifiStylesUit

Item {
    id: loggingInBody
    clip: true
    height: root.height
    width: root.width
    property int textFieldHeight: 31
    property int loggingInGlyphRightMargin: 10
    property string fontFamily: "Raleway"
    property int fontSize: 15
    property bool fontBold: true
    property bool withSteam: withSteam
    property bool withOculus: withOculus
    property bool linkSteam: linkSteam
    property bool linkOculus: linkOculus
    property bool createOculus: createOculus
    property bool isLoggingInToDomain: isLoggingInToDomain
    property string domainLoginDomain: domainLoginDomain
    property string displayName: ""

    readonly property bool loginDialogPoppedUp: loginDialog.getLoginDialogPoppedUp()

    QtObject {
        id: d
        readonly property int minWidth: 480
        readonly property int minWidthButton: !root.isTablet ? 256 : 174
        property int maxWidth: root.width
        readonly property int minHeight: 120
        readonly property int minHeightButton: 36
        property int maxHeight: root.height

        function resize() {
            maxWidth = root.width;
            maxHeight = root.height;
            var targetWidth = Math.max(titleWidth, mainContainer.width);
            var targetHeight =  hifi.dimensions.contentSpacing.y + mainContainer.height +
                    4 * hifi.dimensions.contentSpacing.y;

            var newWidth = Math.max(d.minWidth, Math.min(d.maxWidth, targetWidth));
            if (!isNaN(newWidth)) {
                parent.width = root.width = newWidth;
            }

            parent.height = root.height = Math.max(d.minHeight, Math.min(d.maxHeight, targetHeight)) + hifi.dimensions.contentSpacing.y;
        }
    }

    // timer to kill the dialog upon login success
    Timer {
        id: successTimer
        interval: 1000;
        running: false;
        repeat: false;
        onTriggered: {
            if (loggingInBody.loginDialogPoppedUp) {
                loginDialog.dismissLoginDialog();
                var data = {
                    "action": "user logged in successfully"
                };
                UserActivityLogger.logAction("encourageLoginDialog", data);
            }
            root.tryDestroy();
        }
    }

    Timer {
        id: oculusSuccessTimer
        interval: 500;
        running: false;
        repeat: false;
        onTriggered: {
            loginDialog.loginThroughOculus();
            init();
        }
    }

    function init() {
        // For the process of logging in.
        loggingInText.wrapMode = Text.NoWrap;
        if (loggingInBody.createOculus) {
            loggingInGlyph.text = hifi.glyphs.oculus;
            loggingInGlyph.visible = true;
            loggingInText.text = "Creating account with Oculus";
            loggingInText.x = loggingInHeader.width/2 - loggingInTextMetrics.width/2 + loggingInGlyphTextMetrics.width/2;
        } else if (loggingInBody.withSteam) {
            loggingInGlyph.visible = true;
            loggingInText.text = "Logging in to Steam";
            loggingInText.x = loggingInHeader.width/2 - loggingInTextMetrics.width/2 + loggingInGlyphTextMetrics.width/2;
        } else if (loggingInBody.withOculus) {
            loggingInGlyph.text = hifi.glyphs.oculus;
            loggingInGlyph.visible = true;
            loggingInText.text = "Logging in to Oculus";
            loggingInText.x = loggingInHeader.width/2 - loggingInTextMetrics.width/2 + loggingInGlyphTextMetrics.width/2;
        } else if (loggingInBody.isLoggingInToDomain) {
            loggingInText.text = "Logging in to " + domainLoginDomain;
            loggingInText.anchors.centerIn = loggingInHeader;
        } else {
            loggingInText.text = "Logging in";
            loggingInText.anchors.centerIn = loggingInHeader;
        }
        loggingInSpinner.visible = true;
    }
    function loadingSuccess() {
        if (loggingInBody.linkSteam) {
            loggingInText.text = "Linking to Steam";
            loggingInText.x = loggingInHeader.width/2 - loggingInTextMetrics.width/2 + loggingInGlyphTextMetrics.width/2;
            loginDialog.linkSteam();
            return;
        } else if (loggingInBody.linkOculus) {
            loggingInText.text = "Linking to Oculus";
            loggingInText.x = loggingInHeader.width/2 - loggingInTextMetrics.width/2 + loggingInGlyphTextMetrics.width/2;
            loginDialog.linkOculus();
            return;
        }
        loggingInSpinner.visible = false;
        if (loggingInBody.withSteam) {
            // reset the flag.
            loggingInGlyph.visible = false;
            loggingInText.text = "You are now logged into Steam!";
            loggingInText.x = 0;
            loggingInText.anchors.centerIn = loggingInHeader;
            loggedInGlyph.visible = true;
            if (loggingInBody.loginDialogPoppedUp) {
                var data = {
                    "action": "user logged in with Steam successfully"
                };
                UserActivityLogger.logAction("encourageLoginDialog", data);
            }
        } else if (loggingInBody.withOculus) {
            // reset the flag.
            loggingInGlyph.visible = false;
            loggingInText.text = "You are now logged into Oculus!";
            loggingInText.x = 0;
            loggingInText.anchors.centerIn = loggingInHeader;
            loggedInGlyph.text = hifi.glyphs.oculus;
            loggedInGlyph.visible = true;
            if (loggingInBody.loginDialogPoppedUp) {
                var data = {
                    "action": "user logged in with Oculus successfully"
                };
                UserActivityLogger.logAction("encourageLoginDialog", data);
            }
        } else {
            loggingInText.text = "You are now logged in!";
            if (loggingInBody.loginDialogPoppedUp) {
                var data = {
                    "action": "user logged in successfully"
                };
                UserActivityLogger.logAction("encourageLoginDialog", data);
            }

        }
        MyAvatar.displayName = displayName;
        successTimer.start();
    }

    Item {
        id: mainContainer
        anchors.fill: parent
        onHeightChanged: d.resize(); onWidthChanged: d.resize();

        Item {
            id: loggingInContainer
            anchors.fill: parent
            onHeightChanged: d.resize(); onWidthChanged: d.resize();

            Item {
                id: loggingInHeader
                width: parent.width
                height: loggingInGlyph.height
                anchors {
                    top: parent.top
                    topMargin: 0.4 * parent.height
                    left: parent.left
                }
                TextMetrics {
                    id: loggingInGlyphTextMetrics;
                    font: loggingInGlyph.font;
                    text: loggingInGlyph.text;
                }
                HifiStylesUit.HiFiGlyphs {
                    id: loggingInGlyph;
                    text: hifi.glyphs.steamSquare;
                    // Color
                    color: "white";
                    // Size
                    size: 36;
                    // Anchors
                    anchors.right: loggingInText.left;
                    anchors.rightMargin: loggingInBody.loggingInGlyphRightMargin
                    anchors.bottom: parent.bottom;
                    // Alignment
                    horizontalAlignment: Text.AlignHCenter;
                    verticalAlignment: Text.AlignVCenter;
                    visible: loggingInBody.withSteam || loggingInBody.withOculus;
                }

                TextMetrics {
                    id: loggingInTextMetrics;
                    font: loggingInText.font;
                    text: loggingInText.text;
                }
                Text {
                    id: loggingInText;
                    width: loggingInTextMetrics.width
                    anchors.verticalCenter: parent.verticalCenter
                    color: "white";
                    font.family: loggingInBody.fontFamily
                    font.pixelSize: 24
                    font.bold: loggingInBody.fontBold
                    wrapMode: Text.NoWrap
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    text: "Logging in"
                }
            }
            Item {
                id: loggingInFooter
                width: parent.width
                height: 0.5 * parent.height
                anchors {
                    top: loggingInHeader.bottom
                    topMargin: 2 * hifi.dimensions.contentSpacing.y
                }
                AnimatedImage {
                    id: loggingInSpinner
                    source: "images/loader-snake-256.gif"
                    width: 64
                    height: width
                    anchors.left: parent.left;
                    anchors.leftMargin: (parent.width - width) / 2;
                    anchors.top: parent.top
                    anchors.topMargin: hifi.dimensions.contentSpacing.y
                }
                TextMetrics {
                    id: loggedInGlyphTextMetrics;
                    font: loggedInGlyph.font;
                    text: loggedInGlyph.text;
                }
                HifiStylesUit.HiFiGlyphs {
                    id: loggedInGlyph;
                    text: hifi.glyphs.steamSquare;
                    // color
                    color: "white"
                    // Size
                    size: 78;
                    // Anchors
                    anchors.left: parent.left;
                    anchors.leftMargin: (parent.width - loggedInGlyph.size) / 2;
                    anchors.top: parent.top
                    anchors.topMargin: hifi.dimensions.contentSpacing.y
                    // Alignment
                    horizontalAlignment: Text.AlignHCenter;
                    verticalAlignment: Text.AlignVCenter;
                    visible: false;
                }
                HifiControlsUit.Button {
                    id: okButton;
                    width: d.minWidthButton
                    height: d.minHeightButton
                    text: qsTr("OK")
                    color: hifi.buttons.white
                    anchors {
                        top: loggedInGlyph.bottom
                        topMargin: 3 * hifi.dimensions.contentSpacing.y
                        left: parent.left
                        leftMargin: (parent.width - width) / 2;
                    }
                    onClicked: {
                        root.tryDestroy();
                        if (loginDialog.getLoginDialogPoppedUp()) {
                            loginDialog.dismissLoginDialog();
                        }
                    }
                    visible: false
                }
            }
        }
    }

    Component.onCompleted: {
        d.resize();
        loggingInBody.init();
    }

    Connections {
        target: loginDialog
        onHandleCreateCompleted: {
            console.log("Create Succeeded")
            if (loggingInBody.withOculus) {
                if (loggingInBody.loginDialogPoppedUp) {
                    var data = {
                        "action": "user created Oculus account successfully"
                    };
                    UserActivityLogger.logAction("encourageLoginDialog", data);
                }
                loggingInBody.createOculus = false;
                loggingInText.text = "Account created!";
                loggingInText.x = loggingInHeader.width/2 - loggingInTextMetrics.width/2 + loggingInGlyphTextMetrics.width/2;
                oculusSuccessTimer.start();
            }
        }
        onHandleCreateFailed: {
            console.log("Create Failed: " + error);
            if (loggingInBody.withOculus) {
                if (loggingInBody.loginDialogPoppedUp) {
                    var data = {
                        "action": "user created Oculus account unsuccessfully"
                    };
                    UserActivityLogger.logAction("encourageLoginDialog", data);
                }
                bodyLoader.setSource("CompleteProfileBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "withSteam": loggingInBody.withSteam,
                    "withOculus": loggingInBody.withOculus, "errorString": error });
            }
        }
        onHandleLinkCompleted: {
            console.log("Link Succeeded");
            if (loggingInBody.linkSteam) {
                loggingInBody.linkSteam = false;
                if (loggingInBody.loginDialogPoppedUp) {
                    var data = {
                        "action": "user linked Steam with their hifi account credentials successfully"
                    };
                    UserActivityLogger.logAction("encourageLoginDialog", data);
                }
            } else if (loggingInBody.linkOculus) {
                loggingInBody.linkOculus = false;
                if (loggingInBody.loginDialogPoppedUp) {
                    var data = {
                        "action": "user linked Oculus with their hifi account credentials successfully"
                    };
                    UserActivityLogger.logAction("encourageLoginDialog", data);
                }
            }
            loggingInBody.loadingSuccess();
        }
        onHandleLinkFailed: {
            console.log("Link Failed: " + error);
            loggingInSpinner.visible = false;
            if (loggingInBody.linkOculus) {
                loggingInText.text = "Oculus failed to link";
                if (loggingInBody.loginDialogPoppedUp) {
                    var data = {
                        "action": "user linked Oculus unsuccessfully"
                    };
                    UserActivityLogger.logAction("encourageLoginDialog", data);
                }
                okButton.visible = true;
            } else if (loggingInBody.linkSteam){
                if (loggingInBody.loginDialogPoppedUp) {
                    var data = {
                        "action": "user linked Steam unsuccessfully"
                    };
                    UserActivityLogger.logAction("encourageLoginDialog", data);
                }
            } else {
                bodyLoader.setSource("LinkAccountBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "linkSteam": loggingInBody.linkSteam,
                    "linkOculus": loggingInBody.linkOculus, "errorString": error });
            }
        }

        onHandleLoginCompleted: {
            console.log("Login Succeeded");
            loggingInBody.loadingSuccess();
        }

        onHandleLoginFailed: {
            console.log("Login Failed")
            loggingInSpinner.visible = false;
            loggingInGlyph.visible = false;
            var errorString = "";
            if (loggingInBody.linkOculus && loggingInBody.withOculus) {
                errorString = "Username or password is incorrect.";
                if (loggingInBody.loginDialogPoppedUp) {
                    var data = {
                        "action": "user failed to link Oculus with their hifi account credentials"
                    };
                    UserActivityLogger.logAction("encourageLoginDialog", data);
                }
                bodyLoader.setSource("LinkAccountBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "withSteam": loggingInBody.withSteam,
                    "withOculus": loggingInBody.withOculus, "linkSteam": loggingInBody.linkSteam, "linkOculus": loggingInBody.linkOculus, "errorString": errorString });
            } else if (loggingInBody.linkSteam && loggingInBody.withSteam) {
                errorString = "Username or password is incorrect.";
                if (loggingInBody.loginDialogPoppedUp) {
                    var data = {
                        "action": "user failed to link Steam with their hifi account credentials"
                    };
                    UserActivityLogger.logAction("encourageLoginDialog", data);
                }
                bodyLoader.setSource("LinkAccountBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "withSteam": loggingInBody.withSteam,
                    "withOculus": loggingInBody.withOculus, "linkSteam": loggingInBody.linkSteam, "linkOculus": loggingInBody.linkOculus, "errorString": errorString });
            } else if (loggingInBody.withSteam) {
                errorString = "Your Steam authentication has failed. Please make sure you are logged into Steam and try again.";
                if (loggingInBody.loginDialogPoppedUp) {
                    var data = {
                        "action": "user failed to authenticate with Steam to log in"
                    };
                    UserActivityLogger.logAction("encourageLoginDialog", data);
                }
                bodyLoader.setSource("CompleteProfileBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "withSteam": loggingInBody.withSteam,
                    "withOculus": loggingInBody.withOculus, "linkSteam": loggingInBody.linkSteam, "linkOculus": loggingInBody.linkOculus, "errorString": errorString });
            } else if (loggingInBody.withOculus) {
                errorString = "Your Oculus account is not connected to an existing High Fidelity account. Please create a new one."
                if (loggingInBody.loginDialogPoppedUp) {
                    var data = {
                        "action": "user failed to authenticate with Oculus to log in"
                    };
                    UserActivityLogger.logAction("encourageLoginDialog", data);
                }
                bodyLoader.setSource("CompleteProfileBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "withSteam": loggingInBody.withSteam,
                    "withOculus": loggingInBody.withOculus, "linkSteam": loggingInBody.linkSteam, "linkOculus": loggingInBody.linkOculus, "errorString": errorString });
            } else {
                errorString = "Username or password is incorrect.";
                if (loggingInBody.loginDialogPoppedUp) {
                    var data = {
                        "action": "user failed at logging in"
                    };
                    UserActivityLogger.logAction("encourageLoginDialog", data);
                }

                bodyLoader.setSource("LinkAccountBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "errorString": errorString });
            }
        }
    }
}
