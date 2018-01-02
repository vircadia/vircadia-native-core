import QtQuick 2.5
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3

import "."
import "../../styles-uit"
import "../audio" as HifiAudio

Item {
    id: tablet
    objectName: "tablet"
    property var tabletProxy: Tablet.getTablet("com.highfidelity.interface.tablet.system");
    
    Rectangle {
        id: bgTopBar
        height: 90

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#2b2b2b"
            }

            GradientStop {
                position: 1
                color: "#1e1e1e"
            }
        }

        HifiAudio.MicBar {
            anchors {
                left: parent.left
                leftMargin: 30
                verticalCenter: parent.verticalCenter
            }
        }

        Item {
            width: 150
            height: 50
            anchors.right: parent.right
            anchors.rightMargin: 30
            anchors.verticalCenter: parent.verticalCenter

            ColumnLayout {
                anchors.fill: parent

                RalewaySemiBold {
                    text: Account.loggedIn ? qsTr("Log out") : qsTr("Log in")
                    horizontalAlignment: Text.AlignRight
                    anchors.right: parent.right
                    font.pixelSize: 20
                    color: "#afafaf"
                }

                RalewaySemiBold {
                    visible: Account.loggedIn
                    height: Account.loggedIn ? parent.height/2 - parent.spacing/2 : 0
                    text: Account.loggedIn ? "[" + tabletRoot.usernameShort + "]" : ""
                    horizontalAlignment: Text.AlignRight
                    anchors.right: parent.right
                    font.pixelSize: 20
                    color: "#afafaf"
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (!Account.loggedIn) {
                        DialogsManager.showLoginDialog()
                    } else {
                        Account.logOut()
                    }
                }
            }
        }
    }

    Rectangle {
        id: bgMain
        clip: true
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#2b2b2b"
            }

            GradientStop {
                position: 1
                color: "#0f212e"
            }
        }
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: bgTopBar.bottom

        GridView {
            id: flickable
            anchors.top: parent.top
            anchors.topMargin: 15
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            width: cellWidth * 3
            cellHeight: 145
            cellWidth: 145
            model: tabletProxy.buttons
            delegate: Item {
                width: flickable.cellWidth
                height: flickable.cellHeight
                property var proxy: modelData

                TabletButton {
                    id: tabletButton
                    anchors.centerIn: parent
                    onClicked: modelData.clicked()
                    state: wrapper.GridView.isCurrentItem ? "hover state" : "base state"
                }

                Connections {
                    target: modelData;
                    onPropertiesChanged:  {
                        updateProperties();
                    }
                }

                Component.onCompleted: updateProperties()

                function updateProperties() {
                    var keys = Object.keys(modelData.properties).forEach(function (key) {
                        if (tabletButton[key] !== modelData.properties[key]) {
                            tabletButton[key] = modelData.properties[key];
                        }
                    });
                }
            }
        }
    }

    Keys.onRightPressed: flickable.moveCurrentIndexRight();
    Keys.onLeftPressed: flickable.moveCurrentIndexLeft();
    Keys.onDownPressed: flickable.moveCurrentIndexDown();
    Keys.onUpPressed: flickable.moveCurrentIndexUp();
    Keys.onReturnPressed: {
        if (flickable.currentItem) {
            flickable.currentItem.proxy.clicked();
            if (tabletRoot) {
                tabletRoot.playButtonClickSound();
            }
        }
    }
}
