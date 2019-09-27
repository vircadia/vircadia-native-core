//
//  InitialLaunchWindow.qml
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

    Component.onCompleted: {
        if (Settings.getValue("simplifiedUI/alreadyAutoSelectedAvatar", false)) {
            tempAvatarPageContainer.visible = false;
            controlsContainer.visible = true;
        }
    }

    Image {
        id: topLeftAccentImage
        width: 60
        height: 150
        anchors.left: parent.left
        anchors.top: parent.top
        source: "images/defaultTopLeft.png"
    }

    Image {
        id: bottomRightAccentImage
        width: 30
        height: 100
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        source: "images/defaultBottomRight.png"
    }

    Item {
        id: tempAvatarPageContainer
        visible: true

        GridLayout {
            id: tempAvatarPageGrid
            anchors.fill: parent
            flow: root.width < root.height ? GridLayout.LeftToRight : GridLayout.TopToBottom

            Item {
                id: textAndQRContainer

                HifiStylesUit.GraphikSemiBold {
                    id: headerText
                    width: 700
                    height: 120
                    text: "We know this isn't you..."
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: simplifiedUI.colors.text.darkGrey
                    size: 36
                }

                HifiStylesUit.GraphikSemiBold {
                    width: 700
                    height: 500
                    text: "But, we've given you this <b>temporary avatar</b> to use<br></br>
                    for today. If you see this avatar in-world, walk up and<br></br>
                    say hello to other new users!<br></br><br></br>
                    <b>We want you to be you</b> so we've built an Avatar Creator<br></br>
                    App that's as easy as taking a selfie and picking your<br></br>
                    outfits! Available now on iOS and Android Platforms."
                    anchors.top: headerText.bottom
                    horizontalAlignment: Text.AlignHLeft
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
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        source: "images/qrCode.png"
                    }

                    HifiStylesUit.GraphikSemiBold {
                        width: 600
                        height: 80
                        text: "Use your mobile phone to scan this QR code."
                        anchors.left: avatarAppQRCodeImage.right
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: simplifiedUI.colors.text.darkGrey
                        size: 24
                    }
                }

                HifiStylesUit.GraphikSemiBold {
                    width: 250
                    height: 120
                    text: "Continue"
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: simplifiedUI.colors.text.lightBlue
                    opacity: continueMouseArea.containsMouse ? 1.0 : 0.8
                    size: 30

                    MouseArea {
                        id: continueMouseArea
                        hoverEnabled: false
                        anchors.fill: parent

                        onClicked: {
                            Tablet.playSound(TabletEnums.ButtonClick);
                            Print("CONTINUE CLICKED");
                            tempAvatarPageContainer.visible = false;
                            controlsContainer.visible = true;
                        }
                    }
                }
            }

            Item {
                id: tempAvatarImageContainer
                Image {
                    id: tempAvatarImage
                    width: tempAvatarPageGrid.flow === GridLayout.LeftToRight ? 250 : 500
                    height: tempAvatarPageGrid.flow === GridLayout.LeftToRight ? 500 : 1000
                    source: "images/DefaultAvatar_" + MyAvatar.skeletonModelURL.substring(123, MyAvatar.skeletonModelURL.length - 11) + ".png"
                }
            }
        }
    }

    Item {
        id: controlsContainer
        visible: false

        HifiStylesUit.GraphikSemiBold {
            text: "These are your avatar controls to<br></br>
            <b>Interact with and move around in your new HQ.</b>"
            anchors.fill: parent
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: simplifiedUI.colors.text.darkGrey
            size: 34
        }

        GridLayout {
            Item {
                id: controlsImagesContainer
                Item {
                    Image {
                        id: walkingControls
                        width: 500
                        height: 350
                        source: "images/walkingControls.png"
                    }
                }

                Item {
                    Image {
                        id: mouseControls
                        width: 600
                        height: 350
                        source: "images/mouseControls.png"
                    }
                }

                Item {
                    Image {
                        id: runJumpControls
                        width: 300
                        height: 250
                        source: "images/runJumpControls.png"
                    }
                }

                Item {
                    Image {
                        id: cameraControls
                        width: 500
                        height: 50
                        source: "images/cameraControls.png"
                    }
                }
            }
        }

        HifiStylesUit.GraphikSemiBold {
            text: "Learn more about our controls."
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: 200
            height: 50
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: simplifiedUI.colors.text.lightBlue
            opacity: learnMoreAboutControlsMouseArea.containsMouse ? 1.0 : 0.8
            size: 12

            MouseArea {
                id: learnMoreAboutControlsMouseArea
                hoverEnabled: false
                anchors.fill: parent

                onClicked: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                    // TODO open docs in browser
                    Print("LEARN MORE ABOUT CONTROLS CLICKED");
                }
            }
        }

        HifiStylesUit.GraphikSemiBold {
            text: "I've got a good grip on the controls."
            anchors.bottom: parent.bottom
            width: 700
            height: 120
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: simplifiedUI.colors.text.lightBlue
            opacity: goodGripMouseArea.containsMouse ? 1.0 : 0.8
            size: 30

            MouseArea {
                id: goodGripMouseArea
                hoverEnabled: false
                anchors.fill: parent

                onClicked: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                    Print("GOOD GRIP CLICKED");
                    sendToScript({
                        "source": "InitialLaunchWindow.qml",
                        "method": "closeInitialLaunchWindow"
                    });
                }
            }
        }
    }

    signal sendToScript(var message);
}
