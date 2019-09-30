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
    color: "#ffffff"
    anchors.fill: parent

    Image {
        id: topLeftAccentImage
        width: 400
        height: 180
        anchors.left: parent.left
        anchors.top: parent.top
        source: "images/standOutTopLeft.png"
    }

    Image {
        id: bottomRightAccentImage
        width: 80
        height: 250
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        source: "images/standOutBottomRight.png"
    }

    Item {
        id: tempAvatarPageContainer

        GridLayout {
            id: tempAvatarPageGrid
            anchors.fill: parent
            flow: root.width < root.height ? GridLayout.LeftToRight : GridLayout.TopToBottom
            columns: root.width > root.height ? 2 : 1
            rows: root.width > root.height ? 1 : 2
            anchors.leftMargin: 180
            anchors.topMargin: 50
            anchors.bottomMargin: 50
            anchors.rightMargin: 100

            Item {
                id: textAndQRContainer
                width: 650
                Layout.topMargin: 80

                HifiStylesUit.GraphikSemiBold {
                    id: headerText
                    text: "Stand out from the crowd!"
                    color: "#000000"
                    size: 48
                }

                HifiStylesUit.GraphikSemiBold {
                    id: descriptionText
                    anchors.top: headerText.bottom
                    anchors.topMargin: 20
                    text: "You can create and upload custom avatars from our<br></br>
                        Avatar Creator App. It's as easy as taking a selfie.<br></br>
                        Available now on iOS and Android Platforms."
                    color: "#000000"
                    size: 24
                }

                Item {
                    id: qrAndInstructionsContainer
                    anchors.top: descriptionText.bottom
                    height: avatarAppQRCodeImage.height
                    width: parent.width
                    anchors.topMargin: 50

                    Image {
                        id: avatarAppQRCodeImage
                        source: "images/qrCode.jpg"
                        height: 200
                        width: 200
                    }

                    HifiStylesUit.GraphikSemiBold {
                        id: instructionText
                        anchors.top: avatarAppQRCodeImage.bottom
                        anchors.horizontalCenter: avatarAppQRCodeImage.horizontalCenter
                        anchors.topMargin: 50
                        text: "Use your mobile phone to scan this QR code."
                        color: "#000000"
                        size: 24
                    }
                }

                HifiStylesUit.RalewayBold {
                    text: "No thanks, I'll keep using my default avatar."
                    anchors.top: qrAndInstructionsContainer.bottom
                    anchors.topMargin: 50
                    anchors.horizontalCenter: qrAndInstructionsContainer.horizontalCenter
                    color: "#000000"
                    opacity: continueMouseArea.containsMouse ? 1.0 : 0.8
                    size: 20
                    z: 1

                    MouseArea {
                        id: continueMouseArea
                        hoverEnabled: true
                        anchors.fill: parent

                        onClicked: {
                            Tablet.playSound(TabletEnums.ButtonClick);
                            print("NO THANKS CLICKED");
                            sendToScript({
                                "source": "SecondLaunchWindow.qml",
                                "method": "closeSecondLaunchWindow"
                            });
                        }
                    }
                }
            }
        }

        Item {
            id: heroImageContainer
            Layout.leftMargin: 50
            // these don't change when the window resizes
            width: tempAvatarImage.width
            height: tempAvatarImage.height

            Image {
                id: heroImage
                // if I use preferred width and height, the image does not update when window changes size
                width: tempAvatarPageGrid.flow === GridLayout.LeftToRight ? 400 : 100
                height: tempAvatarPageGrid.flow === GridLayout.LeftToRight ? 748 : 187
                source: "images/hero.png"
            }
        }
    }

    signal sendToScript(var message);
}
