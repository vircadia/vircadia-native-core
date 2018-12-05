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

    QtObject {
        id: d
        readonly property int minWidth: 480
        readonly property int minWidthButton: !root.isTablet ? 256 : 174
        property int maxWidth: root.isTablet ? 1280 : root.width
        readonly property int minHeight: 120
        readonly property int minHeightButton: 36
        property int maxHeight: root.isTablet ? 720 : root.height

        function resize() {
            maxWidth = root.isTablet ? 1280 : root.width;
            maxHeight = root.isTablet ? 720 : root.height;
            if (root.isTablet === false) {
                var targetWidth = Math.max(Math.max(titleWidth, Math.max(additionalTextContainer.contentWidth,
                                                                termsContainer.contentWidth)), mainContainer.width)
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

            TextMetrics {
                id: loginErrorMessageTextMetrics
                font: loginErrorMessage.font
                text: loginErrorMessage.text
            }
            Text {
                id: loginErrorMessage
                anchors.top: parent.top;
                // above buttons.
                anchors.topMargin: (parent.height - additionalTextContainer.height) / 2 - hifi.dimensions.contentSpacing.y - profileButton.height
                anchors.left: parent.left;
                anchors.leftMargin: (parent.width - loginErrorMessageTextMetrics.width) / 2;
                color: "red";
                font.family: completeProfileBody.fontFamily
                font.pixelSize: completeProfileBody.fontSize
                text: completeProfileBody.errorString
                visible: true
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
                        loginErrorMessage.visible = false;
                        loginDialog.createAccountFromSteam();
                    }
                }
            }

            HifiStylesUit.ShortcutText {
                id: additionalTextContainer
                anchors {
                    top: buttons.bottom
                    horizontalCenter: parent.horizontalCenter
                    margins: 0
                    topMargin: hifi.dimensions.contentSpacing.y
                }

                text: "<a href='https://fake.link'>Already have a High Fidelity profile? Link to an existing profile here.</a>"

                font.family: completeProfileBody.fontFamily
                font.pixelSize: 14
                font.bold: completeProfileBody.fontBold
                wrapMode: Text.WordWrap
                lineHeight: 2
                lineHeightMode: Text.ProportionalHeight
                horizontalAlignment: Text.AlignHCenter

                onLinkActivated: {
                    loginDialog.isLogIn = true;
                    bodyLoader.setSource("LinkAccountBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "errorString": "", "linkSteam": true });
                }
            }

            TextMetrics {
                id: termsContainerTextMetrics
                font: termsContainer.font
                text: completeProfileBody.termsContainerText
                Component.onCompleted: {
                    // with the link.
                    termsContainer.text = qsTr("By creating this user profile, you agree to <a href='https://highfidelity.com/terms'>High Fidelity's Terms of Service</a>")
                }
            }

            HifiStylesUit.InfoItem {
                id: termsContainer
                anchors {
                    top: additionalTextContainer.bottom
                    margins: 0
                    topMargin: 2 * hifi.dimensions.contentSpacing.y
                    horizontalCenter: parent.horizontalCenter
                    left: parent.left
                    leftMargin: (parent.width - termsContainerTextMetrics.width) / 2
                }

                text: completeProfileBody.termsContainerText
                font.family: completeProfileBody.fontFamily
                font.pixelSize: 14
                font.bold: completeProfileBody.fontBold
                wrapMode: Text.WordWrap
                color: hifi.colors.baseGrayHighlight
                lineHeight: 1
                lineHeightMode: Text.ProportionalHeight

                onLinkActivated: loginDialog.openUrl(link);
            }
        }
    }

    Connections {
        target: loginDialog
        onHandleCreateCompleted: {
            console.log("Create Succeeded")

            loginDialog.loginThroughSteam();
            bodyLoader.setSource("LoggingInBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "withSteam": true, "linkSteam": false });
        }
        onHandleCreateFailed: {
            console.log("Create Failed: " + error);

            bodyLoader.setSource("UsernameCollisionBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader });
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
