//
//  CompleteProfileBody.qml
//
//  Created by Clement on 7/18/16
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4
import QtQuick.Controls.Styles 1.4 as OriginalStyles

import controlsUit 1.0 as HifiControlsUit
import stylesUit 1.0 as HifiStylesUit
import TabletScriptingInterface 1.0

Item {
    id: completeProfileBody
    clip: true
    width: root.width
    height: root.height
    readonly property string termsContainerText: qsTr("By creating this user profile, you agree to High Fidelity's Terms of Service")
    readonly property string fontFamily: "Raleway"
    readonly property int fontSize: 15
    readonly property bool fontBold: true

    readonly property bool withSteam: withSteam
    property string errorString: errorString

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
            if (root.isTablet === false) {
                var targetWidth = Math.max(Math.max(titleWidth, Math.max(additionalTextContainer.width,
                                                                termsContainer.width)), mainContainer.width)
                parent.width = root.width = Math.max(d.minWidth, Math.min(d.maxWidth, targetWidth))
            }
            var targetHeight = Math.max(5 * hifi.dimensions.contentSpacing.y + d.minHeightButton + additionalTextContainer.height + termsContainer.height, d.maxHeight)
            parent.height = root.height = Math.max(d.minHeight, Math.min(d.maxHeight, targetHeight))
        }
    }

    Item {
        id: mainContainer
        width: root.width
        height: root.height
        onHeightChanged: d.resize(); onWidthChanged: d.resize();

        Item {
            id: contentItem
            anchors.fill: parent

            Item {
                id: errorContainer
                width: parent.width
                height: loginErrorMessageTextMetrics.height
                anchors {
                    bottom: buttons.top;
                    bottomMargin: hifi.dimensions.contentSpacing.y;
                    left: buttons.left;
                }
                TextMetrics {
                    id: loginErrorMessageTextMetrics
                    font: loginErrorMessage.font
                    text: loginErrorMessage.text
                }
                Text {
                    id: loginErrorMessage;
                    width: root.bannerWidth
                    color: "red";
                    font.family: completeProfileBody.fontFamily
                    font.pixelSize: 18
                    font.bold: completeProfileBody.fontBold
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    text: completeProfileBody.errorString
                    visible: true
                }
                Component.onCompleted: {
                    if (loginErrorMessageTextMetrics.width > root.bannerWidth && root.isTablet) {
                        loginErrorMessage.wrapMode = Text.WordWrap;
                        loginErrorMessage.verticalAlignment = Text.AlignLeft;
                        loginErrorMessage.horizontalAlignment = Text.AlignLeft;
                        errorContainer.height = 3 * loginErrorMessageTextMetrics.height;
                    }
                }
            }

            Item {
                id: buttons
                width: root.bannerWidth
                height: d.minHeightButton
                anchors {
                    top: parent.top
                    topMargin: (parent.height - additionalTextContainer.height) / 2 - hifi.dimensions.contentSpacing.y
                    left: parent.left
                    leftMargin: (parent.width - root.bannerWidth) / 2
                }
                HifiControlsUit.Button {
                    id: cancelButton
                    anchors {
                        top: parent.top
                        left: parent.left
                    }
                    width: (parent.width - hifi.dimensions.contentSpacing.x) / 2
                    height: d.minHeightButton

                    text: qsTr("CANCEL")
                    color: hifi.buttons.noneBorderlessWhite

                    fontFamily: completeProfileBody.fontFamily
                    fontSize: completeProfileBody.fontSize
                    fontBold: completeProfileBody.fontBold
                    onClicked: {
                        if (completeProfileBody.loginDialogPoppedUp) {
                            var data = {
                                "action": "user clicked cancel on the complete profile screen"
                            }
                            UserActivityLogger.logAction("encourageLoginDialog", data);
                        }

                        bodyLoader.setSource("LinkAccountBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader });
                    }
                }
                HifiControlsUit.Button {
                    id: profileButton
                    anchors {
                        top: parent.top
                        right: parent.right
                    }
                    width: (parent.width - hifi.dimensions.contentSpacing.x) / 2
                    height: d.minHeightButton

                    text: qsTr("Create your profile")
                    color: hifi.buttons.blue

                    fontFamily: completeProfileBody.fontFamily
                    fontSize: completeProfileBody.fontSize
                    fontBold: completeProfileBody.fontBold
                    onClicked: {
                        if (completeProfileBody.loginDialogPoppedUp) {
                            var data = {
                                "action": "user clicked create profile"
                            }
                            UserActivityLogger.logAction("encourageLoginDialog", data);
                        }
                        loginErrorMessage.visible = false;
                        loginDialog.createAccountFromSteam();
                    }
                }
            }

            Item {
                id: additionalTextContainer
                width: parent.width
                height: additionalTextMetrics.height
                anchors {
                    top: buttons.bottom
                    horizontalCenter: parent.horizontalCenter
                    topMargin: hifi.dimensions.contentSpacing.y
                    left: parent.left
                }

                TextMetrics {
                    id: additionalTextMetrics
                    font: additionalText.font
                    text: "Already have a High Fidelity profile? Link to an existing profile here."
                }

                HifiStylesUit.ShortcutText {
                    id: additionalText
                    text: "<a href='https://fake.link'>Already have a High Fidelity profile? Link to an existing profile here.</a>"

                    font.family: completeProfileBody.fontFamily
                    font.pixelSize: completeProfileBody.fontSize
                    font.bold: completeProfileBody.fontBold
                    wrapMode: Text.NoWrap
                    lineHeight: 1
                    lineHeightMode: Text.ProportionalHeight
                    horizontalAlignment: Text.AlignHCenter
                    linkColor: hifi.colors.blueAccent

                    onLinkActivated: {
                        loginDialog.isLogIn = true;
                        bodyLoader.setSource("LinkAccountBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "errorString": "", "withSteam": true, "linkSteam": true });
                    }
                    Component.onCompleted: {
                        if (additionalTextMetrics.width > root.bannerWidth && root.isTablet) {
                            additionalText.width = root.bannerWidth;
                            additionalText.wrapMode = Text.WordWrap;
                            additionalText.verticalAlignment = Text.AlignLeft;
                            additionalText.horizontalAlignment = Text.AlignLeft;
                            additionalTextContainer.height = (additionalTextMetrics.width / root.bannerWidth) * additionalTextMetrics.height;
                            additionalTextContainer.anchors.left = buttons.left;
                        } else {
                            additionalText.anchors.centerIn = additionalTextContainer;
                        }
                    }
                }
            }

            Item {
                id: termsContainer
                width: parent.width
                height: termsTextMetrics.height
                anchors {
                    top: additionalTextContainer.bottom
                    horizontalCenter: parent.horizontalCenter
                    topMargin: 2 * hifi.dimensions.contentSpacing.y
                    left: parent.left
                }
                TextMetrics {
                    id: termsTextMetrics
                    font: termsText.font
                    text: completeProfileBody.termsContainerText
                    Component.onCompleted: {
                        // with the link.
                        termsText.text = qsTr("By creating this user profile, you agree to <a href='https://highfidelity.com/terms'>High Fidelity's Terms of Service</a>")
                    }
                }

                HifiStylesUit.InfoItem {
                    id: termsText
                    text: completeProfileBody.termsContainerText
                    font.family: completeProfileBody.fontFamily
                    font.pixelSize: completeProfileBody.fontSize
                    font.bold: completeProfileBody.fontBold
                    wrapMode: Text.WordWrap
                    color: hifi.colors.lightGray
                    linkColor: hifi.colors.blueAccent
                    lineHeight: 1
                    lineHeightMode: Text.ProportionalHeight

                    onLinkActivated: loginDialog.openUrl(link);

                    Component.onCompleted: {
                        if (termsTextMetrics.width > root.bannerWidth && root.isTablet) {
                            termsText.width = root.bannerWidth;
                            termsText.wrapMode = Text.WordWrap;
                            additionalText.verticalAlignment = Text.AlignLeft;
                            additionalText.horizontalAlignment = Text.AlignLeft;
                            termsContainer.height = (termsTextMetrics.width / root.bannerWidth) * termsTextMetrics.height;
                            termsContainer.anchors.left = buttons.left;
                        } else {
                            termsText.anchors.centerIn = termsContainer;
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: loginDialog
        onHandleCreateCompleted: {
            console.log("Create Succeeded")

            if (completeProfileBody.withSteam) {
                if (completeProfileBody.loginDialogPoppedUp) {
                    var data = {
                        "action": "user created a profile with Steam successfully from the complete profile screen"
                    }
                    UserActivityLogger.logAction("encourageLoginDialog", data);
                }
                loginDialog.loginThroughSteam();
            }
            bodyLoader.setSource("LoggingInBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "withSteam": completeProfileBody.withSteam, "linkSteam": false });
        }
        onHandleCreateFailed: {
            console.log("Create Failed: " + error);
            if (completeProfileBody.withSteam) {
                if (completeProfileBody.loginDialogPoppedUp) {
                    var data = {
                        "action": "user failed to create a profile with Steam from the complete profile screen"
                    }
                    UserActivityLogger.logAction("encourageLoginDialog", data);
                }
            }

            bodyLoader.setSource("UsernameCollisionBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "withSteam": completeProfileBody.withSteam });
        }
    }

    Component.onCompleted: {
        //but rise Tablet's one instead for Tablet interface
        if (root.isTablet || root.isOverlay) {
            root.keyboardEnabled = HMD.active;
            root.keyboardRaised = Qt.binding( function() { return keyboardRaised; })
        }
        d.resize();
        root.text = "";
    }
}
