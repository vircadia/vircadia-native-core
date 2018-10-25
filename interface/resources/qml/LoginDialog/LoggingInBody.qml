//
//  LoggingInBody.qml
//
//  Created by Wayne Chen on 10/18/18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4 as OriginalStyles

import "qrc:////qml//controls-uit" as HifiControlsUit
import "qrc:////qml//styles-uit" as HifiStylesUit

Item {

    id: loggingInBody
    property int textFieldHeight: 31
    property int loggingInGlyphRightMargin: 10
    property string fontFamily: "Cairo"
    property int fontSize: 24
    property bool fontBold: true
    property bool withSteam: withSteam
    property string fromBody: fromBody

    QtObject {
        id: d
        readonly property int minWidth: 480
        readonly property int minWidthButton: !root.isTablet ? 256 : 174
        property int maxWidth: root.isTablet ? 1280 : Window.innerWidth
        readonly property int minHeight: 120
        readonly property int minHeightButton: !root.isTablet ? 56 : 42
        property int maxHeight: root.isTablet ? 720 : Window.innerHeight

        function resize() {
            maxWidth = root.isTablet ? 1280 : Window.innerWidth;
            maxHeight = root.isTablet ? 720 : Window.innerHeight;
            var targetWidth = Math.max(titleWidth, mainContainer.width);
            var targetHeight =  hifi.dimensions.contentSpacing.y + mainContainer.height +
                    4 * hifi.dimensions.contentSpacing.y;

            var newWidth = Math.max(d.minWidth, Math.min(d.maxWidth, targetWidth));
            if (!isNaN(newWidth)) {
                parent.width = root.pane.width = newWidth;
            }

            parent.height = root.pane.height = Math.max(d.minHeight, Math.min(d.maxHeight, targetHeight)) + hifi.dimensions.contentSpacing.y;
        }
    }

    // timer to kill the dialog upon login success
    Timer {
        id: successTimer
        interval: 1000;
        running: false;
        repeat: false;
        onTriggered: {
            root.tryDestroy();
        }
    }

    // timer to kill the dialog upon login failure
    Timer {
        id: steamFailureTimer
        interval: 2500;
        running: false;
        repeat: false;
        onTriggered: {
            // from linkaccount.
            bodyLoader.setSource("LinkAccountBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader })
        }
    }

    function init() {
        // For the process of logging in.
        loggingInText.wrapMode = Text.NoWrap;
        if (loggingInBody.withSteam) {
            loggingInGlyph.visible = true;
            loggingInText.text = "Logging in to Steam";
            loggingInText.x = loggingInHeader.width/2 - loggingInTextMetrics.width/2 + loggingInGlyphTextMetrics.width/2;
        } else {
            loggingInText.text = "Logging in";
            loggingInText.anchors.bottom = loggingInHeader.bottom;
            loggingInText.anchors.bottomMargin = hifi.dimensions.contentSpacing.y;
        }
        loggingInSpinner.visible = true;
    }
    function loadingSuccess() {
        loggingInSpinner.visible = false;
        if (loggingInBody.withSteam && !loginDialog.isLogIn) {
            // reset the flag.
            loggingInGlyph.visible = false;
            loggingInText.text = "You are now logged into Steam!"
            loggingInText.anchors.centerIn = loggingInHeader;
            loggingInText.anchors.bottom = loggingInHeader.bottom;
            loggedInGlyph.visible = true;
        } else {
            loggingInText.text = "You are now logged in!";
        }
        successTimer.start();
    }

    Item {
        id: mainContainer
        width: root.pane.width
        height: root.pane.height
        onHeightChanged: d.resize(); onWidthChanged: d.resize();

        Rectangle {
            id: opaqueRect
            height: parent.height
            width: parent.width
            opacity: 0.9
            color: "black"
        }

        Item {
            id: bannerContainer
            width: parent.width
            height: banner.height
            anchors {
                top: parent.top
                topMargin: 85
            }
            Image {
                id: banner
                anchors.centerIn: parent
                source: "../../images/high-fidelity-banner.svg"
                horizontalAlignment: Image.AlignHCenter
            }
        }

        Item {
            id: loggingInContainer
            width: parent.width
            height: parent.height
            onHeightChanged: d.resize(); onWidthChanged: d.resize();

            Item {
                id: loggingInHeader
                width: parent.width
                height: 0.5 * parent.height
                anchors {
                    top: parent.top
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
                    size: 31;
                    // Anchors
                    anchors.right: loggingInText.left;
                    anchors.rightMargin: loggingInBody.loggingInGlyphRightMargin
                    anchors.bottom: parent.bottom;
                    anchors.bottomMargin: hifi.dimensions.contentSpacing.y
                    // Alignment
                    horizontalAlignment: Text.AlignHCenter;
                    verticalAlignment: Text.AlignVCenter;
                    visible: loggingInBody.withSteam;
                }

                TextMetrics {
                    id: loggingInTextMetrics;
                    font: loggingInText.font;
                    text: loggingInText.text;
                }
                Text {
                    id: loggingInText;
                    width: loggingInTextMetrics.width
                    anchors.bottom: parent.bottom;
                    anchors.bottomMargin: hifi.dimensions.contentSpacing.y
                    anchors.left: parent.left;
                    anchors.leftMargin: (parent.width - loggingInTextMetrics.width) / 2
                    color: "white";
                    font.family: loggingInBody.fontFamily
                    font.pixelSize: loggingInBody.fontSize
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
                }
                AnimatedImage {
                    id: loggingInSpinner
                    source: "../../icons/loader-snake-64-w.gif"
                    width: 128
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
                    visible: loggingInBody.withSteam;
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

        onHandleSignupCompleted: {
            console.log("SignUp completed!");
        }

        onHandleSignupFailed: {
            console.log("SignUp failed!");
            var errorStringEdited = errorString.replace(/[\n\r]+/g, ' ');
            console.log(errorStringEdited);
            bodyLoader.setSource("SignInBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "errorString": errorStringEdited });
            // bodyLoader.setSource("SignInBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "errorString": "yellow" });
        }

        onHandleLoginCompleted: {
            console.log("Login Succeeded")
            loadingSuccess();
        }

        onHandleLoginFailed: {
            console.log("Login Failed")
            var errorString = "";
            if (loggingInBody.withSteam && loggingInBody.fromBody === "LinkAccountBody") {
                loggingInGlyph.visible = false;
                loggingInText.text = "Your Steam authentication has failed. Please make sure you are logged into Steam and try again."
                loggingInText.wrapMode = Text.WordWrap;
                loggingInText.anchors.centerIn = loggingInHeader;
                loggingInText.anchors.bottom = loggingInHeader.bottom;
                steamFailureTimer.start();
            } else if (loggingInBody.withSteam) {
                errorString = "Your Steam authentication has failed. Please make sure you are logged into Steam and try again.";
                bodyLoader.setSource("UsernameCollisionBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "errorString": errorString });
            } else {
                errorString = loginDialog.isLogIn ? "Username or password is incorrect." : "Failed to sign up. Please try again.";
                bodyLoader.setSource("SignInBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "errorString": errorString });
            }
        }
    }
}
