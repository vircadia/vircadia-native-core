//
//  CompleteProfileBody.qml
//
//  Created by Wayne Chen on 10/18/18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4
import QtQuick.Controls.Styles 1.4 as OriginalStyles

import "qrc:////qml//controls-uit" as HifiControlsUit
import "qrc:////qml//styles-uit" as HifiStylesUit

Item {
    id: completeProfileBody
    clip: true
    width: root.width
    height: root.height
    readonly property string termsContainerText: qsTr("By creating this user profile, you agree to High Fidelity's Terms of Service")
    readonly property string fontFamily: "Cairo"
    readonly property int fontSize: 24
    readonly property bool fontBold: true

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
                color: "red";
                font.family: "Cairo"
                font.pixelSize: 12
                text: ""
                visible: true
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
                anchors.top: loginErrorMessage.bottom
                anchors.topMargin: 2 * hifi.dimensions.contentSpacing.y
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignVCenter;
                visible: false;

            }
            Item {
                id: buttons
                width: Math.max(banner.width, cancelContainer.width + profileButton.width)
                height: d.minHeightButton
                anchors {
                    top: parent.top
                    topMargin: (parent.height - additionalTextContainer.height) / 2 - hifi.dimensions.contentSpacing.y
                    left: parent.left
                    leftMargin: (parent.width - banner.width) / 2
                }
                Item {
                    id: cancelContainer
                    width: cancelText.width
                    height: d.minHeightButton
                    anchors {
                        top: parent.top
                        left: parent.left
                    }
                    Text {
                        id: cancelText
                        text: qsTr("Cancel");

                        lineHeight: 1
                        color: "white"
                        font.family: completeProfileBody.fontFamily
                        font.pixelSize: completeProfileBody.fontSize
                        font.capitalization: Font.AllUppercase;
                        font.bold: completeProfileBody.fontBold
                        lineHeightMode: Text.ProportionalHeight
                        anchors.centerIn: parent
                    }
                    MouseArea {
                        id: cancelArea
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        onClicked: {
                            bodyLoader.setSource("LinkAccountBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader });
                        }
                    }
                }
                TextMetrics {
                    id: profileButtonTextMetrics
                    font: cancelText.font
                    text: qsTr("CREATE YOUR PROFILE")
                }
                HifiControlsUit.Button {
                    id: profileButton
                    anchors {
                        top: parent.top
                        right: parent.right
                    }
                    width: Math.max(profileButtonTextMetrics.width + 2 * hifi.dimensions.contentSpacing.x, d.minWidthButton)
                    height: d.minHeightButton

                    text: qsTr("Create your profile")
                    color: hifi.buttons.blue

                    fontFamily: completeProfileBody.fontFamily
                    fontSize: completeProfileBody.fontSize
                    fontBold: completeProfileBody.fontBold
                    onClicked: {
                        loginErrorMessage.visible = false;
                        loginDialog.createAccountFromSteam();
                        bodyLoader.setSource("LoggingInBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader,
                            "withSteam": true, "fromBody": "CompleteProfileBody" })
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
                    bodyLoader.setSource("SignInBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "errorString": "" });
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

                onLinkActivated: loginDialog.openUrl(link)
            }
        }
    }

    Component.onCompleted: {
        d.resize();
    }
}
