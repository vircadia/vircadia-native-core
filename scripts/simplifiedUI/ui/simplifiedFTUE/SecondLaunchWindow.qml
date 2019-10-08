//
//  SecondLaunchWindow.qml
//
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import QtQuick.Controls 2.3
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3
import stylesUit 1.0 as HifiStylesUit
import TabletScriptingInterface 1.0
import hifi.simplifiedUI.simplifiedConstants 1.0 as SimplifiedConstants
import hifi.simplifiedUI.simplifiedControls 1.0 as SimplifiedControls

Rectangle {
    id: root
    color: simplifiedUI.colors.white
    anchors.fill: parent

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }

    Item {
        id: contentContainer
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: continueLink.top

        Image {
            id: avatarImage
            anchors.verticalCenter: parent.verticalCenter
            height: Math.max(parent.height - 48, 350)
            anchors.left: parent.left
            anchors.leftMargin: 12
            source: resourceDirectoryUrl + "qml/hifi/simplifiedUI/avatarApp/images/hero.png"
            mipmap: true
            fillMode: Image.PreserveAspectFit
        }

        Item {
            anchors.top: parent.top
            anchors.topMargin: 196
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 32
            anchors.left: avatarImage.right
            anchors.leftMargin: 48
            anchors.right: parent.right

            Flickable {
                id: textContainer
                clip: true
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                width: Math.min(700, parent.width)
                contentWidth: width
                contentHeight: contentItem.childrenRect.height
                interactive: contentHeight > height

                HifiStylesUit.RalewayBold {
                    id: headerText
                    text: "Stand out from the crowd!"
                    color: simplifiedUI.colors.text.black
                    size: 48
                    height: paintedHeight
                    wrapMode: Text.Wrap
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.rightMargin: 16
                }

                HifiStylesUit.RalewayRegular {
                    id: descriptionText
                    anchors.top: headerText.bottom
                    anchors.topMargin: 10
                    anchors.left: parent.left
                    width: parent.width - headerText.anchors.rightMargin
                    height: paintedHeight
                    text: "You can create and upload custom avatars from our Avatar Creator App. " +
                        "It's as easy as taking a selfie.<br>Available now on iOS and Android Platforms."
                    color: simplifiedUI.colors.text.black
                    size: 22
                    wrapMode: Text.Wrap
                }

                Item {
                    id: qrAndInstructionsContainer
                    anchors.top: descriptionText.bottom
                    anchors.topMargin: 24
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.rightMargin: 16
                    height: avatarAppQRCodeImage.height

                    Image {
                        id: avatarAppQRCodeImage
                        source: resourceDirectoryUrl + "qml/hifi/simplifiedUI/avatarApp/images/qrCode.jpg"
                        anchors.top: parent.top
                        anchors.left: parent.left
                        width: 130
                        height: width
                        mipmap: true
                        fillMode: Image.PreserveAspectFit
                    }

                    HifiStylesUit.RalewayBold {
                        id: instructionText
                        anchors.top: avatarAppQRCodeImage.top
                        anchors.bottom: avatarAppQRCodeImage.bottom
                        anchors.left: avatarAppQRCodeImage.right
                        anchors.leftMargin: 30
                        anchors.right: parent.right
                        text: "Use your mobile phone to scan this QR code."
                        color: simplifiedUI.colors.text.black
                        size: 22
                        wrapMode: Text.Wrap
                    }
                }
            }
        }

        SimplifiedControls.VerticalScrollBar {
            parent: textContainer
            visible: parent.contentHeight > parent.height
            size: parent.height / parent.contentHeight
        }
    }
        

    HifiStylesUit.RalewayBold {
        id: continueLink
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.right: parent.right
        anchors.rightMargin: 16
        height: 96
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: "No thanks, I'll keep using my default avatar."
        color: simplifiedUI.colors.text.lightBlue
        opacity: continueMouseArea.containsMouse ? 1.0 : 0.7
        size: 24

        MouseArea {
            id: continueMouseArea
            hoverEnabled: true
            anchors.fill: parent

            onClicked: {
                Tablet.playSound(TabletEnums.ButtonClick);
                sendToScript({
                    "source": "SecondLaunchWindow.qml",
                    "method": "closeSecondLaunchWindow"
                });
            }
        }
    }

    Image {
        id: topLeftAccentImage
        width: 130
        height: 320
        anchors.left: parent.left
        anchors.top: parent.top
        source: "images/standOutTopLeft.png"
    }

    Image {
        id: bottomRightAccentImage
        width: 250
        height: 80
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        source: "images/standOutBottomRight.png"
    }

    signal sendToScript(var message);
}
