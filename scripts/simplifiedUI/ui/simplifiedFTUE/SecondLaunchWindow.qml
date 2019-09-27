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

    Image {
        id: topLeftAccentImage
        width: 60
        height: 150
        anchors.left: parent.left
        anchors.top: parent.top
        source: "images/standOutTopLeft.png"
    }

    Image {
        id: bottomRightAccentImage
        width: 30
        height: 100
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        source: "images/standOutBottomRight.png"
    }

    Item {

        GridLayout {
            id: controlsPageGrid
            anchors.fill: parent
            flow: root.width < root.height ? GridLayout.LeftToRight : GridLayout.TopToBottom

            Item {
                id: textAndQRContainer

                HifiStylesUit.GraphikSemiBold {
                    id: headerText
                    width: 700
                    height: 120
                    text: "Stand out from the crowd!"
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: simplifiedUI.colors.text.darkGrey
                    size: 36
                }

                HifiStylesUit.GraphikSemiBold {
                    width: 700
                    height: 250
                    text: "You can create and upload custom avatars from our<br></br>
                        Avatar Creator App. It's as easy as taking a selfie.<br></br>
                        Available now on iOS and Android Platforms."
                    anchors.top: headerText.bottom
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: simplifiedUI.colors.text.darkGrey
                    size: 24
                }

                Item {
                    id: qrAndInstructionsContainer

                    Image {
                        id: avatarAppQRCodeImage
                        width: 200
                        height: 200
                        source: "images/qrCode.png"
                    }

                    HifiStylesUit.GraphikSemiBold {
                        width: 600
                        height: 80
                        text: "Use your mobile phone to scan this QR code."
                        anchors.top: avatarAppQRCodeImage.bottom
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: simplifiedUI.colors.text.darkGrey
                        size: 24
                    }
                }

                HifiStylesUit.GraphikSemiBold {
                    text: "No thanks, I'll keep using my default avatar."
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: simplifiedUI.colors.text.lightBlue
                    opacity: noThanksMouseArea.containsMouse ? 1.0 : 0.8
                    size: 12

                    MouseArea {
                        id: noThanksMouseArea
                        hoverEnabled: false
                        anchors.fill: parent

                        onClicked: {
                            Tablet.playSound(TabletEnums.ButtonClick);
                            Print("NO THANKS CLICKED");
                            sendToScript({
                                "source": "SecondLaunchWindow.qml",
                                "method": "closeInitialLaunchWindow"
                            });
                        }
                    }
                }
            }
        }

        Item {
            id: heroImageContainer
            Image {
                id: heroImage
                width: 600
                height: 350
                source: "images/hero.png"
            }
        }
    }

    signal sendToScript(var message);
}
